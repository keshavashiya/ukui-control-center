/*
 *  Authors: Rodney Dawes <dobey@ximian.com>
 *  Modified by: zhangshuhao <zhangshuhao@kylinos.cn>
 *  Copyright 2003-2006 Novell, Inc. (www.novell.com)
 *  Copyright (C) 2016,Tianjin KYLIN Information Technology Co., Ltd.
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of version 2 of the GNU General Public License
 *  as published by the Free Software Foundation
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02110-1301, USA.
 *
 */

#include "appearance.h"
#include "ukui-wp-item.h"
#include <gio/gio.h>
#include <string.h>
#include <libxml/parser.h>

static gboolean ukui_wp_xml_get_bool(const xmlNode* parent, const char* prop_name)
{
	gboolean ret_val = FALSE;

	if (parent != NULL && prop_name != NULL)
	{
		xmlChar* prop = xmlGetProp((xmlNode*) parent, (xmlChar*) prop_name);

		if (prop != NULL)
		{
			if (!g_ascii_strcasecmp((char*) prop, "true") || !g_ascii_strcasecmp((char*) prop, "1"))
			{
				ret_val = TRUE;
			}
			else
			{
				ret_val = FALSE;
			}

			g_free(prop);
		}
	}

	return ret_val;
}

static void ukui_wp_xml_set_bool(const xmlNode* parent, const xmlChar* prop_name, gboolean value)
{
	if (parent != NULL && prop_name != NULL)
	{
		if (value)
		{
			xmlSetProp((xmlNode*) parent, prop_name, (xmlChar*) "true");
		}
		else
		{
			xmlSetProp((xmlNode*) parent, prop_name, (xmlChar*) "false");
		}
	}
}

static void ukui_wp_load_legacy(AppearanceData* data)
{
	/* Legacy of GNOME2
	 * ~/.gnome2/wallpapers.list */
	char* filename = g_build_filename(g_get_home_dir(), ".gnome2", "wallpapers.list", NULL);

	if (g_file_test(filename, G_FILE_TEST_EXISTS))
	{
		FILE* fp;

		if ((fp = fopen(filename, "r")) != NULL)
		{
			char* foo = (char*) g_malloc(sizeof(char) * 4096);

			while (fgets(foo, 4096, fp))
			{
				UkuiWPItem * item;

				if (foo[strlen(foo) - 1] == '\n')
				{
					foo[strlen(foo) - 1] = '\0';
				}

				item = g_hash_table_lookup(data->wp_hash, foo);

				if (item != NULL)
				{
					continue;
				}

				if (!g_file_test(foo, G_FILE_TEST_EXISTS))
				{
					continue;
				}

				item = ukui_wp_item_new(foo, data->wp_hash, data->thumb_factory);

				if (item != NULL && item->fileinfo == NULL)
				{
					ukui_wp_item_free(item);
				}
			}

			fclose(fp);
			g_free(foo);
		}
	}

	g_free(filename);
}

