/* gamma-coopgamma.h -- coopgamma gamma adjustment source
   This file is part of Redshift.

   Redshift is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Redshift is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Redshift.  If not, see <http://www.gnu.org/licenses/>.

   Copyright (c) 2016  Mattias Andrée <maandree@member.fsf.org>
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#else
# define PACKAGE "redshift"
#endif

#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif

#include "gamma-coopgamma.h"
#include "colorramp.h"


struct signal_blockage {
};

static int
unblocked_signal(int signo, struct signal_blockage *prev)
{
	/* TODO */
	return 0;
}


static int
restore_signal_blockage(int signo, const struct signal_blockage *blockage)
{
	/* TODO */
	return 0;
}


static int
update(coopgamma_state_t *state)
{
	for (size_t i = 0; i < state->n_crtcs; i++)
		libcoopgamma_set_gamma_sync(&state->crtcs[i].filter, &state->ctx);
}


static void
print_error(coopgamma_state_t *state)
{
	const char* side = state->ctx.error.server_side ? _("server-side") : _("client-side");
	if (state->ctx.error.custom) {
		if (state->ctx.error.number != 0 && state->ctx.error.description != NULL)
			fprintf(stderr, "%s error number %llu: %s\n",
				side, (unsigned long long int)state->ctx.error.number,
				state->ctx.error.description);
		else if (state->ctx.error.number != 0)
			fprintf(stderr, _("%s error number %llu\n"),
				side, (unsigned long long int)state->ctx.error.number);
		else if (state->ctx.error.description != NULL)
			fprintf(stderr, _("%s error: %s\n"), side, state->ctx.error.description);
	} else if (state->ctx.error.description != NULL) {
		fprintf(stderr, _("%s error: %s\n"), side, state->ctx.error.description);
	} else {
		fprintf(stderr, _("%s error: %s\n"), side, strerror(state->ctx.error.number));
	}
}


int
coopgamma_init(coopgamma_state_t *state)
{
	struct signal_blockage signal_blockage;
	memset(state, 0, sizeof(*state));
	if (libcoopgamma_context_initialise(&state->ctx)) {
		perror("libcoopgamma_context_initialise");
		return -1;
	}

	/* This is done this early to check if coopgamma is available */
	if (unblocked_signal(SIGCHLD, &signal_blockage) < 0)
		return -1;
	state->methods = libcoopgamma_get_methods();
	if (state->methods == NULL) {
		perror("libcoopgamma_get_methods");
		if (restore_signal_blockage(SIGCHLD, &signal_blockage) < 0)
			exit(EXIT_FAILURE);
		return -1;
	}
	if (restore_signal_blockage(SIGCHLD, &signal_blockage) < 0)
		return -1;

	state->priority = 0x0800000000000000LL;

	return 0;
}

