/* redshift-config-gtk.c -- GTK+ GUI config tool
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

   Copyright (c) 2018 Masanori Kakura <kakurasan@gmail.com>
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef ENABLE_NLS
# include <glib/gi18n.h>
#endif

#if !defined(__WIN32__)
# include <glib-unix.h>
#endif

#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <stdio.h>

#include "redshift.h"
#include "colorramp.h"
#include "config-ini.h"
#include "options.h"

#include "gamma-dummy.h"

#ifdef ENABLE_DRM
# include "gamma-drm.h"
#endif

#ifdef ENABLE_RANDR
# include "gamma-randr.h"
#endif

#ifdef ENABLE_VIDMODE
# include "gamma-vidmode.h"
#endif

#ifdef ENABLE_QUARTZ
# include "gamma-quartz.h"
#endif

#ifdef ENABLE_WINGDI
# include "gamma-w32gdi.h"
#endif

#include "location-manual.h"

#ifdef ENABLE_GEOCLUE2
# include "location-geoclue2.h"
#endif

#ifdef ENABLE_CORELOCATION
# include "location-corelocation.h"
#endif

#define MIN_TEMPERATURE 1000
#define MAX_TEMPERATURE 12000
#define MIN_BRIGHTNESS 0.1
#define MAX_BRIGHTNESS 1.0
#define MIN_GAMMA 0.4
#define MAX_GAMMA 2.5

typedef struct {
	options_t opts;
	const gamma_method_t *gamma_methods;
	const location_provider_t *location_providers;
	gamma_state_t *method_state;

	/* Used for reverting color settings. */
	color_setting_t color_settings_revert[2];
	/* 0 = Daytime tab is active, 1 = Night tab is active. */
	guint active_page;
	/* Used for blocking handlers. */
	gulong handler_ids[10];

	/* Adjustments and widgets. */
	GtkAdjustment *adj_temperature[2];
	GtkAdjustment *adj_gamma[2][3];
	GtkAdjustment *adj_brightness[2];
	GtkAdjustment *adj_dawn_start_h;
	GtkAdjustment *adj_dawn_start_m;
	GtkAdjustment *adj_dawn_end_h;
	GtkAdjustment *adj_dawn_end_m;
	GtkAdjustment *adj_dusk_start_h;
	GtkAdjustment *adj_dusk_start_m;
	GtkAdjustment *adj_dusk_end_h;
	GtkAdjustment *adj_dusk_end_m;
	GtkAdjustment *adj_elevation_high;
	GtkAdjustment *adj_elevation_low;
	GtkAdjustment *adj_latitude;
	GtkAdjustment *adj_longitude;
	GtkAdjustment *adj_card;
	GtkAdjustment *adj_crtc;
	GtkAdjustment *adj_screen;
	GtkWidget *entry_crtcs;
	GtkWidget *combo_method;
	GtkWidget *combo_provider;
} gtk_configurator_t;

static GtkApplication *app;
static gtk_configurator_t cfg;


#if !defined(__WIN32__)
static gboolean
signal_exit()
{
	g_application_quit(G_APPLICATION(app));
	/* The sources will be removed in shutdown_cb(). */
	return G_SOURCE_CONTINUE;
}
#endif

static int
method_try_start(const gamma_method_t *method,
		gamma_state_t **state, config_ini_state_t *config, gchar **args)
{
	int r;

	r = method->init(state);
	if (r < 0) {
		fprintf(stderr, _("Initialization of %s failed.\n"),
			method->name);
		return -1;
	}

	/* Set method options from config file. */
	config_ini_section_t *section =
		config_ini_get_section(config, method->name);
	if (section != NULL) {
		config_ini_setting_t *setting = section->settings;
		while (setting != NULL) {
			r = method->set_option(
				*state, setting->name, setting->value);
			if (r < 0) {
				method->free(*state);
				fprintf(stderr, _("Failed to set %s"
						  " option.\n"),
					method->name);
				return -1;
			}
			setting = setting->next;
		}
	}

	/* Set method options from command line. */
	if (args != NULL) {
		for (int i = 1; args[i] != NULL; i++) {
			if (strcmp(args[i], "") == 0) continue;
			gchar **option = g_strsplit(args[i], "=", -1);
			if (option[1] != NULL) {
				r = method->set_option(*state, option[0], option[1]);
			} else {
				fprintf(stderr, _("Failed to parse option `%s'.\n"),
					args[i]);
				g_strfreev(option);
				return -1;
			}
			g_strfreev(option);
			if (r < 0) {
				method->free(*state);
				fprintf(stderr, _("Failed to set %s option.\n"),
					method->name);
				/* TRANSLATORS: `help' must not be translated. */
				fprintf(stderr, _("Try -m %s:help' for more"
						  " information.\n"), method->name);
				return -1;
			}
		}
	}

	/* Start method. */
	r = method->start(*state);
	if (r < 0) {
		method->free(*state);
		fprintf(stderr, _("Failed to start adjustment method %s.\n"),
			method->name);
		return -1;
	}

	return 0;
}

static gboolean
validate_dawn_dusk_settings()
{
	gint dawn_start_h = gtk_adjustment_get_value(cfg.adj_dawn_start_h);
	gint dawn_start_m = gtk_adjustment_get_value(cfg.adj_dawn_start_m);
	gint dawn_end_h = gtk_adjustment_get_value(cfg.adj_dawn_end_h);
	gint dawn_end_m = gtk_adjustment_get_value(cfg.adj_dawn_end_m);
	gint dusk_start_h = gtk_adjustment_get_value(cfg.adj_dusk_start_h);
	gint dusk_start_m = gtk_adjustment_get_value(cfg.adj_dusk_start_m);
	gint dusk_end_h = gtk_adjustment_get_value(cfg.adj_dusk_end_h);
	gint dusk_end_m = gtk_adjustment_get_value(cfg.adj_dusk_end_m);
	gint dawn_start = dawn_start_h * 3600 + dawn_start_m * 60;
	gint dawn_end = dawn_end_h * 3600 + dawn_end_m * 60;
	gint dusk_start = dusk_start_h * 3600 + dusk_start_m * 60;
	gint dusk_end = dusk_end_h * 3600 + dusk_end_m * 60;

	if (dawn_start > dawn_end) {
		GtkWidget *d = gtk_message_dialog_new(
			gtk_application_get_active_window(app),
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
			_("Start time for dawn (%02d:%02d) must be earlier "
			  "than end time for dawn (%02d:%02d)."),
			dawn_start_h, dawn_start_m, dawn_end_h, dawn_end_m);
		gtk_window_set_title(
			GTK_WINDOW(d), _("Invalid dawn/dusk time configuration"));
		gtk_dialog_run(GTK_DIALOG(d));
		gtk_widget_destroy(d);
		return FALSE;
	} else if (dusk_start > dusk_end) {
		GtkWidget *d = gtk_message_dialog_new(
			gtk_application_get_active_window(app),
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
			_("Start time for dusk (%02d:%02d) must be earlier "
			  "than end time for dusk (%02d:%02d)."),
			dusk_start_h, dusk_start_m, dusk_end_h, dusk_end_m);
		gtk_window_set_title(
			GTK_WINDOW(d), _("Invalid dawn/dusk time configuration"));
		gtk_dialog_run(GTK_DIALOG(d));
		gtk_widget_destroy(d);
		return FALSE;
	} else if (dawn_end > dusk_start) {
		GtkWidget *d = gtk_message_dialog_new(
			gtk_application_get_active_window(app),
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
			_("End time for dawn (%02d:%02d) must be earlier "
			  "than start time for dusk (%02d:%02d)."),
			dawn_end_h, dawn_end_m, dusk_start_h, dusk_start_m);
		gtk_window_set_title(
			GTK_WINDOW(d), _("Invalid dawn/dusk time configuration"));
		gtk_dialog_run(GTK_DIALOG(d));
		gtk_widget_destroy(d);
		return FALSE;
	}

	cfg.opts.scheme.dawn.start = dawn_start;
	cfg.opts.scheme.dawn.end = dawn_end;
	cfg.opts.scheme.dusk.start = dusk_start;
	cfg.opts.scheme.dusk.end = dusk_end;

	return TRUE;
}

