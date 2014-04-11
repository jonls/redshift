/* gamma-drm.c -- DRM gamma adjustment source
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

   Copyright (c) 2014  Mattias Andrée <maandree@member.fsf.org>
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <alloca.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <grp.h>

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif

#ifndef O_CLOEXEC
  #define O_CLOEXEC  02000000
#endif

#include "gamma-drm.h"
#include "colorramp.h"


static void
drm_free_partition(void *data)
{
	drm_card_data_t *card_data = data;
	if (card_data->res != NULL)
		drmModeFreeResources(card_data->res);
	if (card_data->fd >= 0)
		close(card_data->fd);
	free(data);
}

static void
drm_free_crtc(void *data)
{
	(void) data;
}

static int
drm_open_site(gamma_server_state_t *state, char *site, gamma_site_state_t *site_out)
{
	(void) state;
	(void) site;
	site_out->data = NULL;
	site_out->partitions_available = 0;

	/* Count the number of available graphics cards. */
	char pathname[PATH_MAX];
	struct stat _attr;
	while (1) {
		snprintf(pathname, PATH_MAX, DRM_DEV_NAME, DRM_DIR_NAME,
			 (int)site_out->partitions_available);
		if (stat(pathname, &_attr))
			break;
		site_out->partitions_available++;
	}

	return 0;
}

static int
drm_open_partition(gamma_server_state_t *state, gamma_site_state_t *site,
		   size_t partition, gamma_partition_state_t *partition_out)
{
	(void) state;
	(void) site;

	partition_out->data = NULL;
	drm_card_data_t *data = malloc(sizeof(drm_card_data_t));
	if (data == NULL) {
		perror("malloc");
		return -1;
	}
	data->fd = -1;
	data->res = NULL;
	data->index = partition;
	partition_out->data = data;

	/* Acquire access to a graphics card. */
	char pathname[PATH_MAX];
	snprintf(pathname, PATH_MAX, DRM_DEV_NAME, DRM_DIR_NAME, (int)partition);

	data->fd = open(pathname, O_RDWR | O_CLOEXEC);
	if (data->fd < 0) {
		if ((errno == ENXIO) || (errno == ENODEV)) {
			goto card_removed;
		} else if (errno == EACCES) {
			struct stat attr;
			int r;
			r = stat(pathname, &attr);
			if (r != 0) {
				if (errno == EACCES)
					goto card_removed;
				perror("stat");
#define __test(R, W) ((attr.st_mode & (R | W)) == (R | W))
			} else if ((attr.st_uid == geteuid() && __test(S_IRUSR, S_IWUSR)) ||
				   (attr.st_gid == getegid() && __test(S_IRGRP, S_IWGRP)) ||
				   __test(S_IROTH, S_IWOTH)) {
				fprintf(stderr, _("Failed to access the graphics card.\n"));
				fprintf(stderr, _("Perhaps it is locked.\n"));
			} else if (attr.st_gid == 0 /* root group */ || __test(S_IRGRP, S_IWGRP)) {
				fprintf(stderr,
					_("It appears that your system administrator have\n"
					  "restricted access to the graphics card."));
#undef __test
			} else {
				gid_t supplemental_groups[NGROUPS_MAX];
				r = getgroups(NGROUPS_MAX, supplemental_groups);
				if (r < 0) {
					perror("getgroups");
					goto card_error;
				}
				int i, n = r;
				for (i = 0; i < n; i++) {
					if (supplemental_groups[i] == attr.st_gid)
						break;
				}
				if (i != n) {
					fprintf(stderr, _("Failed to access the graphics card.\n"));
				} else {
					struct group *group = getgrgid(attr.st_gid);
					if (group == NULL || group->gr_name == NULL) {
						fprintf(stderr,
							_("You need to be in the group %i to used DRM.\n"),
							attr.st_gid);
					} else {
						fprintf(stderr,
							_("You need to be in the group `%s' to used DRM.\n"),
							group->gr_name);
					}
				}
			}
		} else {
			perror("open");
		}
		goto card_error;
	card_removed:
		fprintf(stderr, _("It appears that you have removed a graphics card.\n"
				  "Please do not do that, it's so rude. I cannot\n"
				  "imagine what issues it might have caused you.\n"));
	card_error:
		free(data);
		return -1;
	}

	/* Acquire mode resources. */
	data->res = drmModeGetResources(data->fd);
	if (data->res == NULL) {
		fprintf(stderr, _("Failed to get DRM mode resources.\n"));
		close(data->fd);
		free(data);
		return -1;
	}
	if (data->res->count_crtcs < 0) {
		fprintf(stderr, _("Got negative number of graphics cards from DRM.\n"));
		close(data->fd);
		free(data);
		return -1;
	}

	partition_out->crtcs_available = (size_t)(data->res->count_crtcs);
	return 0;
}