static void ukui_wp_xml_load_xml(AppearanceData* data, const char* filename)
{
	xmlDoc* wplist;
	xmlNode* root;
	xmlNode* list;
	xmlNode* wpa;
	xmlChar* nodelang;
	const char* const* syslangs;
#if GTK_CHECK_VERSION (3, 0, 0)
	GdkRGBA color1;
	GdkRGBA color2;
#else
	GdkColor color1;
	GdkColor color2;
#endif
	gint i;

	wplist = xmlParseFile(filename);

	if (!wplist)
	{
		return;
	}

	syslangs = g_get_language_names();

	root = xmlDocGetRootElement(wplist);

	for (list = root->children; list != NULL; list = list->next)
	{
		if (!strcmp((char*) list->name, "wallpaper"))
		{
			UkuiWPItem * wp;
			char *pcolor = NULL, *scolor = NULL;
			gboolean have_scale = FALSE, have_shade = FALSE, have_artist = FALSE;

			wp = g_new0(UkuiWPItem, 1);

			wp->deleted = ukui_wp_xml_get_bool(list, "deleted");

			for (wpa = list->children; wpa != NULL; wpa = wpa->next)
			{
				if (wpa->type == XML_COMMENT_NODE)
				{
					continue;
				}
				else if (!strcmp ((char*) wpa->name, "filename"))
				{
					if (wpa->last != NULL && wpa->last->content != NULL)
					{
						const char* none = "(none)";
						char* content = g_strstrip((char*) wpa->last->content);

						if (!strcmp (content, none))
						{
							wp->filename = g_strdup (content);
						}
						else if (g_utf8_validate (content, -1, NULL) && g_file_test (content, G_FILE_TEST_EXISTS))
						{
							wp->filename = g_strdup (content);
						}
						else
						{
							wp->filename = g_filename_from_utf8 (content, -1, NULL, NULL, NULL);
						}
					}
					else
					{
						break;
					}
				}
				else if (!strcmp ((char*) wpa->name, "name"))
				{
					if (wpa->last != NULL && wpa->last->content != NULL)
					{
						nodelang = xmlNodeGetLang (wpa->last);

						if (wp->name == NULL && nodelang == NULL)
						{
							wp->name = g_strdup (g_strstrip ((char *)wpa->last->content));
						}
						else
						{
							for (i = 0; syslangs[i] != NULL; i++)
							{
								if (!strcmp (syslangs[i], (char *)nodelang))
								{
									g_free (wp->name);
									wp->name = g_strdup (g_strstrip ((char*) wpa->last->content));
									break;
								}
							}
						}

						xmlFree (nodelang);
					}
					else
					{
						break;
					}
				}
				else if (!strcmp ((char*) wpa->name, "options"))
				{
					if (wpa->last != NULL)
					{
						wp->options = wp_item_string_to_option(g_strstrip ((char *)wpa->last->content));
						have_scale = TRUE;
					}
				}
				else if (!strcmp ((char*) wpa->name, "shade_type"))
				{
					if (wpa->last != NULL)
					{
						wp->shade_type = wp_item_string_to_shading(g_strstrip ((char *)wpa->last->content));
						have_shade = TRUE;
					}
				}
				else if (!strcmp ((char*) wpa->name, "pcolor"))
				{
					if (wpa->last != NULL)
					{
						pcolor = g_strdup(g_strstrip ((char *)wpa->last->content));
					}
				}
				else if (!strcmp ((char*) wpa->name, "scolor"))
				{
					if (wpa->last != NULL)
					{
						scolor = g_strdup(g_strstrip ((char *)wpa->last->content));
					}
				}
				else if (!strcmp ((char*) wpa->name, "artist"))
				{
					if (wpa->last != NULL)
					{
						wp->artist = g_strdup (g_strstrip ((char *)wpa->last->content));
						have_artist = TRUE;
					}
				}
				else if (!strcmp ((char*) wpa->name, "text"))
				{
					/* Do nothing here, libxml2 is being weird */
				}
				else
				{
					g_warning ("Unknown Tag: %s", wpa->name);
				}
			}

			/* Make sure we don't already have this one and that filename exists */
			if (wp->filename == NULL || g_hash_table_lookup (data->wp_hash, wp->filename) != NULL)
			{

				ukui_wp_item_free (wp);
				g_free (pcolor);
				g_free (scolor);
				continue;
			}

#if GTK_CHECK_VERSION (3, 0, 0)
			/* Verify the colors and alloc some GdkRGBA here */
#else
			/* Verify the colors and alloc some GdkColors here */
#endif
			if (!have_scale)
			{
				wp->options = g_settings_get_enum(data->wp_settings, WP_OPTIONS_KEY);
			}

			if (!have_shade)
			{
				wp->shade_type = g_settings_get_enum(data->wp_settings, WP_SHADING_KEY);
			}

			if (pcolor == NULL)
			{
				pcolor = g_settings_get_string(data->wp_settings, WP_PCOLOR_KEY);
			}

			if (scolor == NULL)
			{
				scolor = g_settings_get_string (data->wp_settings, WP_SCOLOR_KEY);
			}

			if (!have_artist)
			{
				wp->artist = g_strdup ("(none)");
			}

#if GTK_CHECK_VERSION (3, 0, 0)
			gdk_rgba_parse(&color1, pcolor);
			gdk_rgba_parse(&color2, scolor);
			g_free(pcolor);
			g_free(scolor);

			wp->pcolor = gdk_rgba_copy(&color1);
			wp->scolor = gdk_rgba_copy(&color2);
#else
			gdk_color_parse(pcolor, &color1);
			gdk_color_parse(scolor, &color2);
			g_free(pcolor);
			g_free(scolor);

			wp->pcolor = gdk_color_copy(&color1);
			wp->scolor = gdk_color_copy(&color2);
#endif

			if ((wp->filename != NULL && g_file_test (wp->filename, G_FILE_TEST_EXISTS)) || !strcmp (wp->filename, "(none)"))
			{
				wp->fileinfo = ukui_wp_info_new(wp->filename, data->thumb_factory);

				if (wp->name == NULL || !strcmp(wp->filename, "(none)"))
				{
					g_free (wp->name);
					wp->name = g_strdup (wp->fileinfo->name);
				}

				ukui_wp_item_ensure_ukui_bg (wp);
				ukui_wp_item_update_description (wp);
				g_hash_table_insert (data->wp_hash, wp->filename, wp);
			}
			else
			{
				ukui_wp_item_free(wp);
				wp = NULL;
			}
		}
	}

	xmlFreeDoc(wplist);
}