static gboolean
validate_crtcs_setting()
{
	gboolean result = TRUE;
	GRegex *regex_valid = g_regex_new(
		"^[0-9]+(,[0-9]+)*$", G_REGEX_DOLLAR_ENDONLY, 0, NULL);
	const gchar *crtcs = gtk_entry_get_text(GTK_ENTRY(cfg.entry_crtcs));

	if (!g_regex_match_all(regex_valid, crtcs, 0, NULL)) {
		GtkWindow *win = gtk_application_get_active_window(app);
		/* Switch to "Advanced" page because it contains the GtkEntry widget. */
		gtk_notebook_set_current_page(
			GTK_NOTEBOOK(gtk_bin_get_child(GTK_BIN(win))), 2);
		GtkWidget *d = gtk_message_dialog_new(win,
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
			_("crtc list (%s) must be comma-separated list of numbers."), crtcs);
		gtk_window_set_title(
			GTK_WINDOW(d), _("Invalid crtc configuration"));
		gtk_dialog_run(GTK_DIALOG(d));
		gtk_widget_destroy(d);
		result = FALSE;
	}

	g_regex_unref (regex_valid);

	return result;
}

static void
disable_revert_menuitem()
{
	g_action_map_remove_action(G_ACTION_MAP(app), "revert");
}

static void
write_config_file(const gchar *outfile)
{
	/* If the directory that contains config file doesn't exist, create it. */
	gchar *configdir = g_path_get_dirname(outfile);
	if (g_mkdir_with_parents(configdir, 0755) == -1) {
		GtkWidget *d = gtk_message_dialog_new(
			gtk_application_get_active_window(app),
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
			_("Failed to create directory \"%s\""), configdir);
		gtk_window_set_title(GTK_WINDOW(d), _("Error"));
		gtk_dialog_run(GTK_DIALOG(d));
		gtk_widget_destroy(d);
		goto cleanup_configdir;
	}

	/* Temporary output */
	GError *err = NULL;
	gchar *tmpfilepath = g_strdup_printf("%s.tmp", outfile);
	GFile *gfile_tmp = g_file_new_for_path(tmpfilepath);
	GFileOutputStream *fos_tmp = g_file_replace(
		gfile_tmp, NULL, TRUE, G_FILE_CREATE_NONE, NULL, &err);
	if (fos_tmp == NULL) {
		GtkWidget *d = gtk_message_dialog_new(
			gtk_application_get_active_window(app),
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
			_("Failed to write to file \"%s\""), tmpfilepath);
		gtk_window_set_title(GTK_WINDOW(d), _("Error"));
		gtk_message_dialog_format_secondary_text(
			GTK_MESSAGE_DIALOG(d), "%s", err->message);
		gtk_dialog_run(GTK_DIALOG(d));
		gtk_widget_destroy(d);
		goto cleanup_tmp;
	}

	/* Settings */
	const gchar *selected_method_id = gtk_combo_box_get_active_id(
		GTK_COMBO_BOX(cfg.combo_method));
	const gchar *selected_provider_id = gtk_combo_box_get_active_id(
		GTK_COMBO_BOX(cfg.combo_provider));
	gchar *temperature_day_config = g_strdup_printf("temp-day=%d\n",
		(gint)gtk_adjustment_get_value(cfg.adj_temperature[0]));
	gchar *temperature_night_config = g_strdup_printf("temp-night=%d\n",
		(gint)gtk_adjustment_get_value(cfg.adj_temperature[1]));
	gchar *brightness_day_config = g_strdup_printf("brightness-day=%.3f\n",
		gtk_adjustment_get_value(cfg.adj_brightness[0]));
	gchar *brightness_night_config = g_strdup_printf("brightness-night=%.3f\n",
		gtk_adjustment_get_value(cfg.adj_brightness[1]));
	gchar *gamma_day_config = g_strdup_printf("gamma-day=%.3f:%.3f:%.3f\n",
		gtk_adjustment_get_value(cfg.adj_gamma[0][0]),
		gtk_adjustment_get_value(cfg.adj_gamma[0][1]),
		gtk_adjustment_get_value(cfg.adj_gamma[0][2]));
	gchar *gamma_night_config = g_strdup_printf("gamma-night=%.3f:%.3f:%.3f\n",
		gtk_adjustment_get_value(cfg.adj_gamma[1][0]),
		gtk_adjustment_get_value(cfg.adj_gamma[1][1]),
		gtk_adjustment_get_value(cfg.adj_gamma[1][2]));
	gchar *fade_config = g_strdup_printf("fade=%d\n", cfg.opts.use_fade);
	gchar *dawn_config = NULL;
	gchar *dusk_config = NULL;
	if (cfg.opts.scheme.use_time) {
		gchar *dawn_start = g_strdup_printf("%02d:%02d",
			cfg.opts.scheme.dawn.start / 3600,
			cfg.opts.scheme.dawn.start % 3600 / 60);
		gchar *dawn_end = NULL;
		if (cfg.opts.scheme.dawn.start != cfg.opts.scheme.dawn.end) {
			dawn_end = g_strdup_printf("-%02d:%02d",
				cfg.opts.scheme.dawn.end / 3600,
				cfg.opts.scheme.dawn.end % 3600 / 60);
		}
		dawn_config = g_strdup_printf("dawn-time=%s%s\n",
			dawn_start, dawn_end ? dawn_end : "");
		g_free(dawn_end);
		g_free(dawn_start);
		gchar *dusk_start = g_strdup_printf("%02d:%02d",
			cfg.opts.scheme.dusk.start / 3600,
			cfg.opts.scheme.dusk.start % 3600 / 60);
		gchar *dusk_end = NULL;
		if (cfg.opts.scheme.dusk.start != cfg.opts.scheme.dusk.end) {
			dusk_end = g_strdup_printf("-%02d:%02d",
				cfg.opts.scheme.dusk.end / 3600,
				cfg.opts.scheme.dusk.end % 3600 / 60);
		}
		dusk_config = g_strdup_printf("dusk-time=%s%s\n",
			dusk_start, dusk_end ? dusk_end : "");
		g_free(dusk_end);
		g_free(dusk_start);
	}
	gchar *elevation_high_config = g_strdup_printf("elevation-high=%.2f\n",
		cfg.opts.scheme.high);
	gchar *elevation_low_config = g_strdup_printf("elevation-low=%.2f\n",
		cfg.opts.scheme.low);
	gchar *method_config = (strcmp(selected_method_id, "auto") == 0) ? NULL
		: g_strdup_printf("adjustment-method=%s\n", selected_method_id);
	gchar *provider_config = NULL;
	gchar *latitude_config = NULL;
	gchar *longitude_config = NULL;
	if (strcmp(selected_provider_id, "auto") != 0) {
		provider_config = g_strdup_printf(
			"location-provider=%s\n", selected_provider_id);
		if (strcmp(selected_provider_id, "manual") == 0) {
			latitude_config = g_strdup_printf(
				"lat=%.2f\n", gtk_adjustment_get_value(cfg.adj_latitude));
			longitude_config = g_strdup_printf(
				"lon=%.2f\n", gtk_adjustment_get_value(cfg.adj_longitude));
		}
	}
	gchar *crtcs_config = g_strdup_printf(
		"crtc=%s\n", gtk_entry_get_text(GTK_ENTRY(cfg.entry_crtcs)));
	gchar *card_config = g_strdup_printf(
		"card=%d\n", (gint)gtk_adjustment_get_value(cfg.adj_card));
	gchar *crtc_config = g_strdup_printf(
		"crtc=%d\n", (gint)gtk_adjustment_get_value(cfg.adj_crtc));
	gchar *screen_config = g_strdup_printf(
		"screen=%d\n", (gint)gtk_adjustment_get_value(cfg.adj_screen));

	/* Build contents. */
	GString *contents = g_string_new("[redshift]\n");
	g_string_append(contents, temperature_day_config);
	g_string_append(contents, temperature_night_config);
	g_string_append(contents, brightness_day_config);
	g_string_append(contents, brightness_night_config);
	g_string_append(contents, gamma_day_config);
	g_string_append(contents, gamma_night_config);
	g_string_append(contents, fade_config);
	if (dawn_config) g_string_append(contents, dawn_config);
	if (dusk_config) g_string_append(contents, dusk_config);
	g_string_append(contents, elevation_high_config);
	g_string_append(contents, elevation_low_config);
	if (method_config) g_string_append(contents, method_config);
	if (provider_config) g_string_append(contents, provider_config);
	g_string_append(contents, "\n[randr]\n");
	g_string_append(contents, crtcs_config);
	g_string_append(contents, screen_config);
	g_string_append(contents, "\n[drm]\n");
	g_string_append(contents, card_config);
	g_string_append(contents, crtc_config);
	g_string_append(contents, "\n[vidmode]\n");
	g_string_append(contents, screen_config);
	if (latitude_config && longitude_config) {
		g_string_append(contents, "\n[manual]\n");
		g_string_append(contents, latitude_config);
		g_string_append(contents, longitude_config);
	}

	/* Write to temporary file. */
	GDataOutputStream *dos_tmp = g_data_output_stream_new(
		G_OUTPUT_STREAM(fos_tmp));
	if (!g_data_output_stream_put_string(dos_tmp, contents->str, NULL, &err)) {
		GtkWidget *d = gtk_message_dialog_new(
			gtk_application_get_active_window(app),
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
			_("Failed to write to file \"%s\""), tmpfilepath);
		gtk_window_set_title(GTK_WINDOW(d), _("Error"));
		gtk_message_dialog_format_secondary_text(
			GTK_MESSAGE_DIALOG(d), "%s", err->message);
		gtk_dialog_run(GTK_DIALOG(d));
		gtk_widget_destroy(d);
		goto cleanup;
	}

	/* Finally rename it. */
	if (g_rename(tmpfilepath, outfile) == -1) {
		GtkWidget *d = gtk_message_dialog_new(
			gtk_application_get_active_window(app),
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
			_("Failed to rename \"%s\" to \"%s\""), tmpfilepath, outfile);
		gtk_window_set_title(GTK_WINDOW(d), _("Error"));
		gtk_dialog_run(GTK_DIALOG(d));
		gtk_widget_destroy(d);
	}

	cfg.color_settings_revert[0] = cfg.opts.scheme.day;
	cfg.color_settings_revert[1] = cfg.opts.scheme.night;
	disable_revert_menuitem();

cleanup:
	g_string_free(contents, TRUE);
	g_object_unref(dos_tmp);
	g_object_unref(fos_tmp);
	g_free(screen_config);
	g_free(crtc_config);
	g_free(card_config);
	g_free(crtcs_config);
	g_free(longitude_config);
	g_free(latitude_config);
	g_free(provider_config);
	g_free(method_config);
	g_free(elevation_low_config);
	g_free(elevation_high_config);
	g_free(dusk_config);
	g_free(dawn_config);
	g_free(fade_config);
	g_free(gamma_night_config);
	g_free(gamma_day_config);
	g_free(brightness_night_config);
	g_free(brightness_day_config);
	g_free(temperature_night_config);
	g_free(temperature_day_config);

cleanup_tmp:
	g_object_unref(gfile_tmp);
	g_free(tmpfilepath);
	g_clear_error(&err);

cleanup_configdir:
	g_free(configdir);
}