int
coopgamma_start(coopgamma_state_t *state, program_mode_t mode)
{
	struct signal_blockage signal_blockage;
	libcoopgamma_lifespan_t lifespan;
	char** outputs;
	size_t i, j, n_outputs;
	int r;
	double d;

	switch (mode) {
	case PROGRAM_MODE_RESET:
		lifespan = LIBCOOPGAMMA_REMOVE;
		break;
	case PROGRAM_MODE_ONE_SHOT:
	case PROGRAM_MODE_MANUAL:
		lifespan = LIBCOOPGAMMA_UNTIL_REMOVAL;
		break;
	default:
		lifespan = LIBCOOPGAMMA_UNTIL_DEATH;
		break;
	}

	free(state->methods);
	state->methods = NULL;

	/* Connect to server */
	if (unblocked_signal(SIGCHLD, &signal_blockage) < 0)
		return -1;
	if (libcoopgamma_connect(state->method, state->site, &state->ctx) < 0) {
		if (errno)
			perror("libcoopgamma_connect");
		else
			fprintf(stderr, _("libcoopgamma_connect: could not "
					  "start coopgamma server\n"));
		if (restore_signal_blockage(SIGCHLD, &signal_blockage) < 0)
			exit(EXIT_FAILURE);
		return -1;
	}
	if (restore_signal_blockage(SIGCHLD, &signal_blockage) < 0)
		return -1;
	free(state->method);
	state->method = NULL;
	free(state->site);
	state->site = NULL;

	/* Get available outputs */
	outputs = libcoopgamma_get_crtcs_sync(&state->ctx);
	for (n_outputs = 0; outputs[n_outputs]; n_outputs++);

	/* List available output if edid=list was used */
	if (state->list_outputs) {
		if (outputs == NULL) {
			print_error(state);
			return -1;
		}
		printf(_("Available outputs:\n"));
		for (i = 0; outputs[i]; i++)
			printf("  %s\n", outputs[i]);
		if (ferror(stdout)) {
			perror("printf");
			exit(EXIT_FAILURE);
		}
		exit(EXIT_SUCCESS);
	}

	/* Translate crtc=N to edid=EDID */
	for (i = 0; i < state->n_outputs; i++) {
		if (state->outputs[i].edid != NULL)
			continue;
		if (state->outputs[i].index >= n_outputs) {
			fprintf(stderr, _("monitor number %zu does not exist,"
					  "available monitors are [0, %zu]\n"),
				state->outputs[i].index, n_outputs - 1);
			return -1;
		}
		state->outputs[i].edid = strdup(outputs[state->outputs[i].index]);
		if (state->outputs[i].edid == NULL) {
			perror("strdup");
			return -1;
		}
	}

	/* Use all outputs if none were specified */
	if (state->n_outputs == 0) {
		state->n_outputs = state->a_outputs = n_outputs;
		state->outputs = malloc(n_outputs * sizeof(*state->outputs));
		if (state->outputs == NULL) {
			perror("malloc");
			return -1;
		}
		for (i = 0; i < n_outputs; i++) {
			state->outputs[i].edid = strdup(outputs[i]);
			if (state->outputs[i].edid == NULL) {
				perror("strdup");
				return -1;
			}
		}
	}

	free(outputs);

	/* Initialise information for each output */
	state->crtcs = calloc(state->n_outputs, sizeof(*state->crtcs));
	if (state->crtcs == NULL) {
		perror("calloc");
		return -1;
	}
	for (i = 0; i < state->n_outputs; i++) {
		libcoopgamma_crtc_info_t info;
		coopgamma_crtc_state_t *crtc = state->crtcs + state->n_crtcs;

		crtc->filter.priority = state->priority;
		crtc->filter.crtc = state->outputs[i].edid;
		crtc->filter.class = PACKAGE "::redshift::standard";
		crtc->filter.lifespan = lifespan;

		if (libcoopgamma_get_gamma_info_sync(crtc->filter.crtc, &info, &state->ctx) < 0) {
			int saved_errno = errno;
			fprintf(stderr, _("failed to retrieve information for output `%s':\n"),
				outputs[i]);
			errno = saved_errno;
			print_error(state);
			return -1;
		}
		if (!info.cooperative) {
			fprintf(stderr, _("coopgamma is not available\n"));
			return -1;
		}
		if (info.supported == LIBCOOPGAMMA_NO) {
			fprintf(stderr, _("output `%s' does not support gamma "
					  "adjustments, skipping\n"), outputs[i]);
			continue;
		}

		/* Get total size of the ramps */
		switch (info.depth) {
#define X(SUFFIX, TYPE, MAX, TRUE_MAX, DEPTH)\
		case DEPTH:\
			crtc->rampsize = sizeof(TYPE);\
			break;
		LIST_RAMPS_STOP_VALUE_TYPES
#undef X
		default:
			if (info.depth > 0)
				fprintf(stderr, _("output `%s' uses an unsupported depth "
						  "for its gamma ramps: %i bits, skipping\n"),
					outputs[i], info.depth);
			else
				fprintf(stderr, _("output `%s' uses an unrecognised depth, "
						  "for its gamma ramps, with the code %i, "
						  "skipping\n"), outputs[i], info.depth);
			continue;
		}
		crtc->rampsize *= info.red_size + info.green_size + info.blue_size;

		crtc->filter.depth = info.depth;
		crtc->filter.ramps.u8.red_size = info.red_size;
		crtc->filter.ramps.u8.green_size = info.green_size;
		crtc->filter.ramps.u8.blue_size = info.blue_size;
		crtc->plain_ramps.u8.red_size = info.red_size;
		crtc->plain_ramps.u8.green_size = info.green_size;
		crtc->plain_ramps.u8.blue_size = info.blue_size;

		/* Initialise plain ramp and working ramp */
#define float f
#define double d
		switch (info.depth) {
#define X(SUFFIX, TYPE, MAX, TRUE_MAX, DEPTH)\
		case DEPTH:\
			r = libcoopgamma_ramps_initialise(&crtc->filter.ramps.SUFFIX);\
			if (r < 0) {\
				perror("libcoopgamma_ramps_initialise");\
				return -1;\
			}\
			r = libcoopgamma_ramps_initialise(&crtc->plain_ramps.SUFFIX);\
			if (r < 0) {\
				perror("libcoopgamma_ramps_initialise");\
				return -1;\
			}\
			for (j = 0; j < crtc->plain_ramps.SUFFIX.red_size; j++) {\
				d = j;\
				d /= crtc->plain_ramps.SUFFIX.red_size;\
				crtc->plain_ramps.SUFFIX.red[j] = d * TRUE_MAX;\
			}\
			for (j = 0; j < crtc->plain_ramps.SUFFIX.green_size; j++) {\
				d = j;\
				d /= crtc->plain_ramps.SUFFIX.green_size;\
				crtc->plain_ramps.SUFFIX.green[j] = d * TRUE_MAX;\
			}\
			for (j = 0; j < crtc->plain_ramps.SUFFIX.blue_size; j++) {\
				d = j;\
				d /= crtc->plain_ramps.SUFFIX.blue_size;\
				crtc->plain_ramps.SUFFIX.blue[j] = d * TRUE_MAX;\
			}\
			break;
		LIST_RAMPS_STOP_VALUE_TYPES
#undef X
		default:
			abort();
		}
#undef float
#undef double

		state->outputs[i].edid = NULL;
		state->n_crtcs++;
	}

	free(state->outputs);
	state->outputs = NULL;
	state->n_outputs = 0;

	return 0;
}

