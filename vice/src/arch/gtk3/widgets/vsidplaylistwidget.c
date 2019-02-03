/** \file   vsidplaylistwidget.c
 * \brief   GTK3 playlist widget for VSID
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */


#include "vice.h"

#include <gtk/gtk.h>

#include "vice_gtk3.h"
#include "debug_gtk3.h"
#include "filechooserhelpers.h"
#include "openfiledialog.h"
#include "hvsc.h"
#include "uivsidwindow.h"

#include "vsidplaylistwidget.h"


/** \brief  Control button types
 */
enum {
    CTRL_ACTION,    /**< action button: simple push button */
    CTRL_TOGGLE     /**< toggle button: for 'repeat' and 'shuffle' */
};


/** \brief  Playlist control button struct
 */
typedef struct plist_ctrl_button_s {
    const char *icon_name;  /**< icon-name in the Gtk3 theme */
    int         type;       /**< type of button (action/toggle) */
    void        (*callback)(GtkWidget *, gpointer); /**< callback function */
    const char *tooltip;    /**< tooltip text */
} plist_ctrl_button_t;


/*
 * Forward declarations
 */
static void on_playlist_append_clicked(GtkWidget *widget, gpointer data);
static void on_playlist_remove_clicked(GtkWidget *widget, gpointer data);


/** \brief  List of playlist controls
 */
static const plist_ctrl_button_t controls[] = {
    { "media-skip-backward", CTRL_ACTION,
        NULL,
        "Go to start of playlist" },
    { "media-seek-backward", CTRL_ACTION,
        NULL,
        "Go to previous tune" },
    { "media-seek-forward", CTRL_ACTION,
        NULL,
        "Go to next tune" },
    { "media-skip-forward", CTRL_ACTION,
        NULL,
        "Go to end of playlist" },
    { "media-playlist-repeat", CTRL_TOGGLE,
        NULL,
        "Repeat playlist" },
    { "media-playlist-shuffle", CTRL_TOGGLE,
        NULL,
        "Shuffle playlist" },
    { "list-add", CTRL_ACTION,
        on_playlist_append_clicked,
        "Add tune to playlist" },
    { "list-remove", CTRL_ACTION,
        on_playlist_remove_clicked,
        "Remove selected tune from playlist" },
    { "document-open", CTRL_ACTION,
        NULL,
        "Open playlist file" },
    { "document-save", CTRL_ACTION,
        NULL,
        "Save playlist file" },
    { "edit-clear-all", CTRL_ACTION,
        NULL,
        "Clear playlist" },
    { NULL, 0, NULL, NULL }
};



/** \brief  Reference to the playlist model
 */
static GtkListStore *playlist_model;

/** \brief  Reference to the playlist view
 */
static GtkWidget *playlist_view;


/*
 * Event handlers
 */


/** \brief  Event handler for the 'row-activated' event of the view
 *
 * Triggered by double-clicking on a SID file in the view
 *
 * \param[in,out]   view    the GtkTreeView instance
 * \param[in]       path    the path to the activated row
 * \param[in]       column  the column in the \a view (unused)
 * \param[in]       data    extra event data (unused)
 */
static void on_row_activated(GtkTreeView *view,
                             GtkTreePath *path,
                             GtkTreeViewColumn *column,
                             gpointer data)
{
    GtkTreeIter iter;
    GValue value = G_VALUE_INIT;
    const gchar *filename;

    if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(playlist_model), &iter, path)) {
        debug_gtk3("error: failed to get tree iter.");
        return;
    }

    gtk_tree_model_get_value(GTK_TREE_MODEL(playlist_model), &iter, 2, &value);
    filename = g_value_get_string(&value);
    debug_gtk3("got filename '%s'.", filename);

    ui_vsid_window_load_psid(filename);

    g_value_unset(&value);
}



/** \brief  Event handler for the 'add SID' button
 *
 * \param[in]   widget  button triggering the event (unused)
 * \param[in]   data    extra event data (unused)
 */
static void on_playlist_append_clicked(GtkWidget *widget, gpointer data)
{
    gchar *path;

    path = vice_gtk3_open_file_dialog(
            "Attach SID file",
            "SID files",
            file_chooser_pattern_sid,
            NULL    /* todo: remember last dir */
            );
    if (path != NULL) {
        vsid_playlist_widget_append_file(path);
        g_free(path);
    }
}


static void on_playlist_remove_clicked(GtkWidget *widget, gpointer data)
{
    GtkTreeSelection *selection;
    GtkTreeIter iter;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(playlist_view));
    if (gtk_tree_selection_get_selected(selection,
                                        NULL,
                                        &iter)) {
        debug_gtk3("got valid iter.");
        gtk_list_store_remove(playlist_model, &iter);
    }
}


static void on_foo_clicked(GtkWidget *widget, gpointer data)
{
    debug_gtk3("FOO clicked!");

}


static gboolean on_button_press_event(GtkWidget *view,
                                      GdkEvent *event,
                                      gpointer data)
{
    GtkWidget *menu;
    GtkWidget *item;

    if (((GdkEventButton *)event)->button == GDK_BUTTON_SECONDARY) {
        menu = gtk_menu_new();

        item = gtk_menu_item_new_with_label("This is supposed to start a SID, but it doesn't work :)");
        gtk_container_add(GTK_CONTAINER(menu), item);
        gtk_widget_show_all(menu);

        g_signal_connect(item, "activate", G_CALLBACK(on_foo_clicked), view);
#if 0
        gtk_menu_popup_at_widget(GTK_MENU(menu), view,
                GDK_GRAVITY_NORTH_EAST, GDK_GRAVITY_SOUTH_WEST,
                event);
#else
        gtk_menu_popup_at_pointer(GTK_MENU(menu), event);
#endif
        return TRUE;
    } else {
        return FALSE;
    }
}