static void
save_cb()
{
	/* Write config file only when:
	 *   1. configured to use dawn/dusk settings and they are valid
	 *      or not configured to use dawn/dusk settings
	 *   2. crtc setting for randr is valid (comma-separated list of numbers) */
	if ((!cfg.opts.scheme.use_time ||
	     (cfg.opts.scheme.use_time && validate_dawn_dusk_settings())) &&
	    validate_crtcs_setting()) {
		write_config_file(cfg.opts.config_filepath);
	}
}

static void
revert_cb()
{
	cfg.opts.scheme.day = cfg.color_settings_revert[0];
	cfg.opts.scheme.night = cfg.color_settings_revert[1];

	gtk_adjustment_set_value(
		cfg.adj_temperature[0], cfg.opts.scheme.day.temperature);
	gtk_adjustment_set_value(
		cfg.adj_temperature[1], cfg.opts.scheme.night.temperature);
	gtk_adjustment_set_value(
		cfg.adj_brightness[0], cfg.opts.scheme.day.brightness);
	gtk_adjustment_set_value(
		cfg.adj_brightness[1], cfg.opts.scheme.night.brightness);
	for (int i = 0; i < 3; i++) {
		gtk_adjustment_set_value(
			cfg.adj_gamma[0][i], cfg.opts.scheme.day.gamma[i]);
		gtk_adjustment_set_value(
			cfg.adj_gamma[1][i], cfg.opts.scheme.night.gamma[i]);
	}

	disable_revert_menuitem();
}

static void
quit_cb()
{
	g_application_quit(G_APPLICATION(app));
}

static void
enable_revert_menuitem()
{
	const GActionEntry act_entries[] = {
		{ "revert", revert_cb }
	};
	g_action_map_add_action_entries(
		G_ACTION_MAP(app), act_entries, G_N_ELEMENTS(act_entries), NULL);
}

static void
check_fade_toggled_cb(GtkToggleButton *togglebutton)
{
	cfg.opts.use_fade = !gtk_toggle_button_get_active(togglebutton);
}

static void
check_set_dawn_dusk_toggled_cb(GtkToggleButton *togglebutton)
{
	cfg.opts.scheme.use_time = gtk_toggle_button_get_active(togglebutton);
}

static void
temperature_value_changed_cb(GtkAdjustment *adjustment)
{
	color_setting_t *color_setting =
		cfg.active_page ? &cfg.opts.scheme.night : &cfg.opts.scheme.day;
	color_setting->temperature = gtk_adjustment_get_value(adjustment);
	cfg.opts.method->set_temperature(cfg.method_state, color_setting, 0);
	enable_revert_menuitem();
}

static void
brightness_value_changed_cb(GtkAdjustment *adjustment)
{
	color_setting_t *color_setting =
		cfg.active_page ? &cfg.opts.scheme.night : &cfg.opts.scheme.day;
	color_setting->brightness = gtk_adjustment_get_value(adjustment);
	cfg.opts.method->set_temperature(cfg.method_state, color_setting, 0);
	enable_revert_menuitem();
}

static void
gamma_value_changed_cb(GtkAdjustment *adjustment, gpointer user_data)
{
	gint i = GPOINTER_TO_INT(user_data);
	color_setting_t *color_setting =
		cfg.active_page ? &cfg.opts.scheme.night : &cfg.opts.scheme.day;
	color_setting->gamma[i] = gtk_adjustment_get_value(adjustment);
	cfg.opts.method->set_temperature(cfg.method_state, color_setting, 0);
	enable_revert_menuitem();
}

static void
block_adjustment_handlers(guint page)
{
	g_return_if_fail(page == 0 || page == 1);

	g_signal_handler_block(
		cfg.adj_temperature[page], cfg.handler_ids[page * 5]);
	g_signal_handler_block(
		cfg.adj_brightness[page], cfg.handler_ids[page * 5 + 1]);
	for (int i = 0; i < 3; i++) {
		g_signal_handler_block(
			cfg.adj_gamma[page][i], cfg.handler_ids[page * 5 + 2 + i]);
	}
}