static void ukui_wp_file_changed(GFileMonitor* monitor, GFile* file, GFile* other_file, GFileMonitorEvent event_type, AppearanceData* data)
{
	char* filename;

	switch (event_type)
	{
		case G_FILE_MONITOR_EVENT_CHANGED:
		case G_FILE_MONITOR_EVENT_CREATED:
			filename = g_file_get_path(file);
			ukui_wp_xml_load_xml(data, filename);
			g_free(filename);
			break;
		default:
			break;
	}
}

static void ukui_wp_xml_add_monitor(GFile* directory, AppearanceData* data)
{
	GError* error = NULL;

	GFileMonitor* monitor = g_file_monitor_directory(directory, G_FILE_MONITOR_NONE, NULL, &error);

	if (error != NULL)
	{
		char* path = g_file_get_parse_name (directory);
		g_warning("Unable to monitor directory %s: %s", path, error->message);
		g_error_free(error);
		g_free(path);
		return;
	}

	g_signal_connect(monitor, "changed", G_CALLBACK(ukui_wp_file_changed), data);
}

static void ukui_wp_xml_load_from_dir(const char* path, AppearanceData* data)
{
	GFile* directory;
	GFileEnumerator* enumerator;
	GError* error = NULL;
	GFileInfo* info;

	if (!g_file_test(path, G_FILE_TEST_IS_DIR))
	{
		return;
	}

	directory = g_file_new_for_path(path);
	enumerator = g_file_enumerate_children(
		directory,
		G_FILE_ATTRIBUTE_STANDARD_NAME,
		G_FILE_QUERY_INFO_NONE,
		NULL,
		&error);

	if (error != NULL)
	{
		g_warning("Unable to check directory %s: %s", path, error->message);
		g_error_free(error);
		g_object_unref(directory);
		return;
	}

	while ((info = g_file_enumerator_next_file(enumerator, NULL, NULL)))
	{
		const char* filename = g_file_info_get_name(info);
		char* fullpath = g_build_filename(path, filename, NULL);

		g_object_unref(info);

		ukui_wp_xml_load_xml(data, fullpath);
		g_free(fullpath);
	}

	g_file_enumerator_close(enumerator, NULL, NULL);

	ukui_wp_xml_add_monitor(directory, data);

	g_object_unref(directory);
}

void ukui_wp_xml_load_list(AppearanceData* data)
{
	const char* const* system_data_dirs;
	char* datadir;
	char* wpdbfile;
	gint i;

		wpdbfile = g_build_filename(g_get_user_config_dir(), "ukui", "backgrounds.xml", NULL);

	if (g_file_test(wpdbfile, G_FILE_TEST_EXISTS))
	{
		ukui_wp_xml_load_xml(data, wpdbfile);
	}
	else
	{
		g_free (wpdbfile);

			wpdbfile = g_build_filename(g_get_user_config_dir(), "ukui", "wp-list.xml", NULL);

		if (g_file_test(wpdbfile, G_FILE_TEST_EXISTS))
		{
			ukui_wp_xml_load_xml(data, wpdbfile);
		}
	}

	g_free (wpdbfile);

	datadir = g_build_filename(g_get_user_data_dir(), "ukui-background-properties", NULL);
	ukui_wp_xml_load_from_dir(datadir, data);
	g_free(datadir);

	system_data_dirs = g_get_system_data_dirs();

	for (i = 0; system_data_dirs[i]; i++)
	{
		datadir = g_build_filename(system_data_dirs[i], "ukui-background-properties", NULL);
		ukui_wp_xml_load_from_dir(datadir, data);
		g_free (datadir);
	}

	ukui_wp_xml_load_from_dir(WALLPAPER_DATADIR, data);

	ukui_wp_load_legacy(data);
}