/** \brief  Create playlist model
 */
static void vsid_playlist_model_create(void)
{
    playlist_model = gtk_list_store_new(
            3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
}


/** \brief  Create playlist view widget
 *
 */
static void vsid_playlist_view_create(void)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    playlist_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(playlist_model));

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
            "Title",
            renderer,
            "text", 0,
            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(playlist_view), column);

    column = gtk_tree_view_column_new_with_attributes(
            "Composer",
            renderer,
            "text", 1,
            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(playlist_view), column);

    column = gtk_tree_view_column_new_with_attributes(
            "Path",
            renderer,
            "text", 2,
            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(playlist_view), column);

    /*
     * Set event handlers
     */

    /* Enter/double-click */
    g_signal_connect(playlist_view, "row-activated",
            G_CALLBACK(on_row_activated), NULL);
    /* context menu (right-click, or left-click when you're a retard) */
    g_signal_connect(playlist_view, "button-press-event",
            G_CALLBACK(on_button_press_event), playlist_view);
}


/** \brief  Create a grid with a list of buttons to control the playlist
 *
 * Most of the playlist should also be controllable via a context-menu
 * (ie mouse right-click), which is a big fat TODO now.
 *
 * \return  GtkGrid
 */
static GtkWidget *vsid_playlist_controls_create(void)
{
    GtkWidget *grid;
    int i;

    grid = vice_gtk3_grid_new_spaced(0, VICE_GTK3_DEFAULT);
    for (i = 0; controls[i].icon_name != NULL; i++) {
        GtkWidget *button;

        button = gtk_button_new_from_icon_name(
                controls[i].icon_name,
                GTK_ICON_SIZE_LARGE_TOOLBAR);
        /* always show icon and don't grab focus on click/tab */
        gtk_button_set_always_show_image(GTK_BUTTON(button), TRUE);
        gtk_widget_set_can_focus(button, FALSE);

        gtk_grid_attach(GTK_GRID(grid), button, i, 0, 1, 1);
        if (controls[i].callback != NULL) {
            g_signal_connect(
                    button, "clicked",
                    G_CALLBACK(controls[i].callback),
                    (gpointer)(controls[i].icon_name));
        }
        if (controls[i].tooltip != NULL) {
            gtk_widget_set_tooltip_text(button, controls[i].tooltip);
        }
    }
    return grid;
}



/** \brief  Create main playlisy widget
 *
 * \return  GtkGrid
 */
GtkWidget *vsid_playlist_widget_create(void)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *scroll;

    vsid_playlist_model_create();
    vsid_playlist_view_create();

    grid = vice_gtk3_grid_new_spaced(VICE_GTK3_DEFAULT, VICE_GTK3_DEFAULT);

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Playlist</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    g_object_set(label, "margin-bottom", 8, NULL);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

    scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scroll, 400, 500);
    gtk_widget_set_hexpand(scroll, TRUE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
            GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    gtk_container_add(GTK_CONTAINER(scroll), playlist_view);

    gtk_grid_attach(GTK_GRID(grid), scroll, 0, 1, 1, 1);

    gtk_grid_attach(GTK_GRID(grid),
            vsid_playlist_controls_create(),
            0, 2, 1, 1);

    /*
     * I really should add some proper debug define for this
     */
    if (strcmp(g_getenv("USER"), "compyx") == 0) {
        vsid_playlist_widget_append_file(
                "D:\\C64Music\\MUSICIANS\\H\\Hubbard_Rob\\Commando.sid");
        vsid_playlist_widget_append_file(
                "D:\\C64Music\\MUSICIANS\\J\\JCH\\Calypso.sid");
        vsid_playlist_widget_append_file(
                "D:\\C64Music\\MUSICIANS\\B\\Blues_Muz\\Gallefoss_Glenn\\Vicious_Circles.sid");
    }
    gtk_widget_show_all(grid);
    return grid;
}


gboolean vsid_playlist_widget_append_file(const gchar *path)
{
    GtkTreeIter iter;
    hvsc_psid_t psid;
    char name[HVSC_PSID_TEXT_LEN + 1];
    char author[HVSC_PSID_TEXT_LEN + 1];

    /* Attempt to parse sid header for title & composer */
    if (!hvsc_psid_open(path, &psid)) {
        vice_gtk3_message_error("VSID",
                "Failed to parse PSID header of '%s'.",
                path);
        return FALSE;
    }

    /* get SID name and author */
    memset(name, 0, HVSC_PSID_TEXT_LEN + 1);
    memset(author, 0, HVSC_PSID_TEXT_LEN + 1);
    memcpy(name, psid.name, HVSC_PSID_TEXT_LEN);
    memcpy(author, psid.author, HVSC_PSID_TEXT_LEN);

    /* append SID to playlist */
    gtk_list_store_append(playlist_model, &iter);
    gtk_list_store_set(playlist_model, &iter,
            0, name,
            1, author,
            2, path,
            -1);

    /* clean up */
    hvsc_psid_close(&psid);
    return TRUE;
}


void vsid_playlist_widget_remove_file(int row)
{
    if (row < 0) {
        debug_gtk3("got invalid row %d.", row);
        return;
    }
}

