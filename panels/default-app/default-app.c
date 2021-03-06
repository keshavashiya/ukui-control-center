/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2016 Tianjin KYLIN Information Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#include "default-app.h"
#include <gio/gio.h>
#include <string.h>
#include <glib.h>
#include <gio/gdesktopappinfo.h>
#include <stdlib.h>

#define APPLICATIONSDIR       "/usr/share/applications"
#define TERMINAL_SCHEMA       "org.mate.applications-terminal"
#define TERMINAL_KEY          "exec"

#define VISUAL_SCHEMA         "org.mate.applications-at-visual"
#define VISUAL_KEY            "exec"
#define VISUAL_STARTUP_KEY    "startup"

#define MOBILITY_SCHEMA       "org.mate.applications-at-mobility"
#define MOBILITY_KEY          "exec"
#define MOBILITY_STARTUP_KEY  "startup"

typedef struct _MateDACapplet 
{
	//combo box
	GtkWidget* web_combo_box;
	GtkWidget* mail_combo_box;
	GtkWidget* term_combo_box;
	GtkWidget* media_combo_box;
	GtkWidget* video_combo_box;
	GtkWidget* visual_combo_box;
	GtkWidget* mobility_combo_box;
	GtkWidget* file_combo_box;
	GtkWidget* text_combo_box;
	GtkWidget* image_combo_box;

	//Lists of available apps
	GList* web_browsers;
	GList* mail_readers;
	GList* terminals;
	GList* media_players;
	GList* video_players;
	GList* visual_ats;
	GList* mobility_ats;
	GList* file_managers;
	GList* text_editors;
	GList* image_viewers;

	//Settings object
	GSettings * terminal_settings;
	GSettings * visual_settings;
	GSettings * mobility_settings;

	GtkIconTheme * icon_theme;
}MateDACapplet;

MateDACapplet * capplet;

enum {
	DA_TYPE_WEB_BROWSER,
	DA_TYPE_EMAIL,
	DA_TYPE_TERMINAL,
	DA_TYPE_MEDIA,
	DA_TYPE_VIDEO,
	DA_TYPE_VISUAL,
	DA_TYPE_MOBILITY,
	DA_TYPE_IMAGE,
	DA_TYPE_TEXT,
	DA_TYPE_FILE,
	DA_N_COLUMNS
};

/* For combo box */
enum {
	PIXBUF_COL,
	TEXT_COL,
	ID_COL,
	ICONAME_COL,
	N_COLUMNS
};