static void
unblock_adjustment_handlers(guint page)
{
	g_return_if_fail(page == 0 || page == 1);

	g_signal_handler_unblock(
		cfg.adj_temperature[page], cfg.handler_ids[page * 5]);
	g_signal_handler_unblock(
		cfg.adj_brightness[page], cfg.handler_ids[page * 5 + 1]);
	for (int i = 0; i < 3; i++) {
		g_signal_handler_unblock(
			cfg.adj_gamma[page][i], cfg.handler_ids[page * 5 + 2 + i]);
	}
}

static void
connect_adjustment_signals()
{
	for (int i = 0; i < 2; i++) {
		cfg.handler_ids[i * 5] = g_signal_connect(cfg.adj_temperature[i],
			"value-changed", G_CALLBACK(temperature_value_changed_cb), NULL);
		cfg.handler_ids[i * 5 + 1] = g_signal_connect(cfg.adj_brightness[i],
			"value-changed", G_CALLBACK(brightness_value_changed_cb), NULL);
		for (int j = 0; j < 3; j++) {
			cfg.handler_ids[i * 5 + 2 + j] = g_signal_connect(
				cfg.adj_gamma[i][j], "value-changed",
				G_CALLBACK(gamma_value_changed_cb), GINT_TO_POINTER(j));
		}
	}
	block_adjustment_handlers(1);  /* Default page is 0 (daytime). */
}

static void
init_adjustments()
{
	cfg.adj_temperature[0] = gtk_adjustment_new(cfg.opts.scheme.day.temperature,
		MIN_TEMPERATURE, MAX_TEMPERATURE, 1, 10, 0);
	cfg.adj_temperature[1] = gtk_adjustment_new(cfg.opts.scheme.night.temperature,
		MIN_TEMPERATURE, MAX_TEMPERATURE, 1, 10, 0);
	cfg.adj_brightness[0] = gtk_adjustment_new(cfg.opts.scheme.day.brightness,
		MIN_BRIGHTNESS, MAX_BRIGHTNESS, 0.001, 0.01, 0);
	cfg.adj_brightness[1] = gtk_adjustment_new(cfg.opts.scheme.night.brightness,
		MIN_BRIGHTNESS, MAX_BRIGHTNESS, 0.001, 0.01, 0);
	for (int i = 0; i < 3; i++) {
		cfg.adj_gamma[0][i] = gtk_adjustment_new(cfg.opts.scheme.day.gamma[i],
			MIN_GAMMA, MAX_GAMMA, 0.001, 0.01, 0);
		cfg.adj_gamma[1][i] = gtk_adjustment_new(cfg.opts.scheme.night.gamma[i],
			MIN_GAMMA, MAX_GAMMA, 0.001, 0.01, 0);
	}
	cfg.adj_latitude = gtk_adjustment_new(0, -90, 90, 0.01, 0.01, 0);
	cfg.adj_longitude = gtk_adjustment_new(0, -180, 180, 0.01, 0.01, 0);
	cfg.adj_dawn_start_h = gtk_adjustment_new(
		cfg.opts.scheme.dawn.start / 3600, 0, 23, 1, 1, 0);
	cfg.adj_dawn_start_m = gtk_adjustment_new(
		cfg.opts.scheme.dawn.start % 3600 / 60, 0, 59, 1, 1, 0);
	cfg.adj_dawn_end_h = gtk_adjustment_new(
		cfg.opts.scheme.dawn.end / 3600, 0, 23, 1, 1, 0);
	cfg.adj_dawn_end_m = gtk_adjustment_new(
		cfg.opts.scheme.dawn.end % 3600 / 60, 0, 59, 1, 1, 0);
	cfg.adj_dusk_start_h = gtk_adjustment_new(
		cfg.opts.scheme.dusk.start / 3600, 0, 23, 1, 1, 0);
	cfg.adj_dusk_start_m = gtk_adjustment_new(
		cfg.opts.scheme.dusk.start % 3600 / 60, 0, 59, 1, 1, 0);
	cfg.adj_dusk_end_h = gtk_adjustment_new(
		cfg.opts.scheme.dusk.end / 3600, 0, 23, 1, 1, 0);
	cfg.adj_dusk_end_m = gtk_adjustment_new(
		cfg.opts.scheme.dusk.end % 3600 / 60, 0, 59, 1, 1, 0);
	cfg.adj_elevation_high = gtk_adjustment_new(
		cfg.opts.scheme.high, -90, 90, 0.01, 0.01, 0);
	cfg.adj_elevation_low = gtk_adjustment_new(
		cfg.opts.scheme.low, -90, 90, 0.01, 0.01, 0);
	cfg.adj_card = gtk_adjustment_new(0, 0, 99, 1, 1, 0);
	cfg.adj_crtc = gtk_adjustment_new(0, 0, 99, 1, 1, 0);
	cfg.adj_screen = gtk_adjustment_new(0, 0, 99, 1, 1, 0);
}

static void
startup_cb()
{
	options_init(&cfg.opts);
}

static gboolean
draw_cb(GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
	color_setting_t *color_setting = user_data;
	int width = gtk_widget_get_allocated_width(widget);
	int height = gtk_widget_get_allocated_height(widget);

	/* Gamma ramp */
	int ramp_size = width + 1;
	float ramp_red[ramp_size];
	float ramp_green[ramp_size];
	float ramp_blue[ramp_size];
	for (int i = 0; i < ramp_size; i++) {
		ramp_red[i] = ramp_green[i] = ramp_blue[i] = (float)i / ramp_size;
	}
	colorramp_fill_float(
		ramp_red, ramp_green, ramp_blue, ramp_size, color_setting);

	/* Background */
	cairo_set_source_rgb(cr, 0.95, 0.95, 0.95);
	cairo_rectangle(cr, 0.0, 0.0, width, height);
	cairo_fill(cr);
	cairo_set_source_rgba(cr, 0.8, 0.8, 0.8, 0.2);
	for (int i = 0; i < 5; i++) {
		double x = (double)i * width / 4;
		double y = (double)i * height / 4;
		cairo_new_path(cr);
		cairo_move_to(cr, x, 0.0);
		cairo_line_to(cr, x, height);
		cairo_stroke(cr);
		cairo_new_path(cr);
		cairo_move_to(cr, 0.0, y);
		cairo_line_to(cr, width, y);
		cairo_stroke(cr);
	}

	/* Red */
	cairo_set_source_rgba(cr, 0.8, 0.0, 0.0, 0.5);
	cairo_new_path(cr);
	for (int i = 0; i < ramp_size; i++) {
		cairo_line_to(cr, i, height - height * ramp_red[i]);
	}
	cairo_stroke(cr);

	/* Green */
	cairo_set_source_rgba(cr, 0.0, 0.8, 0.0, 0.5);
	cairo_new_path(cr);
	for (int i = 0; i < ramp_size; i++) {
		cairo_line_to(cr, i, height - height * ramp_green[i]);
	}
	cairo_stroke(cr);

	/* Blue */
	cairo_set_source_rgba(cr, 0.0, 0.0, 0.8, 0.5);
	cairo_new_path(cr);
	for (int i = 0; i < ramp_size; i++) {
		cairo_line_to(cr, i, height - height * ramp_blue[i]);
	}
	cairo_stroke(cr);

	return FALSE;
}

static void
switch_page_cb(GtkNotebook *notebook, GtkWidget *page, guint page_num)
{
	cfg.active_page = page_num;
	cfg.opts.method->set_temperature(cfg.method_state,
		page_num ? &cfg.opts.scheme.night : &cfg.opts.scheme.day, 0);
	block_adjustment_handlers(!page_num);
	unblock_adjustment_handlers(page_num);
}

