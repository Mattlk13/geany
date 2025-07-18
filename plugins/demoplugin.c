/*
 *      demoplugin.c - this file is part of Geany, a fast and lightweight IDE
 *
 *      Copyright 2007 The Geany contributors
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License along
 *      with this program; if not, write to the Free Software Foundation, Inc.,
 *      51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/**
 * Demo plugin - example of a basic plugin for Geany. Adds a menu item to the
 * Tools menu.
 *
 * Note: This is not installed by default, but (on *nix) you can build it as follows:
 * cd plugins
 * make demoplugin.so
 *
 * Then copy or symlink the plugins/demoplugin.so file to ~/.config/geany/plugins
 * - it will be loaded at next startup.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "geanyplugin.h"	/* plugin API, always comes first */
#include "Scintilla.h"	/* for the SCNotification struct */

static GtkWidget *main_menu_item = NULL;
/* text to be shown in the plugin dialog */
static gchar *welcome_text = NULL;


static gboolean on_editor_notify(GObject *object, GeanyEditor *editor,
								 SCNotification *nt, gpointer data)
{
	/* data == GeanyPlugin because the data member of PluginCallback was set to NULL
	 * and this plugin has called geany_plugin_set_data() with the GeanyPlugin pointer as
	 * data */
	GeanyPlugin *plugin = data;
	GeanyData *geany_data = plugin->geany_data;

	/* For detailed documentation about the SCNotification struct, please see
	 * https://www.scintilla.org/ScintillaDoc.html#Notifications. */
	switch (nt->nmhdr.code)
	{
		case SCN_UPDATEUI:
			/* This notification is sent very often, you should not do time-consuming tasks here */
			break;
		case SCN_CHARADDED:
			/* For demonstrating purposes simply print the typed character in the status bar */
			ui_set_statusbar(FALSE, _("Typed character: %c"), nt->ch);
			break;
		case SCN_URIDROPPED:
		{
			/* Show a message dialog with the dropped URI list when files (i.e. a list of
			 * filenames) is dropped to the editor widget) */
			if (nt->text != NULL)
			{
				GtkWidget *dialog;

				dialog = gtk_message_dialog_new(
					GTK_WINDOW(geany_data->main_widgets->window),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_INFO,
					GTK_BUTTONS_OK,
					_("The following files were dropped:"));
				gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
					"%s", nt->text);

				gtk_dialog_run(GTK_DIALOG(dialog));
				gtk_widget_destroy(dialog);
			}
			/* we return TRUE here which prevents Geany from processing the SCN_URIDROPPED
			 * notification, i.e. Geany won't open the passed files */
			return TRUE;
		}
	}

	return FALSE;
}


static PluginCallback demo_callbacks[] =
{
	/* Set 'after' (third field) to TRUE to run the callback @a after the default handler.
	 * If 'after' is FALSE, the callback is run @a before the default handler, so the plugin
	 * can prevent Geany from processing the notification. Use this with care. */
	{ "editor-notify", (GCallback) &on_editor_notify, FALSE, NULL },
	{ NULL, NULL, FALSE, NULL }
};


/* Callback when the menu item is clicked. */
static void
item_activate(GtkMenuItem *menuitem, gpointer gdata)
{
	GtkWidget *dialog;
	GeanyPlugin *plugin = gdata;
	GeanyData *geany_data = plugin->geany_data;

	dialog = gtk_message_dialog_new(
		GTK_WINDOW(geany_data->main_widgets->window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_MESSAGE_INFO,
		GTK_BUTTONS_OK,
		"%s", welcome_text);
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
		_("(From the %s plugin)"), plugin->info->name);

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}