static void 
set_changed(GtkComboBox* combo, MateDACapplet* capplet, GList* list, gint type)
{
	guint index;
	GAppInfo* item;
	GSettings* settings;

	index = gtk_combo_box_get_active(combo);

	if (index < g_list_length(list))
	{
		item = (GAppInfo*) g_list_nth_data(list, index);

		switch (type)
		{
			case DA_TYPE_WEB_BROWSER:
				g_app_info_set_as_default_for_type(item, "x-scheme-handler/http", NULL);
				g_app_info_set_as_default_for_type(item, "x-scheme-handler/https", NULL);
				/* about:config is used by firefox and others */
				g_app_info_set_as_default_for_type(item, "x-scheme-handler/about", NULL);
				break;

			case DA_TYPE_EMAIL:
				g_app_info_set_as_default_for_type(item, "x-scheme-handler/mailto", NULL);
				g_app_info_set_as_default_for_type(item, "application/x-extension-eml", NULL);
				g_app_info_set_as_default_for_type(item, "message/rfc822", NULL);
				break;

			case DA_TYPE_FILE:
				g_app_info_set_as_default_for_type(item, "inode/directory", NULL);
				break;

			case DA_TYPE_TEXT:
				g_app_info_set_as_default_for_type(item, "text/plain", NULL);
				break;

			case DA_TYPE_MEDIA:
				g_app_info_set_as_default_for_type(item, "audio/mpeg", NULL);
				g_app_info_set_as_default_for_type(item, "audio/x-mpegurl", NULL);
				g_app_info_set_as_default_for_type(item, "audio/x-scpls", NULL);
				g_app_info_set_as_default_for_type(item, "audio/x-vorbis+ogg", NULL);
				g_app_info_set_as_default_for_type(item, "audio/x-wav", NULL);
				break;

			case DA_TYPE_VIDEO:
				g_app_info_set_as_default_for_type(item, "video/mp4", NULL);
				g_app_info_set_as_default_for_type(item, "video/mpeg", NULL);
				g_app_info_set_as_default_for_type(item, "video/mp2t", NULL);
				g_app_info_set_as_default_for_type(item, "video/msvideo", NULL);
				g_app_info_set_as_default_for_type(item, "video/quicktime", NULL);
				g_app_info_set_as_default_for_type(item, "video/webm", NULL);
				g_app_info_set_as_default_for_type(item, "video/x-avi", NULL);
				g_app_info_set_as_default_for_type(item, "video/x-flv", NULL);
				g_app_info_set_as_default_for_type(item, "video/x-matroska", NULL);
				g_app_info_set_as_default_for_type(item, "video/x-mpeg", NULL);
				g_app_info_set_as_default_for_type(item, "video/x-ogm+ogg", NULL);
				break;

			case DA_TYPE_IMAGE:
				g_app_info_set_as_default_for_type(item, "image/bmp", NULL);
				g_app_info_set_as_default_for_type(item, "image/gif", NULL);
				g_app_info_set_as_default_for_type(item, "image/jpeg", NULL);
				g_app_info_set_as_default_for_type(item, "image/png", NULL);
				g_app_info_set_as_default_for_type(item, "image/tiff", NULL);
				break;

			case DA_TYPE_TERMINAL:
				g_settings_set_string (capplet->terminal_settings, TERMINAL_KEY, g_app_info_get_executable (item));
				break;

			case DA_TYPE_VISUAL:
				g_settings_set_string (capplet->visual_settings, VISUAL_KEY, g_app_info_get_executable (item));
				break;
			
			case DA_TYPE_MOBILITY:
				g_settings_set_string (capplet->mobility_settings, MOBILITY_KEY, g_app_info_get_executable (item));
				break;

			default:
				break;
		}
	}
}

/* Combo boxes callbacks */
static void
web_combo_changed_cb(GtkComboBox* combo, MateDACapplet* capplet)
{
	set_changed(combo, capplet, capplet->web_browsers, DA_TYPE_WEB_BROWSER);
}

static void
mail_combo_changed_cb(GtkComboBox* combo, MateDACapplet* capplet)
{
	set_changed(combo, capplet, capplet->mail_readers, DA_TYPE_EMAIL);
}

static void
file_combo_changed_cb(GtkComboBox* combo, MateDACapplet* capplet)
{
	set_changed(combo, capplet, capplet->file_managers, DA_TYPE_FILE);
}

static void
text_combo_changed_cb(GtkComboBox* combo, MateDACapplet* capplet)
{
	set_changed(combo, capplet, capplet->text_editors, DA_TYPE_TEXT);
}

static void
media_combo_changed_cb(GtkComboBox* combo, MateDACapplet* capplet)
{
	set_changed(combo, capplet, capplet->media_players, DA_TYPE_MEDIA);
}

static void
video_combo_changed_cb(GtkComboBox* combo, MateDACapplet* capplet)
{
	set_changed(combo, capplet, capplet->video_players, DA_TYPE_VIDEO);
}

/*static void
terminal_combo_changed_cb(GtkComboBox* combo, MateDACapplet* capplet)
{
	set_changed(combo, capplet, capplet->terminals, DA_TYPE_TERMINAL);
}*/

static void
visual_combo_changed_cb(GtkComboBox* combo, MateDACapplet* capplet)
{
	set_changed(combo, capplet, capplet->visual_ats, DA_TYPE_VISUAL);
}