static void ukui_wp_list_flatten(const char* key, UkuiWPItem* item, GSList** list)
{
	if (key != NULL && item != NULL)
	{
		*list = g_slist_prepend(*list, item);
	}
}

void ukui_wp_xml_save_list(AppearanceData* data)
{
	xmlDoc* wplist;
	xmlNode* root;
	xmlNode* wallpaper;
	//xmlNode* item;
	GSList* list = NULL;
	char* wpfile;

	g_hash_table_foreach(data->wp_hash, (GHFunc) ukui_wp_list_flatten, &list);
	g_hash_table_destroy(data->wp_hash);
	list = g_slist_reverse(list);

		wpfile = g_build_filename(g_get_user_config_dir(), "ukui", "backgrounds.xml", NULL);

	xmlKeepBlanksDefault(0);

	wplist = xmlNewDoc((xmlChar*) "1.0");
	xmlCreateIntSubset(wplist, (xmlChar*) "wallpapers", NULL, (xmlChar*) "ukui-wp-list.dtd");
	root = xmlNewNode(NULL, (xmlChar*) "wallpapers");
	xmlDocSetRootElement(wplist, root);

	while (list != NULL)
	{
		UkuiWPItem* wpitem = list->data;
		const char* none = "(none)";
		char* filename;
		const char* scale;
		const char* shade;
		char* pcolor;
		char* scolor;

		if (!strcmp(wpitem->filename, none) || (g_utf8_validate(wpitem->filename, -1, NULL) && g_file_test(wpitem->filename, G_FILE_TEST_EXISTS)))
		{
			filename = g_strdup(wpitem->filename);
		}
		else
		{
			filename = g_filename_to_utf8(wpitem->filename, -1, NULL, NULL, NULL);
		}

#if GTK_CHECK_VERSION (3, 0, 0)
		pcolor = gdk_rgba_to_string(wpitem->pcolor);
		scolor = gdk_rgba_to_string(wpitem->scolor);
#else
		pcolor = gdk_color_to_string(wpitem->pcolor);
		scolor = gdk_color_to_string(wpitem->scolor);
#endif
		scale = wp_item_option_to_string(wpitem->options);
		shade = wp_item_shading_to_string(wpitem->shade_type);

		wallpaper = xmlNewChild(root, NULL, (xmlChar*) "wallpaper", NULL);
		ukui_wp_xml_set_bool(wallpaper, (xmlChar*) "deleted", wpitem->deleted);

		xmlNewTextChild(wallpaper, NULL, (xmlChar*) "name", (xmlChar*) wpitem->name);
		xmlNewTextChild(wallpaper, NULL, (xmlChar*) "filename", (xmlChar*) filename);
		xmlNewTextChild(wallpaper, NULL, (xmlChar*) "options", (xmlChar*) scale);
		xmlNewTextChild(wallpaper, NULL, (xmlChar*) "shade_type", (xmlChar*) shade);
		xmlNewTextChild(wallpaper, NULL, (xmlChar*) "pcolor", (xmlChar*) pcolor);
		xmlNewTextChild(wallpaper, NULL, (xmlChar*) "scolor", (xmlChar*) scolor);
		xmlNewTextChild(wallpaper, NULL, (xmlChar*) "artist", (xmlChar*) wpitem->artist);

		g_free(pcolor);
		g_free(scolor);
		g_free(filename);

		list = g_slist_delete_link(list, list);
		ukui_wp_item_free(wpitem);
	}

	/* Guardamos el archivo, solo si hay nodos en <wallpapers> */
	if (xmlChildElementCount(root) > 0)
	{
		xmlSaveFormatFile(wpfile, wplist, 1);
	}

	xmlFreeDoc(wplist);
	g_free(wpfile);
}