static void
create_window()
{
	/* Daytime / Night tabs (temperature, brightness, and gamma settings) */
	const gchar *period_names[] = {
		N_("Da_ytime"),
		N_("_Night")
	};
	GtkWidget *notebook_general = gtk_notebook_new();
	GtkWidget *label_temperature[2];
	GtkWidget *label_brightness[2];
	GtkWidget *label_gamma[2][3];
	GtkWidget *scale_temperature[2];
	GtkWidget *scale_brightness[2];
	GtkWidget *scale_gamma[2][3];
	GtkWidget *spin_temperature[2];
	GtkWidget *spin_brightness[2];
	GtkWidget *spin_gamma[2][3];
	GtkWidget *drawingarea_gammaramp[2];
	GtkWidget *grid_notebook[2];
	for (int i = 0; i < 2; i++) {
		drawingarea_gammaramp[i] = gtk_drawing_area_new();
		GValue margin = G_VALUE_INIT;
		g_value_init(&margin, G_TYPE_INT);
		g_value_set_int(&margin, 4);
		g_object_set_property(
			G_OBJECT(drawingarea_gammaramp[i]), "margin", &margin);
		label_temperature[i] = gtk_label_new_with_mnemonic(_("_Temperature:"));
		label_brightness[i] = gtk_label_new_with_mnemonic(_("Br_ightness:"));
		label_gamma[i][0] = gtk_label_new_with_mnemonic(_("_Red gamma:"));
		label_gamma[i][1] = gtk_label_new_with_mnemonic(_("_Green gamma:"));
		label_gamma[i][2] = gtk_label_new_with_mnemonic(_("_Blue gamma:"));
		spin_brightness[i] = gtk_spin_button_new(cfg.adj_brightness[i], 0, 3);
		scale_brightness[i] = gtk_scale_new(
			GTK_ORIENTATION_HORIZONTAL, cfg.adj_brightness[i]);
		spin_temperature[i] = gtk_spin_button_new(cfg.adj_temperature[i], 0, 0);
		scale_temperature[i] = gtk_scale_new(
			GTK_ORIENTATION_HORIZONTAL, cfg.adj_temperature[i]);
		gtk_scale_set_draw_value(GTK_SCALE(scale_temperature[i]), FALSE);
		gtk_scale_set_draw_value(GTK_SCALE(scale_brightness[i]), FALSE);
		gtk_widget_set_halign(label_temperature[i], GTK_ALIGN_START);
		gtk_widget_set_halign(label_brightness[i], GTK_ALIGN_START);
		gtk_widget_set_vexpand(drawingarea_gammaramp[i], TRUE);
		gtk_widget_set_hexpand(drawingarea_gammaramp[i], TRUE);
		gtk_widget_set_hexpand(scale_temperature[i], TRUE);
		gtk_widget_set_hexpand(scale_brightness[i], TRUE);
		gtk_widget_set_size_request(drawingarea_gammaramp[i], 200, 200);
		gtk_widget_set_size_request(scale_temperature[i], 150, 1);
		gtk_widget_set_size_request(scale_brightness[i], 150, 1);
		gtk_label_set_mnemonic_widget(
			GTK_LABEL(label_temperature[i]), spin_temperature[i]);
		gtk_label_set_mnemonic_widget(
			GTK_LABEL(label_brightness[i]), spin_brightness[i]);
		for (int j = 0; j < 3; j++) {
			spin_gamma[i][j] = gtk_spin_button_new(cfg.adj_gamma[i][j], 0, 3);
			scale_gamma[i][j] = gtk_scale_new(
				GTK_ORIENTATION_HORIZONTAL, cfg.adj_gamma[i][j]);
			gtk_scale_set_draw_value(GTK_SCALE(scale_gamma[i][j]), FALSE);
			gtk_widget_set_halign(label_gamma[i][j], GTK_ALIGN_START);
			gtk_widget_set_hexpand(scale_gamma[i][j], TRUE);
			gtk_widget_set_size_request(scale_gamma[i][j], 150, 1);
			gtk_label_set_mnemonic_widget(
				GTK_LABEL(label_gamma[i][j]), spin_gamma[i][j]);
		}
		grid_notebook[i] = gtk_grid_new();
		gtk_grid_set_column_spacing(GTK_GRID(grid_notebook[i]), 4);
		gtk_grid_attach(GTK_GRID(grid_notebook[i]),
			drawingarea_gammaramp[i], 0, 0, 1, 5);
		gtk_grid_attach(GTK_GRID(grid_notebook[i]),
			label_temperature[i], 1, 0, 1, 1);
		gtk_grid_attach(GTK_GRID(grid_notebook[i]),
			scale_temperature[i], 2, 0, 1, 1);
		gtk_grid_attach(GTK_GRID(grid_notebook[i]),
			spin_temperature[i], 3, 0, 1, 1);
		gtk_grid_attach(GTK_GRID(grid_notebook[i]),
			label_brightness[i], 1, 1, 1, 1);
		gtk_grid_attach(GTK_GRID(grid_notebook[i]),
			scale_brightness[i], 2, 1, 1, 1);
		gtk_grid_attach(GTK_GRID(grid_notebook[i]),
			spin_brightness[i], 3, 1, 1, 1);
		gtk_grid_attach(GTK_GRID(grid_notebook[i]),
			label_gamma[i][0], 1, 2, 1, 1);
		gtk_grid_attach(GTK_GRID(grid_notebook[i]),
			scale_gamma[i][0], 2, 2, 1, 1);
		gtk_grid_attach(GTK_GRID(grid_notebook[i]),
			spin_gamma[i][0], 3, 2, 1, 1);
		gtk_grid_attach(GTK_GRID(grid_notebook[i]),
			label_gamma[i][1], 1, 3, 1, 1);
		gtk_grid_attach(GTK_GRID(grid_notebook[i]),
			scale_gamma[i][1], 2, 3, 1, 1);
		gtk_grid_attach(GTK_GRID(grid_notebook[i]),
			spin_gamma[i][1], 3, 3, 1, 1);
		gtk_grid_attach(GTK_GRID(grid_notebook[i]),
			label_gamma[i][2], 1, 4, 1, 1);
		gtk_grid_attach(GTK_GRID(grid_notebook[i]),
			scale_gamma[i][2], 2, 4, 1, 1);
		gtk_grid_attach(GTK_GRID(grid_notebook[i]),
			spin_gamma[i][2], 3, 4, 1, 1);
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook_general),
			grid_notebook[i], gtk_label_new_with_mnemonic(_(period_names[i])));
	}
	g_signal_connect(drawingarea_gammaramp[0],
		"draw", G_CALLBACK(draw_cb), &cfg.opts.scheme.day);
	g_signal_connect(drawingarea_gammaramp[1],
		"draw", G_CALLBACK(draw_cb), &cfg.opts.scheme.night);

	/* Adjustment method */
	GtkWidget *label_method = gtk_label_new_with_mnemonic(
		_("Adjustment met_hod:"));
	gtk_widget_set_halign(label_method, GTK_ALIGN_START);
	cfg.combo_method = gtk_combo_box_text_new();
	gtk_combo_box_text_append(
		GTK_COMBO_BOX_TEXT(cfg.combo_method), "auto", _("Auto"));
	for (int i = 0; cfg.gamma_methods[i].name != NULL; i++) {
		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(cfg.combo_method),
			cfg.gamma_methods[i].name, cfg.gamma_methods[i].name);
	}
	gtk_label_set_mnemonic_widget(GTK_LABEL(label_method), cfg.combo_method);
	gtk_combo_box_set_active_id(
		GTK_COMBO_BOX(cfg.combo_method), cfg.opts.method->name);

	/* Location provider */
	GtkWidget *label_provider = gtk_label_new_with_mnemonic(
		_("Location _provider:"));
	gtk_widget_set_halign(label_provider, GTK_ALIGN_START);
	cfg.combo_provider = gtk_combo_box_text_new();
	gtk_combo_box_text_append(
		GTK_COMBO_BOX_TEXT(cfg.combo_provider), "auto", _("Auto"));
	for (int i = 0; cfg.location_providers[i].name != NULL; i++) {
		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(cfg.combo_provider),
			cfg.location_providers[i].name, cfg.location_providers[i].name);
	}
	gtk_label_set_mnemonic_widget(GTK_LABEL(label_provider), cfg.combo_provider);
	gtk_combo_box_set_active_id(GTK_COMBO_BOX(cfg.combo_provider),
		cfg.opts.provider ? cfg.opts.provider->name : "auto");

	/* Fading */
	GtkWidget *check_disable_fading = gtk_check_button_new_with_mnemonic(
		_("_Disable fading"));
	if (cfg.opts.use_fade == 0) {
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(check_disable_fading), TRUE);
	}

	/* Dawn/dusk settings */
	GtkWidget *grid_dawn_dusk = gtk_grid_new();
	GtkWidget *grid_dawn_dusk_spin = gtk_grid_new();
	GtkWidget *frame_dawn_dusk = gtk_frame_new(_("Dawn/dusk settings"));
	GtkWidget *check_set_dawn_dusk = gtk_check_button_new_with_mnemonic(
		_("Set dawn/dusk time (ignore solar elevation settings)"));
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(check_set_dawn_dusk), cfg.opts.scheme.use_time);
	GtkWidget *label_dawn = gtk_label_new_with_mnemonic(_("Da_wn:"));
	GtkWidget *label_dusk = gtk_label_new_with_mnemonic(_("Dus_k:"));
	GtkWidget *spin_dawn_start_h = gtk_spin_button_new(
		cfg.adj_dawn_start_h, 0, 0);
	GtkWidget *spin_dawn_start_m = gtk_spin_button_new(
		cfg.adj_dawn_start_m, 0, 0);
	GtkWidget *spin_dawn_end_h = gtk_spin_button_new(cfg.adj_dawn_end_h, 0, 0);
	GtkWidget *spin_dawn_end_m = gtk_spin_button_new(cfg.adj_dawn_end_m, 0, 0);
	GtkWidget *spin_dusk_start_h = gtk_spin_button_new(
		cfg.adj_dusk_start_h, 0, 0);
	GtkWidget *spin_dusk_start_m = gtk_spin_button_new(
		cfg.adj_dusk_start_m, 0, 0);
	GtkWidget *spin_dusk_end_h = gtk_spin_button_new(cfg.adj_dusk_end_h, 0, 0);
	GtkWidget *spin_dusk_end_m = gtk_spin_button_new(cfg.adj_dusk_end_m, 0, 0);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label_dawn), spin_dawn_start_h);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label_dusk), spin_dusk_start_h);
	gtk_grid_set_column_spacing(GTK_GRID(grid_dawn_dusk_spin), 4);
	gtk_widget_set_halign(label_dawn, GTK_ALIGN_END);
	gtk_widget_set_halign(label_dusk, GTK_ALIGN_END);
	gtk_widget_set_halign(grid_dawn_dusk_spin, GTK_ALIGN_END);
	gtk_grid_attach(GTK_GRID(grid_dawn_dusk_spin), label_dawn, 0, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_dawn_dusk_spin), spin_dawn_start_h, 1, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_dawn_dusk_spin), gtk_label_new(":"), 2, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_dawn_dusk_spin), spin_dawn_start_m, 3, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_dawn_dusk_spin), gtk_label_new("-"), 4, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_dawn_dusk_spin), spin_dawn_end_h, 5, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_dawn_dusk_spin), gtk_label_new(":"), 6, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_dawn_dusk_spin), spin_dawn_end_m, 7, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_dawn_dusk_spin), label_dusk, 0, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_dawn_dusk_spin), spin_dusk_start_h, 1, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_dawn_dusk_spin), gtk_label_new(":"), 2, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_dawn_dusk_spin), spin_dusk_start_m, 3, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_dawn_dusk_spin), gtk_label_new("-"), 4, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_dawn_dusk_spin), spin_dusk_end_h, 5, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_dawn_dusk_spin), gtk_label_new(":"), 6, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_dawn_dusk_spin), spin_dusk_end_m, 7, 2, 1, 1);
	gtk_widget_set_hexpand(grid_dawn_dusk_spin, TRUE);
	gtk_grid_set_column_spacing(GTK_GRID(grid_dawn_dusk), 4);
	gtk_grid_attach(GTK_GRID(grid_dawn_dusk), check_set_dawn_dusk, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_dawn_dusk), grid_dawn_dusk_spin, 0, 1, 1, 1);
	gtk_container_add(GTK_CONTAINER(frame_dawn_dusk), grid_dawn_dusk);
	g_signal_connect(check_set_dawn_dusk,
		"toggled", G_CALLBACK(check_set_dawn_dusk_toggled_cb), NULL);

	/* Solar elevation */
	GtkWidget *grid_elevation = gtk_grid_new();
	GtkWidget *frame_elevation = gtk_frame_new(_("Solar elevation"));
	GtkWidget *label_high = gtk_label_new_with_mnemonic(_("_High:"));
	GtkWidget *label_low = gtk_label_new_with_mnemonic(_("_Low:"));
	GtkWidget *spin_elevation_high = gtk_spin_button_new(
		cfg.adj_elevation_high, 0, 2);
	GtkWidget *spin_elevation_low = gtk_spin_button_new(
		cfg.adj_elevation_low, 0, 2);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label_high), spin_elevation_high);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label_low), spin_elevation_low);
	gtk_widget_set_hexpand(grid_elevation, TRUE);
	gtk_grid_set_column_spacing(GTK_GRID(grid_elevation), 4);
	gtk_grid_attach(GTK_GRID(grid_elevation), label_high, 0, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_elevation), spin_elevation_high, 1, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_elevation), label_low, 2, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_elevation), spin_elevation_low, 3, 1, 1, 1);
	gtk_widget_set_halign(grid_elevation, GTK_ALIGN_END);
	gtk_container_add(GTK_CONTAINER(frame_elevation), grid_elevation);

	/* Location for "manual" location provider */
	GtkWidget *grid_location = gtk_grid_new();
	GtkWidget *frame_location = gtk_frame_new(
		_("Location for \"manual\" location provider"));
	GtkWidget *label_latitude = gtk_label_new_with_mnemonic(_("Latit_ude:"));
	GtkWidget *label_longitude = gtk_label_new_with_mnemonic(_("L_ongitude:"));
	GtkWidget *spin_latitude = gtk_spin_button_new(cfg.adj_latitude, 0, 2);
	GtkWidget *spin_longitude = gtk_spin_button_new(cfg.adj_longitude, 0, 2);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label_latitude), spin_latitude);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label_longitude), spin_longitude);
	gtk_widget_set_hexpand(grid_location, TRUE);
	gtk_grid_set_column_spacing(GTK_GRID(grid_location), 4);
	gtk_grid_attach(GTK_GRID(grid_location), label_latitude, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_location), spin_latitude, 1, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_location), label_longitude, 2, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_location), spin_longitude, 3, 0, 1, 1);
	gtk_widget_set_halign(grid_location, GTK_ALIGN_END);
	gtk_container_add(GTK_CONTAINER(frame_location), grid_location);

	/* Adjustment method specific options */
	GtkWidget *grid_method_options = gtk_grid_new();
	GtkWidget *frame_method_options = gtk_frame_new(
		_("Adjustment method specific options"));
	/* TRANSLATORS: "crtc" is an option name. "randr" is a method name. */
	GtkWidget *label_crtcs = gtk_label_new_with_mnemonic(
		_("crtc (comma-se_parated list) [randr]:"));
	/* TRANSLATORS: "card" is an option name. "drm" is a method name. */
	GtkWidget *label_card = gtk_label_new_with_mnemonic(_("car_d [drm]:"));
	/* TRANSLATORS: "crtc" is an option name. "drm" is a method name. */
	GtkWidget *label_crtc = gtk_label_new_with_mnemonic(_("_crtc [drm]:"));
	/* TRANSLATORS: "screen" is an option name.
	   "randr" and "vidmode" are method names. */
	GtkWidget *label_screen = gtk_label_new_with_mnemonic(
		_("_screen [randr, vidmode]:"));
	GtkWidget *spin_crtc = gtk_spin_button_new(cfg.adj_crtc, 0, 0);
	GtkWidget *spin_card = gtk_spin_button_new(cfg.adj_card, 0, 0);
	GtkWidget *spin_screen = gtk_spin_button_new(cfg.adj_screen, 0, 0);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label_crtcs), cfg.entry_crtcs);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label_card), spin_card);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label_crtc), spin_crtc);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label_screen), spin_screen);
	gtk_widget_set_hexpand(grid_method_options, TRUE);
	gtk_grid_set_column_spacing(GTK_GRID(grid_method_options), 4);
	gtk_widget_set_halign(label_crtcs, GTK_ALIGN_END);
	gtk_widget_set_halign(label_card, GTK_ALIGN_END);
	gtk_widget_set_halign(label_crtc, GTK_ALIGN_END);
	gtk_widget_set_halign(label_screen, GTK_ALIGN_END);
	gtk_grid_attach(GTK_GRID(grid_method_options), label_crtcs, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_method_options), cfg.entry_crtcs, 1, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_method_options), label_card, 0, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_method_options), spin_card, 1, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_method_options), label_crtc, 0, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_method_options), spin_crtc, 1, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_method_options), label_screen, 0, 3, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_method_options), spin_screen, 1, 3, 1, 1);
	gtk_widget_set_halign(grid_method_options, GTK_ALIGN_END);
	gtk_container_add(GTK_CONTAINER(frame_method_options), grid_method_options);

	/* Window */
	GtkWidget *notebook_pages = gtk_notebook_new();
	GtkWidget *grid_general = gtk_grid_new();
	GtkWidget *grid_time = gtk_grid_new();
	GtkWidget *grid_advanced = gtk_grid_new();
	GtkWidget *appwin = gtk_application_window_new(GTK_APPLICATION(app));
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook_pages), GTK_POS_LEFT);
	gtk_grid_attach(GTK_GRID(grid_general), notebook_general, 0, 0, 2, 1);
	gtk_grid_attach(GTK_GRID(grid_general), label_method, 0, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_general), cfg.combo_method, 1, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_general), label_provider, 0, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_general), cfg.combo_provider, 1, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_general), check_disable_fading, 0, 3, 2, 1);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook_pages),
		grid_general, gtk_label_new_with_mnemonic(_("G_eneral")));
	gtk_grid_attach(GTK_GRID(grid_time), frame_dawn_dusk, 0, 4, 2, 1);
	gtk_grid_attach(GTK_GRID(grid_time), frame_elevation, 0, 5, 2, 1);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook_pages),
		grid_time, gtk_label_new_with_mnemonic(_("Ti_me")));
	gtk_grid_attach(GTK_GRID(grid_advanced), frame_method_options, 0, 6, 2, 1);
	gtk_grid_attach(GTK_GRID(grid_advanced), frame_location, 0, 7, 2, 1);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook_pages),
		grid_advanced, gtk_label_new_with_mnemonic(_("Ad_vanced")));