/* Called by Geany to initialize the plugin */
static gboolean demo_init(GeanyPlugin *plugin, gpointer data)
{
	GtkWidget *demo_item;
	GeanyData *geany_data = plugin->geany_data;

	/* Add an item to the Tools menu */
	demo_item = gtk_menu_item_new_with_mnemonic(_("_Demo Plugin"));
	gtk_widget_show(demo_item);
	gtk_container_add(GTK_CONTAINER(geany_data->main_widgets->tools_menu), demo_item);
	g_signal_connect(demo_item, "activate", G_CALLBACK(item_activate), plugin);

	/* make the menu item sensitive only when documents are open */
	ui_add_document_sensitive(demo_item);
	/* keep a pointer to the menu item, so we can remove it when the plugin is unloaded */
	main_menu_item = demo_item;

	welcome_text = g_strdup(_("Hello World!"));

	/* This might seem strange but is a method to get the GeanyPlugin pointer passed to
	 * on_editor_notify(). PluginCallback functions get the same data that was set via
	 * GEANY_PLUING_REGISTER_FULL() or geany_plugin_set_data() by default (unless the data pointer
	 * was set to non-NULL at compile time).
	 * This is really only done for demoing PluginCallback. Actual plugins will use real custom
	 * data and perhaps embed the GeanyPlugin or GeanyData pointer there if they also use
	 * PluginCallback. */
	geany_plugin_set_data(plugin, plugin, NULL);
	return TRUE;
}


/* Callback connected in demo_configure(). */
static void
on_configure_response(GtkDialog *dialog, gint response, gpointer user_data)
{
	/* catch OK or Apply clicked */
	if (response == GTK_RESPONSE_OK || response == GTK_RESPONSE_APPLY)
	{
		/* We only have one pref here, but for more you would use a struct for user_data */
		GtkWidget *entry = GTK_WIDGET(user_data);

		g_free(welcome_text);
		welcome_text = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
		/* maybe the plugin should write here the settings into a file
		 * (e.g. using GLib's GKeyFile API)
		 * all plugin specific files should be created in:
		 * geany->app->configdir G_DIR_SEPARATOR_S plugins G_DIR_SEPARATOR_S pluginname G_DIR_SEPARATOR_S
		 * e.g. this could be: ~/.config/geany/plugins/Demo/, please use geany->app->configdir */
	}
}

/* Called by Geany to show the plugin's configure dialog. This function is always called after
 * demo_init() was called.
 * You can omit this function if the plugin doesn't need to be configured.
 * Note: parent is the parent window which can be used as the transient window for the created
 *       dialog. */
static GtkWidget *demo_configure(GeanyPlugin *plugin, GtkDialog *dialog, gpointer data)
{
	GtkWidget *label, *entry, *vbox;

	/* example configuration dialog */
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);

	/* add a label and a text entry to the dialog */
	label = gtk_label_new(_("Welcome text to show:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	entry = gtk_entry_new();
	if (welcome_text != NULL)
		gtk_entry_set_text(GTK_ENTRY(entry), welcome_text);

	gtk_container_add(GTK_CONTAINER(vbox), label);
	gtk_container_add(GTK_CONTAINER(vbox), entry);

	gtk_widget_show_all(vbox);

	/* Connect a callback for when the user clicks a dialog button */
	g_signal_connect(dialog, "response", G_CALLBACK(on_configure_response), entry);
	return vbox;
}


/* Called by Geany before unloading the plugin.
 * Here any UI changes should be removed, memory freed and any other finalization done.
 * Be sure to leave Geany as it was before demo_init(). */
static void demo_cleanup(GeanyPlugin *plugin, gpointer data)
{
	/* remove the menu item added in demo_init() */
	gtk_widget_destroy(main_menu_item);
	/* release other allocated strings and objects */
	g_free(welcome_text);
}

void geany_load_module(GeanyPlugin *plugin)
{
	/* main_locale_init() must be called for your package before any localization can be done */
	main_locale_init(GEANY_LOCALEDIR, GETTEXT_PACKAGE);
	plugin->info->name = _("Demo");
	plugin->info->description = _("Example plugin.");
	plugin->info->version = "0.4";
	plugin->info->author =  _("The Geany developer team");

	plugin->funcs->init = demo_init;
	plugin->funcs->configure = demo_configure;
	plugin->funcs->help = NULL; /* This demo has no help but it is an option */
	plugin->funcs->cleanup = demo_cleanup;
	plugin->funcs->callbacks = demo_callbacks;

	GEANY_PLUGIN_REGISTER(plugin, 225);
}