static void
mobility_combo_changed_cb(GtkComboBox* combo, MateDACapplet* capplet)
{
	set_changed(combo, capplet, capplet->mobility_ats, DA_TYPE_MOBILITY);
}

static void
image_combo_changed_cb(GtkComboBox* combo, MateDACapplet* capplet)
{
	set_changed(combo, capplet, capplet->image_viewers, DA_TYPE_IMAGE);
}
static void
refresh_combo_box_icons(GtkIconTheme* theme, GtkComboBox* combo_box, GList* app_list)
{
	GtkTreeIter iter;
	GtkTreeModel* model;
	gboolean valid;
	GdkPixbuf* pixbuf;
	gchar* icon_name;
	model = gtk_combo_box_get_model(combo_box);

	if (model == NULL)
	{
		//g_warning("refresh_combobox, but model=NULL");
		return;
	}
	valid = gtk_tree_model_get_iter_first(model, &iter);
	while (valid)
	{
		gtk_tree_model_get(model, &iter,
				   ICONAME_COL, &icon_name,
				   -1);

		pixbuf = gtk_icon_theme_load_icon(theme, icon_name, 22, 0, NULL);
		if (pixbuf == NULL)
		{
			g_warning("pixbuf ==NULL");
		}
		else
		{
			gtk_list_store_set(GTK_LIST_STORE(model), &iter,
					   PIXBUF_COL, pixbuf,
					   -1);
		}
		if (pixbuf)
		{
			g_object_unref(pixbuf);
		}

		g_free(icon_name);

		valid = gtk_tree_model_iter_next(model, &iter);
	}
}

static struct {
	const char* name;
	const char* icon;
} icons[] = {
	{"web_browser_image",  "web-browser"},
	{"mail_reader_image",  "emblem-mail"},
	{"media_player_image", "audio-x-generic"},
	{"visual_image",       "zoom-best-fit"},
	{"mobility_image",     "preferences-desktop-accessibility"},
	{"filemanager_image",  "file-manager"},
	{"imageviewer_image",  "image-x-generic"},
	{"video_image",        "video-x-generic"},
	{"text_image",         "text-editor"},
};

/* Callback for icon theme change */
static void
theme_changed_cb(GtkIconTheme* theme, GtkBuilder * builder)
{
	/*GObject* icon;
	gint i;

	for (i = 0; i < G_N_ELEMENTS(icons); i++)
	{
		icon = gtk_builder_get_object(builder, icons[i].name);
		GdkPixbuf* pixbuf = gtk_icon_theme_load_icon(theme, icons[i].icon, 32, 0, NULL);

		gtk_image_set_from_pixbuf(GTK_IMAGE(icon), pixbuf);
		if (pixbuf)
		{
			g_object_unref(pixbuf);
		}
	}*/

	refresh_combo_box_icons(theme, GTK_COMBO_BOX(capplet->web_combo_box), capplet->web_browsers);
	refresh_combo_box_icons(theme, GTK_COMBO_BOX(capplet->mail_combo_box), capplet->mail_readers);
	refresh_combo_box_icons(theme, GTK_COMBO_BOX(capplet->media_combo_box), capplet->media_players);
	refresh_combo_box_icons(theme, GTK_COMBO_BOX(capplet->video_combo_box), capplet->video_players);
	//refresh_combo_box_icons(theme, GTK_COMBO_BOX(capplet->term_combo_box), capplet->terminals);
	//refresh_combo_box_icons(theme, GTK_COMBO_BOX(capplet->visual_combo_box), capplet->visual_ats);
	//refresh_combo_box_icons(theme, GTK_COMBO_BOX(capplet->mobility_combo_box), capplet->mobility_ats);
	refresh_combo_box_icons(theme, GTK_COMBO_BOX(capplet->file_combo_box), capplet->file_managers);
	refresh_combo_box_icons(theme, GTK_COMBO_BOX(capplet->text_combo_box), capplet->text_editors);
}