#if GTK_CHECK_VERSION(3,12,0)
	gtk_widget_set_margin_start(grid_general, 4);
	gtk_widget_set_margin_end(grid_general, 4);
	gtk_widget_set_margin_start(grid_time, 4);
	gtk_widget_set_margin_end(grid_time, 4);
	gtk_widget_set_margin_start(grid_advanced, 4);
	gtk_widget_set_margin_end(grid_advanced, 4);
#else
	gtk_widget_set_margin_left(grid_general, 4);
	gtk_widget_set_margin_right(grid_general, 4);
	gtk_widget_set_margin_left(grid_time, 4);
	gtk_widget_set_margin_right(grid_time, 4);
	gtk_widget_set_margin_left(grid_advanced, 4);
	gtk_widget_set_margin_right(grid_advanced, 4);
#endif
	gchar *title = g_strdup_printf(
		"%s - %s", cfg.opts.config_filepath, _("Configure Redshift"));
	gtk_window_set_title(GTK_WINDOW(appwin), title);
	g_free(title);
	gtk_window_set_icon_name(GTK_WINDOW(appwin), "redshift");
	gtk_container_add(GTK_CONTAINER(appwin), notebook_pages);
	gtk_widget_show_all(notebook_pages);
	g_signal_connect(check_disable_fading,
		"toggled", G_CALLBACK(check_fade_toggled_cb), NULL);
	g_signal_connect(
		notebook_general, "switch-page", G_CALLBACK(switch_page_cb), NULL);
	gtk_widget_show(appwin);
}