void
coopgamma_free(coopgamma_state_t *state)
{
	free(state->methods);
	state->methods = NULL;
	free(state->method);
	state->method = NULL;
	free(state->site);
	state->site = NULL;

	while (state->n_crtcs--) {
		state->crtcs[state->n_crtcs].filter.class = NULL;
		libcoopgamma_filter_destroy(&state->crtcs[state->n_crtcs].filter);
		libcoopgamma_ramps_destroy(&state->crtcs[state->n_crtcs].plain_ramps);
	}
	state->n_crtcs = 0;
	free(state->crtcs);
	state->crtcs = NULL;

	libcoopgamma_context_destroy(&state->ctx, 1);
	while (state->n_outputs--)
		free(state->outputs[state->n_outputs].edid);
	state->n_outputs = 0;
	free(state->outputs);
	state->outputs = NULL;
}

void
coopgamma_print_help(FILE *f)
{
	fputs(_("Adjust gamma ramps with coopgamma.\n"), f);
	fputs("\n", f);

	/* TRANSLATORS: coopgamma help output
	   left column must not be translated */
	fputs(_("  edid=EDID      \tEDID of monitor to apply adjustments to, enter "
		"`list' to list available monitors\n"
		"  crtc=N         \tIndex of CRTC to apply adjustments to\n"
		"  priority=N     \tThe application order of the adjustments, "
		"default value is 576460752303423488.\n"
		"  method=METHOD  \tUnderlaying adjustment method, enter "
		"`list' to list available methods\n"
		"  display=DISPLAY\tThe display to apply adjustments to\n"),
	      f);
	fputs("\n", f);
}