static void
screen_changed_cb(GtkWidget* widget, GdkScreen* screen, GtkBuilder * builder)
{
	GtkIconTheme* theme;
	theme = gtk_icon_theme_get_for_screen (screen);

	if (capplet->icon_theme != NULL)
	{
		g_signal_handlers_disconnect_by_func (capplet->icon_theme, theme_changed_cb, builder);
	}

	g_signal_connect (theme, "changed", G_CALLBACK (theme_changed_cb), builder);
	theme_changed_cb (theme, builder);

	capplet->icon_theme = theme;
}

static void
fill_combo_box(GtkIconTheme* theme, GtkComboBox* combo_box, GList* app_list, gchar* mime)
{
	guint index = 0;
	GList* entry;
	GtkTreeModel* model;
	GtkCellRenderer* renderer;
	GtkTreeIter iter;
	GdkPixbuf* pixbuf;
	GAppInfo* default_app;

	default_app = NULL;
	if (g_strcmp0(mime, "terminal") == 0)
	{
		GSettings *terminal_settings = g_settings_new (TERMINAL_SCHEMA);
		gchar *default_terminal = g_settings_get_string (terminal_settings, TERMINAL_KEY);
		for (entry = app_list; entry != NULL; entry = g_list_next(entry))
		{
			GAppInfo* item = (GAppInfo*) entry->data;
			if (g_strcmp0 (g_app_info_get_executable (item), default_terminal) == 0)
			{
				default_app = item;
			}
		}
		g_free (default_terminal);
		g_object_unref (terminal_settings);
	}
	else if (g_strcmp0(mime, "visual") == 0)
	{
		GSettings *visual_settings = g_settings_new (VISUAL_SCHEMA);
		gchar *default_visual = g_settings_get_string (visual_settings, VISUAL_KEY);
		for (entry = app_list; entry != NULL; entry = g_list_next(entry))
		{
			GAppInfo* item = (GAppInfo*) entry->data;
			if (g_strcmp0 (g_app_info_get_executable (item), default_visual) == 0)
			{
				default_app = item;
			}
		}
		g_free (default_visual);
		g_object_unref (visual_settings);
	}
	else if (g_strcmp0(mime, "mobility") == 0)
	{
		GSettings *mobility_settings = g_settings_new (MOBILITY_SCHEMA);
		gchar *default_mobility = g_settings_get_string (mobility_settings, MOBILITY_KEY);
		for (entry = app_list; entry != NULL; entry = g_list_next(entry))
		{
			GAppInfo* item = (GAppInfo*) entry->data;
			if (g_strcmp0 (g_app_info_get_executable (item), default_mobility) == 0)
			{
				default_app = item;
			}
		}
		g_free (default_mobility);
		g_object_unref (mobility_settings);
	}
	else
	{
		default_app = g_app_info_get_default_for_type (mime, FALSE);
	}

	if (theme == NULL)
	{
		theme = gtk_icon_theme_get_default();
	}

	model = GTK_TREE_MODEL(gtk_list_store_new(4, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING));
	gtk_combo_box_set_model(combo_box, model);

	renderer = gtk_cell_renderer_pixbuf_new();

	/* Not all cells have a pixbuf, this prevents the combo box to shrink */
	gtk_cell_renderer_set_fixed_size(renderer, -1, -1);
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo_box), renderer, FALSE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo_box), renderer,
				       "pixbuf", PIXBUF_COL,
				       NULL);

	renderer = gtk_cell_renderer_text_new();

	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo_box), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo_box), renderer,
				       "text", TEXT_COL,
				       NULL);

	for (entry = app_list; entry != NULL; entry = g_list_next(entry))
	{
		GAppInfo* item = (GAppInfo*) entry->data;
		/* Icon */
		GIcon* icon = g_app_info_get_icon(item);
		gchar* icon_name = g_icon_to_string(icon);
		if (icon_name == NULL)
		{
			/* Default icon */
			icon_name = g_strdup("binary");
		}

		pixbuf = gtk_icon_theme_load_icon(theme, icon_name, 25, GTK_ICON_LOOKUP_NO_SVG, NULL);
		gtk_list_store_append(GTK_LIST_STORE(model), &iter);
		//Fixme: when pixbuf == NULL
		if (pixbuf ==NULL)
		{
			//g_warning("pixbuf == NULL");
			gtk_list_store_set(GTK_LIST_STORE(model), &iter,
					   TEXT_COL, g_app_info_get_display_name(item),
					   ID_COL, g_app_info_get_id(item),
					   ICONAME_COL, icon_name,
					   -1);
		}
		else
		{
			gtk_list_store_set(GTK_LIST_STORE(model), &iter,
					   PIXBUF_COL, pixbuf,
					   TEXT_COL, g_app_info_get_display_name(item),
					   ID_COL, g_app_info_get_id(item),
					   ICONAME_COL, icon_name,
					   -1);
		}
		/* Set the index for the default app */
		if (default_app != NULL && g_app_info_equal(item, default_app))
		{
			//g_warning("text_col=%s\tid_col=%s\ticon_name=%s\n",g_app_info_get_display_name(item), g_app_info_get_id(item),icon_name);
			gtk_combo_box_set_active(combo_box, index);
		}
		if (pixbuf)
		{
			g_object_unref(pixbuf);
		}
		g_free(icon_name);

		index++;
	}
}