static void
activate_cb()
{
}

static void
shutdown_cb(GApplication *application, gpointer user_data)
{
	if (cfg.opts.method != NULL) {
		cfg.opts.method->restore(cfg.method_state);
		cfg.opts.method->free(cfg.method_state);
	}
	g_free(cfg.opts.config_filepath);
#if !defined(__WIN32__)
	guint *source_ids = user_data;
	g_source_remove(source_ids[0]);
	g_source_remove(source_ids[1]);
#endif
}

static gint
command_line_cb(GtkApplication *application, GApplicationCommandLine *cmdline)
{
	/* Quit if the instance is not primary. */
	if (g_application_command_line_get_is_remote(cmdline)) {
		GtkWindow *win = gtk_application_get_active_window(application);
		GtkWidget *d = gtk_message_dialog_new(win,
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
			_("This program is already running."));
		gtk_window_set_title(GTK_WINDOW(d), _("Error"));
		gtk_dialog_run(GTK_DIALOG(d));
		gtk_widget_destroy(d);

		return EXIT_FAILURE;
	}

	/* "--configfile" option */
	GVariantDict *options = g_application_command_line_get_options_dict(cmdline);
	GVariant *gv_configfile = g_variant_dict_lookup_value(
		options, "configfile", G_VARIANT_TYPE_STRING);
	if (gv_configfile) {
		cfg.opts.config_filepath = g_strdup(
			g_variant_get_string(gv_configfile, NULL));
	}

	/* Load settings from config file. */
	config_ini_state_t config_state;
	if (config_ini_init(&config_state, cfg.opts.config_filepath) < 0) {
		fputs(_("Unable to load config file.\n"), stderr);
		return EXIT_FAILURE;
	}
	if (cfg.opts.config_filepath == NULL) {
		/* Set default config file path. */
		cfg.opts.config_filepath = g_build_filename(
			g_get_user_config_dir(), "redshift", "redshift.conf", NULL);
	}

	gint ret = EXIT_FAILURE;
	options_parse_config_file(
		&cfg.opts, &config_state, cfg.gamma_methods, cfg.location_providers);
	options_set_defaults(&cfg.opts);
	if (cfg.opts.scheme.dawn.start >= 0 || cfg.opts.scheme.dawn.end >= 0 ||
	    cfg.opts.scheme.dusk.start >= 0 || cfg.opts.scheme.dusk.end >= 0) {
		if (cfg.opts.scheme.dawn.start < 0 ||
		    cfg.opts.scheme.dawn.end < 0 ||
		    cfg.opts.scheme.dusk.start < 0 ||
		    cfg.opts.scheme.dusk.end < 0) {
			fputs(_("Partitial time-configuration not"
				" supported!\n"), stderr);
			goto cleanup_ini;
		}

		if (cfg.opts.scheme.dawn.start > cfg.opts.scheme.dawn.end ||
		    cfg.opts.scheme.dawn.end > cfg.opts.scheme.dusk.start ||
		    cfg.opts.scheme.dusk.start > cfg.opts.scheme.dusk.end) {
			fputs(_("Invalid dawn/dusk time configuration!\n"),
			      stderr);
			goto cleanup_ini;
		}

		cfg.opts.scheme.use_time = 1;
	}

	/* "--method" option */
	const gamma_method_t *specified_method = NULL;
	gchar **method_args = NULL;
	GVariant *gv_method = g_variant_dict_lookup_value(
		options, "method", G_VARIANT_TYPE_STRING);
	if (gv_method) {
		method_args = g_strsplit(
			g_variant_get_string(gv_method, NULL), ":", -1);
		for (int i = 0; cfg.gamma_methods[i].name != NULL; i++) {
			const gamma_method_t *m = &cfg.gamma_methods[i];
			if (strcmp(method_args[0], cfg.gamma_methods[i].name) == 0) {
				specified_method = m;
				break;
			}
		}
		if (specified_method != NULL) {
			cfg.opts.method = specified_method;
		} else {
			fprintf(stderr, _("Unknown adjustment method `%s'.\n"),
				method_args[0]);
			goto cleanup;
		}
	}

	int r;
	if (cfg.opts.method != NULL) {
		/* Use method specified on command line or config file. */
		r = method_try_start(
			cfg.opts.method, &cfg.method_state, &config_state, method_args);
		if (r < 0) goto cleanup;
	} else {
		/* Try all methods, use the first that works. */
		for (int i = 0; cfg.gamma_methods[i].name != NULL; i++) {
			const gamma_method_t *m = &cfg.gamma_methods[i];
			if (!m->autostart) continue;

			r = method_try_start(
				m, &cfg.method_state, &config_state, NULL);
			if (r < 0) {
				fputs(_("Trying next method...\n"), stderr);
				continue;
			}

			/* Found method that works. */
			cfg.opts.method = m;
			break;
		}

		/* Failure if no methods were successful at this point. */
		if (cfg.opts.method == NULL) {
			fputs(_("No more methods to try.\n"), stderr);
			goto cleanup;
		}
	}

	init_adjustments();

	/* Try to load latitude and longitude. */
	config_ini_section_t *sect =
		config_ini_get_section(&config_state, "manual");
	if (sect != NULL) {
		config_ini_setting_t *setting = sect->settings;
		while (setting != NULL) {
			if (strcasecmp(setting->name, "lat") == 0) {
				gtk_adjustment_set_value(cfg.adj_latitude, atof(setting->value));
			}
			else if (strcasecmp(setting->name, "lon") == 0) {
				gtk_adjustment_set_value(cfg.adj_longitude, atof(setting->value));
			}
			setting = setting->next;
		}
	}

	/* Try to load method-specific settings. */
	sect = config_ini_get_section(&config_state, "vidmode");
	if (sect != NULL) {
		config_ini_setting_t *setting = sect->settings;
		while (setting != NULL) {
			if (strcasecmp(setting->name, "screen") == 0) {
				gtk_adjustment_set_value(cfg.adj_screen, atof(setting->value));
			}
			setting = setting->next;
		}
	}
	sect = config_ini_get_section(&config_state, "drm");
	if (sect != NULL) {
		config_ini_setting_t *setting = sect->settings;
		while (setting != NULL) {
			if (strcasecmp(setting->name, "card") == 0) {
				gtk_adjustment_set_value(cfg.adj_card, atof(setting->value));
			}
			else if (strcasecmp(setting->name, "crtc") == 0) {
				gtk_adjustment_set_value(cfg.adj_crtc, atof(setting->value));
			}
			setting = setting->next;
		}
	}
	cfg.entry_crtcs = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(cfg.entry_crtcs), "0");
	sect = config_ini_get_section(&config_state, "randr");
	if (sect != NULL) {
		config_ini_setting_t *setting = sect->settings;
		while (setting != NULL) {
			if (strcasecmp(setting->name, "crtc") == 0) {
				gtk_entry_set_text(
					GTK_ENTRY(cfg.entry_crtcs), setting->value);
			}
			else if (strcasecmp(setting->name, "screen") == 0) {
				gtk_adjustment_set_value(cfg.adj_screen, atof(setting->value));
			}
			setting = setting->next;
		}
	}

	/* Adjust temperature */
	r = cfg.opts.method->set_temperature(
		cfg.method_state, &cfg.opts.scheme.day, 1);
	if (r < 0) {
		fputs(_("Temperature adjustment failed.\n"), stderr);
		cfg.opts.method->free(cfg.method_state);
		goto cleanup;
	}

	connect_adjustment_signals();
	create_window();
	cfg.color_settings_revert[0] = cfg.opts.scheme.day;
	cfg.color_settings_revert[1] = cfg.opts.scheme.night;

	ret = EXIT_SUCCESS;