int
coopgamma_set_option(coopgamma_state_t *state, const char *key, const char *value)
{
	size_t i;
	char *end;
	long long int priority;

	if (!strcasecmp(key, "priority")) {
		errno = 0;
		priority = strtoll(value, &end, 10);
		if (errno || *end || priority < INT64_MIN || priority > INT64_MAX) {
			fprintf(stderr, _("value of method parameter `crtc' "
					  "must be a integer in [%lli, %lli]\n"),
				(long long int)INT64_MIN, (long long int)INT64_MAX);
			return -1;
		}
		state->priority = priority;
	} else if (!strcasecmp(key, "method")) {
		if (state->method != NULL) {
			fprintf(stderr, _("method parameter `method' "
					  "can only be used once\n"));
			return -1;
		}
		if (!strcasecmp(value, "list")) {
			/* TRANSLATORS: coopgamma help output
			   the word "coopgamma" must not be translated */
			printf(_("Available adjustment methods for coopgamma:\n"));
			for (i = 0; state->methods[i]; i++)
				printf("  %s\n", state->methods[i]);
			if (ferror(stdout)) {
				perror("printf");
				exit(EXIT_FAILURE);
			}
			exit(EXIT_SUCCESS);
		}
		state->method = strdup(value);
		if (state->method == NULL) {
			perror("strdup");
			return -1;
		}
	} else if (!strcasecmp(key, "display")) {
		if (state->site != NULL) {
			fprintf(stderr, _("method parameter `display' "
					  "can only be used once\n"));
			return -1;
		}
		state->site = strdup(value);
		if (state->site == NULL) {
			perror("strdup");
			return -1;
		}
	} else if (!strcasecmp(key, "edid") || !strcasecmp(key, "crtc")) {
		if (state->n_outputs == state->a_outputs) {
			state->a_outputs += 8;
			state->outputs = realloc(state->outputs,
						 state->a_outputs * sizeof(*state->outputs));
			if (state->outputs == NULL) {
				perror("realloc");
				return -1;
			}
		}
		if (!strcasecmp(key, "edid")) {
			state->outputs[state->n_outputs].edid = strdup(value);
			if (state->outputs[state->n_outputs].edid == NULL) {
				perror("strdup");
				return -1;
			}
			if (!strcasecmp(state->outputs[state->n_outputs].edid, "list"))
				state->list_outputs = 1;
		} else {
			state->outputs[state->n_outputs].edid = NULL;
			errno = 0;
			state->outputs[state->n_outputs].index = (size_t)strtoul(value, &end, 10);
			if (!*end && errno == ERANGE &&
			    state->outputs[state->n_outputs].index == SIZE_MAX) {
				state->outputs[state->n_outputs].index = SIZE_MAX;
			} else if (errno || *end) {
				fprintf(stderr, _("value of method parameter `crtc' "
						  "must be a non-negative integer\n"));
				return -1;
			}
		}
		state->n_outputs++;
	} else {
		fprintf(stderr, _("Unknown method parameter: `%s'.\n"), key);
		return -1;
	}

	return 0;
}

void
coopgamma_restore(coopgamma_state_t *state)
{
	size_t i;
	for (i = 0; i < state->n_crtcs; i++)
		state->crtcs[i].filter.lifespan = LIBCOOPGAMMA_REMOVE;
	update(state);
	for (i = 0; i < state->n_crtcs; i++)
		state->crtcs[i].filter.lifespan = LIBCOOPGAMMA_UNTIL_DEATH;
}

int
coopgamma_set_temperature(coopgamma_state_t *state, const color_setting_t *setting)
{
	libcoopgamma_filter_t *filter;
	libcoopgamma_filter_t *last_filter = NULL;
	size_t i;

	for (i = 0; i < state->n_crtcs; i++, last_filter = filter) {
		filter = &state->crtcs[i].filter;

		/* Copy ramps for previous CRTC if its ramps is of same size and depth */
		if (last_filter != NULL &&
		    last_filter->ramps.u8.red_size   == filter->ramps.u8.red_size &&
		    last_filter->ramps.u8.green_size == filter->ramps.u8.green_size &&
		    last_filter->ramps.u8.blue_size  == filter->ramps.u8.blue_size) {
			memcpy(filter->ramps.u8.red, last_filter->ramps.u8.red,
			       state->crtcs[i].rampsize);
			continue;
		}

		/* Otherwise, create calculate the ramps */
		memcpy(filter->ramps.u8.red, state->crtcs[i].plain_ramps.u8.red,
		       state->crtcs[i].rampsize);
		switch (filter->depth) {
#define X(SUFFIX, TYPE, MAX, TRUE_MAX, DEPTH)\
		case DEPTH:\
			colorramp_fill_##SUFFIX((void *)(filter->ramps.u8.red),\
						(void *)(filter->ramps.u8.green),\
						(void *)(filter->ramps.u8.blue),\
						filter->ramps.u8.red_size,\
						filter->ramps.u8.green_size,\
						filter->ramps.u8.blue_size,\
						setting);\
			break;
		LIST_RAMPS_STOP_VALUE_TYPES
#undef X
		default:
			abort();
		}
	}

	return update(state);
}