static int
drm_open_crtc(gamma_server_state_t *state, gamma_site_state_t *site,
	      gamma_partition_state_t *partition, size_t crtc, gamma_crtc_state_t *crtc_out)
{
	(void) state;
	(void) site;

	drm_card_data_t *card = partition->data;
	uint32_t crtc_id = card->res->crtcs[(size_t)crtc];
	crtc_out->data = (void*)(size_t)crtc_id;
	drmModeCrtc *crtc_info = drmModeGetCrtc(card->fd, crtc_id);
	if (crtc_info == NULL) {
		fprintf(stderr, _("Please do not unplug monitors!\n"));
		return -1;
	}

	ssize_t gamma_size = crtc_info->gamma_size;
	drmModeFreeCrtc(crtc_info);
	if (gamma_size < 2) {
		fprintf(stderr, _("Could not get gamma ramp size for CRTC %ld\n"
				  "on graphics card %ld.\n"),
			crtc, card->index);
		return -1;
	}
	crtc_out->saved_ramps.red_size   = (size_t)gamma_size;
	crtc_out->saved_ramps.green_size = (size_t)gamma_size;
	crtc_out->saved_ramps.blue_size  = (size_t)gamma_size;

	/* Valgrind complains about us reading uninitialize memory if we just use malloc. */
	crtc_out->saved_ramps.red   = calloc(3 * (size_t)gamma_size, sizeof(uint16_t));
	crtc_out->saved_ramps.green = crtc_out->saved_ramps.red   + gamma_size;
	crtc_out->saved_ramps.blue  = crtc_out->saved_ramps.green + gamma_size;
	if (crtc_out->saved_ramps.red == NULL) {
		perror("malloc");
		return -1;
	}

	int r = drmModeCrtcGetGamma(card->fd, crtc_id, gamma_size, crtc_out->saved_ramps.red,
				    crtc_out->saved_ramps.green, crtc_out->saved_ramps.blue);
	if (r < 0) {
		fprintf(stderr, _("DRM could not read gamma ramps on CRTC %ld on\n"
				  "graphics card %ld.\n"),
			crtc, card->index);
		return -1;
	}

	return 0;
}

static void
drm_invalid_partition(const gamma_site_state_t *site, size_t partition)
{
	fprintf(stderr, _("Card %ld does not exist. "),
		partition);
	if (site->partitions_available > 1) {
		fprintf(stderr, _("Valid cards are [0-%ld].\n"),
			site->partitions_available - 1);
	} else {
		fprintf(stderr, _("Only card 0 exists.\n"));
	}
}

static int
drm_set_ramps(gamma_server_state_t *state, gamma_crtc_state_t *crtc, gamma_ramps_t ramps)
{
	drm_card_data_t *card_data = state->sites[crtc->site_index].partitions[crtc->partition].data;
	int r;
	r = drmModeCrtcSetGamma(card_data->fd, (uint32_t)(long)(crtc->data),
				ramps.red_size, ramps.red, ramps.green, ramps.blue);
	if (r) {
		switch (errno) {
		case EACCES:
		case EAGAIN:
		case EIO:
			/* Permission denied errors must be ignored, because we do not
			   have permission to do this while a display server is active.
			   We are also checking for some other error codes just in case. */
		case EBUSY:
		case EINPROGRESS:
			/* It is hard to find documentation for DRM (in fact all of this is
			   just based on the functions names and some testing,) perhaps we
			   could get this if we are updating to fast. */
			break;
		case EBADF:
		case ENODEV:
		case ENXIO:
			/* XXX: I have not actually tested removing my graphics card or,
			        monitor but I imagine either of these is what would happen. */
			fprintf(stderr,
				_("Please do not unplug your monitors or remove graphics cards.\n"));
			return -1;
		default:
			perror("drmModeCrtcSetGamma");
			return -1;
		}
	}
	return 0;
}

static int
drm_set_option(gamma_server_state_t *state, const char *key, char *value, ssize_t section)
{
	if (strcasecmp(key, "card") == 0) {
		ssize_t card = strcasecmp(value, "all") ? (ssize_t)atoi(value) : -1;
		if (card < 0 && strcasecmp(value, "all")) {
			/* TRANSLATORS: `all' must not be translated. */
			fprintf(stderr, _("Card must be `all' or a non-negative integer.\n"));
			return -1;
		}
		on_selections({ sel->partition = card; });
		return 0;
	} else if (strcasecmp(key, "crtc") == 0) {
		ssize_t crtc = strcasecmp(value, "all") ? (ssize_t)atoi(value) : -1;
		if (crtc < 0 && strcasecmp(value, "all")) {
			/* TRANSLATORS: `all' must not be translated. */
			fprintf(stderr, _("CRTC must be `all' or a non-negative integer.\n"));
			return -1;
		}
		on_selections({ sel->crtc = crtc; });
		return 0;
	}
	return 1;
}

int
drm_init(gamma_server_state_t *state)
{
	int r;
	r = gamma_init(state);
	if (r != 0) return r;

	state->free_partition_data = drm_free_partition;
	state->free_crtc_data      = drm_free_crtc;
	state->open_site           = drm_open_site;
	state->open_partition      = drm_open_partition;
	state->open_crtc           = drm_open_crtc;
	state->invalid_partition   = drm_invalid_partition;
	state->set_ramps           = drm_set_ramps;
	state->set_option          = drm_set_option;

	return 0;
}

int
drm_start(gamma_server_state_t *state)
{
	return gamma_resolve_selections(state);
}

void
drm_print_help(FILE *f)
{
	fputs(_("Adjust gamma ramps with Direct Rendering Manager.\n"), f);
	fputs("\n", f);

	/* TRANSLATORS: DRM help output
	   left column must not be translated */
	fputs(_("  card=N\tGraphics card to apply adjustments to\n"
		"  crtc=N\tCRTC to apply adjustments to\n"), f);
	fputs("\n", f);
}