cleanup:
	g_strfreev(method_args);
	/* Prevent calling restore() and free() in shutdown_cb() on error. */
	if (ret == EXIT_FAILURE) cfg.opts.method = NULL;

cleanup_ini:
	config_ini_free(&config_state);

	return ret;
}

int
main(int argc, char *argv[])
{
	/* List of gamma methods. */
	const gamma_method_t gamma_methods[] = {
#ifdef ENABLE_DRM
		drm_gamma_method,
#endif
#ifdef ENABLE_RANDR
		randr_gamma_method,
#endif
#ifdef ENABLE_VIDMODE
		vidmode_gamma_method,
#endif
#ifdef ENABLE_QUARTZ
		quartz_gamma_method,
#endif
#ifdef ENABLE_WINGDI
		w32gdi_gamma_method,
#endif
		dummy_gamma_method,
		{ NULL }
	};
	cfg.gamma_methods = gamma_methods;

	/* List of location providers. */
	const location_provider_t location_providers[] = {
#ifdef ENABLE_GEOCLUE2
		geoclue2_location_provider,
#endif
#ifdef ENABLE_CORELOCATION
		corelocation_location_provider,
#endif
		manual_location_provider,
		{ NULL }
	};
	cfg.location_providers = location_providers;

#ifdef ENABLE_NLS
	/* Init locale */
	setlocale(LC_CTYPE, "");
	setlocale(LC_MESSAGES, "");

	/* Internationalisation */
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
#endif

	/* Application */
	app = gtk_application_new(
		"dk.jonls.redshift-config-gtk", G_APPLICATION_HANDLES_COMMAND_LINE);
	g_signal_connect(app, "startup", G_CALLBACK(startup_cb), NULL);
	g_signal_connect(app, "activate", G_CALLBACK(activate_cb), NULL);
	g_signal_connect(
		app, "command-line", G_CALLBACK(command_line_cb), NULL);
#if !defined(__WIN32__)
	guint source_ids[2];
	source_ids[0] = g_unix_signal_add(SIGINT, signal_exit, NULL);
	source_ids[1] = g_unix_signal_add(SIGTERM, signal_exit, NULL);
	g_signal_connect(app, "shutdown", G_CALLBACK(shutdown_cb), source_ids);
#else
	g_signal_connect(app, "shutdown", G_CALLBACK(shutdown_cb), NULL);
#endif

	/* Actions */
	const GActionEntry act_entries[] = {
		{ "save", save_cb },
		{ "quit", quit_cb }
	};
	g_action_map_add_action_entries(
		G_ACTION_MAP(app), act_entries, G_N_ELEMENTS(act_entries), NULL);

	/* Options */
	GString *method_names = g_string_new("");
	for (int i = 0; gamma_methods[i].name != NULL; i++) {
		g_string_append(method_names, " ");
		g_string_append(method_names, gamma_methods[i].name);
	}
	gchar *help_method = g_strdup_printf(
		"%s\n                             %s                            %s",
		_("Method to use to set color temperature"),
		_("Available adjustment methods:\n"), method_names->str);
	const GOptionEntry opt_entries[] = {
		{ "configfile", 'c', 0, G_OPTION_ARG_STRING, NULL,
			_("Load settings from specified configuration file"), "FILE" },
		{ "method", 'm', 0, G_OPTION_ARG_STRING, NULL,
			help_method, "METHOD" },
		{ NULL }
	};
	g_application_add_main_option_entries(G_APPLICATION(app), opt_entries);

	/* Run */
	int status = g_application_run(G_APPLICATION(app), argc, argv);

	g_object_unref(app);
	g_free(help_method);
	g_string_free(method_names, TRUE);

	return status;
}