static GList*
fill_list_from_desktop_file (GList* app_list, const gchar *desktop_id)
{
	GList* list = app_list;
	GDesktopAppInfo* appinfo;

	appinfo = g_desktop_app_info_new_from_filename (desktop_id);
	if (appinfo != NULL)
	{
		list = g_list_prepend (list, appinfo);
	}
	return list;
}

void show_dialog(GtkBuilder * builder)
{
	#define get_widget(name) GTK_WIDGET(gtk_builder_get_object(builder, name))

	capplet->web_combo_box = get_widget("web_browser_combobox");
	capplet->mail_combo_box = get_widget("mail_reader_combobox");
	capplet->term_combo_box = get_widget("terminal_combobox");
	capplet->media_combo_box = get_widget("media_player_combobox");
	capplet->video_combo_box = get_widget("video_combobox");
	//capplet->visual_combo_box = get_widget("visual_combobox");
	//capplet->mobility_combo_box = get_widget("mobility_combobox");
	capplet->text_combo_box = get_widget("text_combobox");
	capplet->file_combo_box = get_widget("filemanager_combobox");
	capplet->image_combo_box = get_widget("image_combobox");

	screen_changed_cb(NULL, gdk_screen_get_default(), builder);

	capplet->web_browsers = g_app_info_get_all_for_type("x-scheme-handler/http");
	capplet->mail_readers = g_app_info_get_all_for_type("x-scheme-handler/mailto");
	capplet->media_players = g_app_info_get_all_for_type("audio/x-vorbis+ogg");
	capplet->video_players = g_app_info_get_all_for_type("video/x-ogm+ogg");
	capplet->text_editors = g_app_info_get_all_for_type("text/plain");
	capplet->image_viewers = g_app_info_get_all_for_type("image/png");
	capplet->file_managers = g_app_info_get_all_for_type("inode/directory");

	capplet->visual_ats = NULL;
	capplet->visual_ats = fill_list_from_desktop_file (capplet->visual_ats, APPLICATIONSDIR "/orca.desktop");
	capplet->visual_ats = g_list_reverse (capplet->visual_ats);

	capplet->mobility_ats = NULL;
	capplet->mobility_ats = fill_list_from_desktop_file (capplet->mobility_ats, APPLICATIONSDIR "/dasher.desktop");
	capplet->mobility_ats = fill_list_from_desktop_file (capplet->mobility_ats, APPLICATIONSDIR "/gok.desktop");
	capplet->mobility_ats = fill_list_from_desktop_file (capplet->mobility_ats, APPLICATIONSDIR "/onboard.desktop");
	capplet->mobility_ats = g_list_reverse (capplet->mobility_ats);

	fill_combo_box(capplet->icon_theme, GTK_COMBO_BOX(capplet->web_combo_box), capplet->web_browsers, "x-scheme-handler/http");
	fill_combo_box(capplet->icon_theme, GTK_COMBO_BOX(capplet->mail_combo_box), capplet->mail_readers, "x-scheme-handler/mailto");
	//fill_combo_box(capplet->icon_theme, GTK_COMBO_BOX(capplet->term_combo_box), capplet->terminals, "terminal");
	fill_combo_box(capplet->icon_theme, GTK_COMBO_BOX(capplet->media_combo_box), capplet->media_players, "audio/x-vorbis+ogg");
	fill_combo_box(capplet->icon_theme, GTK_COMBO_BOX(capplet->video_combo_box), capplet->video_players, "video/x-ogm+ogg");
	fill_combo_box(capplet->icon_theme, GTK_COMBO_BOX(capplet->image_combo_box), capplet->image_viewers, "image/png");
	fill_combo_box(capplet->icon_theme, GTK_COMBO_BOX(capplet->text_combo_box), capplet->text_editors, "text/plain");
	fill_combo_box(capplet->icon_theme, GTK_COMBO_BOX(capplet->file_combo_box), capplet->file_managers, "inode/directory");
	//fill_combo_box(capplet->icon_theme, GTK_COMBO_BOX(capplet->visual_combo_box), capplet->visual_ats, "visual");
	//fill_combo_box(capplet->icon_theme, GTK_COMBO_BOX(capplet->mobility_combo_box), capplet->mobility_ats, "mobility");

	g_signal_connect(capplet->web_combo_box, "changed", G_CALLBACK(web_combo_changed_cb), capplet);
	g_signal_connect(capplet->mail_combo_box, "changed", G_CALLBACK(mail_combo_changed_cb), capplet);
	//g_signal_connect(capplet->term_combo_box, "changed", G_CALLBACK(terminal_combo_changed_cb), capplet);
	g_signal_connect(capplet->media_combo_box, "changed", G_CALLBACK(media_combo_changed_cb), capplet);
	g_signal_connect(capplet->video_combo_box, "changed", G_CALLBACK(video_combo_changed_cb), capplet);
	//g_signal_connect(capplet->visual_combo_box, "changed", G_CALLBACK(visual_combo_changed_cb), capplet);
	//g_signal_connect(capplet->mobility_combo_box, "changed", G_CALLBACK(mobility_combo_changed_cb), capplet);
	g_signal_connect(capplet->image_combo_box, "changed", G_CALLBACK(image_combo_changed_cb), capplet);
	g_signal_connect(capplet->text_combo_box, "changed", G_CALLBACK(text_combo_changed_cb), capplet);
	g_signal_connect(capplet->file_combo_box, "changed", G_CALLBACK(file_combo_changed_cb), capplet);

	#undef get_widget
}

void add_default_app(GtkBuilder * builder)
{
	g_warning("add_default_app");
	capplet = g_new0(MateDACapplet, 1);
	capplet->terminal_settings = g_settings_new (TERMINAL_SCHEMA);
	capplet->mobility_settings = g_settings_new (MOBILITY_SCHEMA);
	capplet->visual_settings = g_settings_new (VISUAL_SCHEMA);

	show_dialog(builder);
}

void default_app_destory()
{
	g_list_free(capplet->web_browsers);
	g_list_free(capplet->mail_readers);
	g_list_free(capplet->media_players);
	g_list_free(capplet->video_players);
	g_list_free(capplet->text_editors);
	g_list_free(capplet->image_viewers);
	g_list_free(capplet->file_managers);
	g_object_unref (capplet->terminal_settings);
	g_object_unref (capplet->mobility_settings);
	g_object_unref (capplet->visual_settings);
}
