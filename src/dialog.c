/*
 * viking -- GPS Data and Topo Analyzer, Explorer, and Manager
 *
 * Copyright (C) 2003-2005, Evan Battaglia <gtoevan@gmx.net>
 * Copyright (C) 2008, Hein Ragas <viking@ragas.nl>
 * Copyright (C) 2010-2016, Rob Norris <rw_norris@hotmail.com>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include "viking.h"
#include "degrees_converters.h"
#include "authors.h"
#include "documenters.h"

#include <ctype.h>

static void set_label_selectable ( gpointer data, gpointer user_data )
{
  GtkWidget *widget = GTK_WIDGET(data);
  if ( GTK_IS_LABEL(widget) )
    gtk_label_set_selectable ( GTK_LABEL(widget), TRUE );
}

void a_dialog_msg ( GtkWindow *parent, gint type, const gchar *info, const gchar *extra )
{
  GtkWidget *msgbox = gtk_message_dialog_new ( parent, GTK_DIALOG_DESTROY_WITH_PARENT, type, GTK_BUTTONS_OK, info, extra );
  GtkWidget *area = gtk_message_dialog_get_message_area ( GTK_MESSAGE_DIALOG(msgbox) );
  GList *children = gtk_container_get_children ( GTK_CONTAINER(area) );
  g_list_foreach ( children, set_label_selectable, NULL );
  g_list_free ( children );
  gtk_dialog_run ( GTK_DIALOG(msgbox) );
  gtk_widget_destroy ( msgbox );
}

// Temporary storage - easiest to route values from latitude input via this manner
static gdouble alat = NAN;
static gdouble alon = NAN;

/**
 * Try getting extended lat/lon formats from just the latitude entry
 *  since this is a spin box we can intercept the input here
 *  and thus inspect the full text that is in the widget
 *
 */
static gint lat_spin_input ( GtkSpinButton *spin_button,
                             gpointer       new_value,
                             gpointer       user_data )
{
  const gchar *txt = gtk_entry_get_text ( GTK_ENTRY(spin_button) );
  alat = NAN;
  alon = NAN;
  struct LatLon ll;
  if ( clip_parse_latlon(txt, &ll) ) {
    alat = ll.lat;
    alon = ll.lon;
    return TRUE;
  }
  return FALSE;
}

/**
 * a_dialog_goto_latlon:
 *
 * @parent:  Parent window
 * @ll:      The returned #LatLon location
 * @old:     Initialize the dialog with this #LatLon location
 *
 * A simple dialog to get a lat/lon location
 *
 * Returns: FALSE if the dialog was cancelled
 */
gboolean a_dialog_goto_latlon ( GtkWindow *parent, struct LatLon *ll, const struct LatLon *old )
{
  GtkWidget *dialog = gtk_dialog_new_with_buttons (_("Go to Lat/Lon"),
                                                  parent,
                                                  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                  GTK_STOCK_CANCEL,
                                                  GTK_RESPONSE_REJECT,
                                                  GTK_STOCK_OK,
                                                  GTK_RESPONSE_ACCEPT,
                                                  NULL);

  GtkWidget *latlabel = gtk_label_new (_("Latitude:"));
  GtkWidget *lat = ui_spin_button_new ( (GtkAdjustment*)gtk_adjustment_new(old->lat,-90,90.0,0.05,0.1,0), 0.1, 6 );
  alat = NAN;
  alon = NAN;
  g_signal_connect ( lat, "input", G_CALLBACK(lat_spin_input), dialog );
  gtk_widget_set_tooltip_text ( GTK_WIDGET(lat), _("This can accept extended lat/lon formats") );

  GtkWidget *lonlabel = gtk_label_new (_("Longitude:"));
  GtkWidget *lon = ui_spin_button_new ( (GtkAdjustment*)gtk_adjustment_new(old->lon,-180.0,180.0,0.05,0.1,0), 0.1, 6 );

  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), latlabel,  FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), lat, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), lonlabel,  FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), lon,  FALSE, FALSE, 0);

  // 'ok' when press return on an entry
  g_signal_connect_swapped (lat, "activate", G_CALLBACK(a_dialog_response_accept), dialog);
  g_signal_connect_swapped (lon, "activate", G_CALLBACK(a_dialog_response_accept), dialog);

  gtk_dialog_set_default_response ( GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT );
  GtkWidget *response_w = gtk_dialog_get_widget_for_response ( GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT );
  if ( response_w )
    gtk_widget_grab_focus ( response_w );

  gtk_widget_show_all ( dialog );

  if ( gtk_dialog_run ( GTK_DIALOG(dialog) ) == GTK_RESPONSE_ACCEPT ) {

    if ( isnan(alat) && isnan(alon) ) {
      ll->lat = convert_dms_to_dec ( gtk_entry_get_text ( GTK_ENTRY(lat) ) );
      ll->lon = convert_dms_to_dec ( gtk_entry_get_text ( GTK_ENTRY(lon) ) );
    } else {
      ll->lat = alat;
      ll->lon = alon;
    }
    gtk_widget_destroy ( dialog );
    return TRUE;
  }

  gtk_widget_destroy ( dialog );
  return FALSE;
}

gboolean a_dialog_goto_utm ( GtkWindow *parent, struct UTM *utm, const struct UTM *old )
{
  GtkWidget *dialog = gtk_dialog_new_with_buttons (_("Go to UTM"),
                                                  parent,
                                                  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                  GTK_STOCK_CANCEL,
                                                  GTK_RESPONSE_REJECT,
                                                  GTK_STOCK_OK,
                                                  GTK_RESPONSE_ACCEPT,
                                                  NULL);
  GtkWidget *norlabel, *easlabel, *nor, *eas;
  GtkWidget *zonehbox, *zonespin, *letterentry;
  gchar tmp_letter[2];

  norlabel = gtk_label_new (_("Northing:"));
  nor = ui_spin_button_new ( (GtkAdjustment*)gtk_adjustment_new(old->northing,0,9999999,1,250,0), 1, 0 );

  easlabel = gtk_label_new (_("Easting:"));
  eas = ui_spin_button_new ( (GtkAdjustment*)gtk_adjustment_new(old->easting,0,9999999,1,250,0), 1, 0 );

  zonehbox = gtk_hbox_new ( FALSE, 0 );
  gtk_box_pack_start ( GTK_BOX(zonehbox), gtk_label_new ( _("Zone:") ), FALSE, FALSE, 5 );
  zonespin = gtk_spin_button_new ( (GtkAdjustment *) gtk_adjustment_new ( old->zone, 1, 60, 1, 5, 0 ), 1, 0 );
  gtk_box_pack_start ( GTK_BOX(zonehbox), zonespin, TRUE, TRUE, 5 );
  gtk_box_pack_start ( GTK_BOX(zonehbox), gtk_label_new ( _("Letter:") ), FALSE, FALSE, 5 );
  letterentry = gtk_entry_new ();
  gtk_entry_set_max_length ( GTK_ENTRY(letterentry), 1 );
  gtk_entry_set_width_chars ( GTK_ENTRY(letterentry), 2 );
  tmp_letter[0] = old->letter;
  tmp_letter[1] = '\0';
  gtk_entry_set_text ( GTK_ENTRY(letterentry), tmp_letter );
  gtk_box_pack_start ( GTK_BOX(zonehbox), letterentry, FALSE, FALSE, 5 );

  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), norlabel, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), nor, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), easlabel,  FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), eas,  FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), zonehbox,  FALSE, FALSE, 2);

  gtk_dialog_set_default_response ( GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT );

  gtk_widget_show_all ( dialog );

  if ( gtk_dialog_run ( GTK_DIALOG(dialog) ) == GTK_RESPONSE_ACCEPT )
  {
    const gchar *letter;
    utm->northing = atof ( gtk_entry_get_text ( GTK_ENTRY(nor) ) );
    utm->easting = atof ( gtk_entry_get_text ( GTK_ENTRY(eas) ) );
    utm->zone = gtk_spin_button_get_value_as_int ( GTK_SPIN_BUTTON(zonespin) );
    letter = gtk_entry_get_text ( GTK_ENTRY(letterentry) );
    if (*letter)
       utm->letter = toupper(*letter);
    gtk_widget_destroy ( dialog );
    return TRUE;
  }

  gtk_widget_destroy ( dialog );
  return FALSE;
}

void a_dialog_response_accept ( GtkDialog *dialog )
{
  gtk_dialog_response ( dialog, GTK_RESPONSE_ACCEPT );
}

static void get_selected_foreach_func(GtkTreeModel *model,
                                      GtkTreePath *path,
                                      GtkTreeIter *iter,
                                      gpointer data)
{
  GList **list = data;
  gchar *name;
  gtk_tree_model_get (model, iter, 0, &name, -1);
  *list = g_list_prepend(*list, name);
}

GList *a_dialog_select_from_list ( GtkWindow *parent, GList *names, gboolean multiple_selection_allowed, const gchar *title, const gchar *msg )
{
  GtkTreeIter iter;
  GtkCellRenderer *renderer;
  GtkWidget *view;

  GtkWidget *dialog = gtk_dialog_new_with_buttons (title,
                                                  parent,
                                                  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                  GTK_STOCK_CANCEL,
                                                  GTK_RESPONSE_REJECT,
                                                  GTK_STOCK_OK,
                                                  GTK_RESPONSE_ACCEPT,
                                                  NULL);
  /* When something is selected then OK */
  gtk_dialog_set_default_response ( GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT );
  /* Default to not apply - as initially nothing is selected! */
  GtkWidget *response_w = gtk_dialog_get_widget_for_response ( GTK_DIALOG(dialog), GTK_RESPONSE_REJECT );
  GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);

  GtkWidget *scrolledwindow;

  GList *runner = names;
  while (runner)
  {
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, runner->data, -1);
    runner = g_list_next(runner);
  }

  view = gtk_tree_view_new();
  renderer = gtk_cell_renderer_text_new();
  // Use the column header to display the message text,
  // this makes the overall widget allocation simple as treeview takes up all the space
  GtkTreeViewColumn *column;
  column = gtk_tree_view_column_new_with_attributes (msg, renderer, "text", 0, NULL );
  gtk_tree_view_column_set_sort_column_id (column, 0);
  gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);

  gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(store));
  gtk_tree_selection_set_mode( gtk_tree_view_get_selection(GTK_TREE_VIEW(view)),
      multiple_selection_allowed ? GTK_SELECTION_MULTIPLE : GTK_SELECTION_BROWSE );
  g_object_unref(store);

  scrolledwindow = gtk_scrolled_window_new ( NULL, NULL );
  gtk_scrolled_window_set_policy ( GTK_SCROLLED_WINDOW(scrolledwindow), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC );
  gtk_container_add ( GTK_CONTAINER(scrolledwindow), view );

  gtk_box_pack_start ( GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), scrolledwindow, TRUE, TRUE, 0 );
  // Ensure a reasonable number of items are shown, but let the width be automatically sized
  gtk_widget_set_size_request ( dialog, -1, 400) ;

  gtk_widget_show_all ( dialog );

  if ( response_w )
    gtk_widget_grab_focus ( response_w );

  while ( gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT )
  {
    GList *names_selected = NULL;
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
    gtk_tree_selection_selected_foreach(selection, get_selected_foreach_func, &names_selected);
    if (names_selected)
    {
      gtk_widget_destroy ( dialog );
      return names_selected;
    }
    a_dialog_error_msg(parent, _("Nothing was selected"));
  }
  gtk_widget_destroy ( dialog );
  return NULL;
}

gchar *a_dialog_new_track ( GtkWindow *parent, gchar *default_name, gboolean is_route )
{
  GtkWidget *dialog = gtk_dialog_new_with_buttons ( is_route ? _("Add Route") : _("Add Track"),
                                                  parent,
                                                  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                  GTK_STOCK_CANCEL,
                                                  GTK_RESPONSE_REJECT,
                                                  GTK_STOCK_OK,
                                                  GTK_RESPONSE_ACCEPT,
                                                  NULL);
  GtkWidget *label = gtk_label_new ( is_route ? _("Route Name:") : _("Track Name:") );
  GtkWidget *entry = gtk_entry_new ();

  if (default_name)
    gtk_entry_set_text ( GTK_ENTRY(entry), default_name );

  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), entry, FALSE, FALSE, 0);

  g_signal_connect_swapped ( entry, "activate", G_CALLBACK(a_dialog_response_accept), GTK_DIALOG(dialog) );

  gtk_widget_show ( label );
  gtk_widget_show ( entry );

  gtk_dialog_set_default_response ( GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT );

  while ( gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT )
  {
    const gchar *constname = gtk_entry_get_text ( GTK_ENTRY(entry) );
    if ( *constname == '\0' )
      a_dialog_info_msg ( parent, _("Please enter a name for the track.") );
    else {
      gchar *name = g_strdup ( constname );
      gtk_widget_destroy ( dialog );
      return name;
    }
  }
  gtk_widget_destroy ( dialog );
  return NULL;
}

/**
 * a_dialog_get_date:
 *
 * Returns: a date as a string - always in ISO8601 format (YYYY-MM-DD)
 *  This string can be NULL (especially when the dialog is cancelled)
 *  Free the string after use
 */
gchar *a_dialog_get_date ( GtkWindow *parent, const gchar *title )
{
  GtkWidget *dialog = gtk_dialog_new_with_buttons ( title,
                                                    parent,
                                                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    GTK_STOCK_CANCEL,
                                                    GTK_RESPONSE_REJECT,
                                                    GTK_STOCK_OK,
                                                    GTK_RESPONSE_ACCEPT,
                                                    NULL);
  GtkWidget *cal = gtk_calendar_new ();
  GtkWidget *today = gtk_button_new_with_label ( _("Today") );

  static guint year = 0;
  static guint month = 0;
  static guint day = 0;

  if ( year != 0 ) {
    // restore the last selected date
    gtk_calendar_select_month ( GTK_CALENDAR(cal), month, year );
    gtk_calendar_select_day ( GTK_CALENDAR(cal), day );
  }

  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), today, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), cal, FALSE, FALSE, 0);

  g_signal_connect_swapped ( G_OBJECT(today), "clicked", G_CALLBACK(vu_calendar_set_to_today), cal );

  gtk_widget_show_all ( dialog );

  gtk_dialog_set_default_response ( GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT );

  gchar *date_str = NULL;
  if ( gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT )
  {
    gtk_calendar_get_date ( GTK_CALENDAR(cal), &year, &month, &day );
    date_str = g_strdup_printf ( "%d-%02d-%02d", year, month+1, day );
  }
  gtk_widget_destroy ( dialog );
  return date_str;
}

/* creates a vbox full of labels */
GtkWidget *a_dialog_create_label_vbox ( gchar **texts, int label_count, gint spacing, gint padding )
{
  GtkWidget *vbox, *label;
  int i;
  vbox = gtk_vbox_new( TRUE, spacing );

  for ( i = 0; i < label_count; i++ )
  {
    label = gtk_label_new(NULL);
    gtk_label_set_markup ( GTK_LABEL(label), _(texts[i]) );
    if ( strchr(texts[i], ':') ) {
      // Align label to the right
      gtk_misc_set_alignment ( GTK_MISC(label), 1.0, 0.5 );
    }
    gtk_box_pack_start ( GTK_BOX(vbox), label, FALSE, TRUE, padding );
  }
  return vbox;
}

gboolean a_dialog_yes_or_no ( GtkWindow *parent, const gchar *message, const gchar *extra )
{
  GtkWidget *dia;
  dia = gtk_message_dialog_new ( parent,
                                 GTK_DIALOG_DESTROY_WITH_PARENT,
                                 GTK_MESSAGE_QUESTION,
                                 GTK_BUTTONS_YES_NO,
                                 message, extra );

  if ( gtk_dialog_run ( GTK_DIALOG(dia) ) == GTK_RESPONSE_YES )
  {
    gtk_widget_destroy ( dia );
    return TRUE;
  }
  else
  {
    gtk_widget_destroy ( dia );
    return FALSE;
  }
}

/**
 * a_dialog_yes_or_no_suppress:
 *
 * If the *message is actively being suppressed then this will return TRUE.
 *
 * Allow the user to select when such messages are no longer desired,
 *  primarily aimed at disabing warning messages, which once you have seen them a few times,
 *  you might decide they are annoying.
 *
 * ATM this assumes all of the text in the *message is fixed,
 *  such that it can be matched to previous messages.
 * Any variable text such as layer or track names should be in the *extra component
 */
gboolean a_dialog_yes_or_no_suppress ( GtkWindow *parent, const gchar *message, const gchar *extra )
{
  // See if message has been seen before and desired to be ignored
  if ( a_settings_get_string_list_contains(VIK_SUPPRESS_MESSAGES, message) ) {
    return TRUE;
  }
  GtkWidget *dialog = gtk_dialog_new_with_buttons ( _("Question"),
                                                    parent,
                                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    GTK_STOCK_NO, GTK_RESPONSE_REJECT,
                                                    GTK_STOCK_YES, GTK_RESPONSE_YES,
                                                    NULL );

  gtk_dialog_set_default_response ( GTK_DIALOG(dialog), GTK_RESPONSE_REJECT );
  GtkWidget *response_w = gtk_dialog_get_widget_for_response ( GTK_DIALOG(dialog), GTK_RESPONSE_REJECT );
  if ( response_w )
    gtk_widget_grab_focus ( response_w );

  GtkWidget *hbox = gtk_hbox_new ( FALSE, 0 );
  GtkWidget *ckb = gtk_check_button_new ();
  GtkWidget *lab = gtk_label_new ( _("Do not show this message again" ) );
  gtk_widget_set_tooltip_text ( GTK_WIDGET(lab), _("\"Yes\" will be applied automatically in the future") );

  gtk_box_pack_start ( GTK_BOX(hbox), ckb, FALSE, FALSE, 2 );
  gtk_box_pack_start ( GTK_BOX(hbox), lab, FALSE, FALSE, 2 );

  GtkWidget *ca = gtk_dialog_get_content_area ( GTK_DIALOG(dialog) );
  gchar *msg = g_strdup_printf ( message, extra );
  gtk_box_pack_start ( GTK_BOX(ca), gtk_label_new(msg), TRUE, TRUE, 10 );
  gtk_box_pack_start ( GTK_BOX(ca), hbox, TRUE, TRUE, 0 );
  g_free ( msg );
  gtk_widget_show_all ( dialog );

  gboolean ans = FALSE;
  if ( gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES ) {
    ans = TRUE;
    // Only apply chk button if explicitly saying yes.
    if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ckb)) ) {
      a_settings_set_string_list_containing ( VIK_SUPPRESS_MESSAGES, message );
    }
  }
  gtk_widget_destroy ( dialog );
  return ans;
}

static void zoom_spin_changed ( GtkSpinButton *spin, GtkWidget *pass_along[3] )
{
  if ( gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON(pass_along[2]) ) )
    gtk_spin_button_set_value ( 
        GTK_SPIN_BUTTON(pass_along[GTK_WIDGET(spin) == pass_along[0] ? 1 : 0]),
        gtk_spin_button_get_value ( spin ) );
}

gboolean a_dialog_custom_zoom ( GtkWindow *parent, gdouble *xmpp, gdouble *ympp )
{
  GtkWidget *dialog = gtk_dialog_new_with_buttons (_("Zoom Factors..."),
                                                  parent,
                                                  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                  GTK_STOCK_CANCEL,
                                                  GTK_RESPONSE_REJECT,
                                                  GTK_STOCK_OK,
                                                  GTK_RESPONSE_ACCEPT,
                                                  NULL);
  GtkWidget *table, *label, *xlabel, *xspin, *ylabel, *yspin, *samecheck;
  GtkWidget *pass_along[3];

  table = gtk_table_new ( 4, 2, FALSE );
  gtk_box_pack_start ( GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), table, TRUE, TRUE, 0 );

  label = gtk_label_new ( _("Zoom factor (in meters per pixel):") );
  xlabel = gtk_label_new ( _("X (easting): "));
  ylabel = gtk_label_new ( _("Y (northing): "));

  pass_along[0] = xspin = gtk_spin_button_new ( (GtkAdjustment *) gtk_adjustment_new ( *xmpp, VIK_VIEWPORT_MIN_ZOOM, VIK_VIEWPORT_MAX_ZOOM, 1, 5, 0 ), 1, 8 );
  pass_along[1] = yspin = gtk_spin_button_new ( (GtkAdjustment *) gtk_adjustment_new ( *ympp, VIK_VIEWPORT_MIN_ZOOM, VIK_VIEWPORT_MAX_ZOOM, 1, 5, 0 ), 1, 8 );

  pass_along[2] = samecheck = gtk_check_button_new_with_label ( _("X and Y zoom factors must be equal") );
  /* TODO -- same factor */
  /*  samecheck = gtk_check_button_new_with_label ( "Same x/y zoom factor" ); */

  if ( *xmpp == *ympp )
    gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON(samecheck), TRUE );

  gtk_table_attach_defaults ( GTK_TABLE(table), label, 0, 2, 0, 1 );
  gtk_table_attach_defaults ( GTK_TABLE(table), xlabel, 0, 1, 1, 2 );
  gtk_table_attach_defaults ( GTK_TABLE(table), xspin, 1, 2, 1, 2 );
  gtk_table_attach_defaults ( GTK_TABLE(table), ylabel, 0, 1, 2, 3 );
  gtk_table_attach_defaults ( GTK_TABLE(table), yspin, 1, 2, 2, 3 );
  gtk_table_attach_defaults ( GTK_TABLE(table), samecheck, 0, 2, 3, 4 );

  gtk_widget_show_all ( table );

  g_signal_connect ( G_OBJECT(xspin), "value-changed", G_CALLBACK(zoom_spin_changed), pass_along );
  g_signal_connect ( G_OBJECT(yspin), "value-changed", G_CALLBACK(zoom_spin_changed), pass_along );

  gtk_dialog_set_default_response ( GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT );

  if ( gtk_dialog_run ( GTK_DIALOG(dialog) ) == GTK_RESPONSE_ACCEPT )
  {
    *xmpp = gtk_spin_button_get_value ( GTK_SPIN_BUTTON(xspin) );
    *ympp = gtk_spin_button_get_value ( GTK_SPIN_BUTTON(yspin) );
    gtk_widget_destroy ( dialog );
    return TRUE;
  }
  gtk_widget_destroy ( dialog );
  return FALSE;
}

static void split_spin_focused ( GtkSpinButton *spin, GtkWidget *pass_along[1] )
{
  gtk_toggle_button_set_active    (GTK_TOGGLE_BUTTON(pass_along[0]), 1);
}

static void time_1m_clicked_cb ( GtkButton *button, GtkWidget *spin )
{
  gtk_spin_button_set_value ( GTK_SPIN_BUTTON(spin), 1.0 );
}

static void time_1h_clicked_cb ( GtkButton *button, GtkWidget *spin )
{
  gtk_spin_button_set_value ( GTK_SPIN_BUTTON(spin), 60.0 );
}

static void time_6h_clicked_cb ( GtkButton *button, GtkWidget *spin )
{
  gtk_spin_button_set_value ( GTK_SPIN_BUTTON(spin), 6*60.0 );
}

static void time_1d_clicked_cb ( GtkButton *button, GtkWidget *spin )
{
  gtk_spin_button_set_value ( GTK_SPIN_BUTTON(spin), 24*60.0 );
}

gboolean a_dialog_time_threshold ( GtkWindow *parent, gchar *title_text, gchar *label_text, guint *thr )
{
  GtkWidget *dialog = gtk_dialog_new_with_buttons (title_text, 
                                                  parent,
                                                  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                  GTK_STOCK_CANCEL,
                                                  GTK_RESPONSE_REJECT,
                                                  GTK_STOCK_OK,
                                                  GTK_RESPONSE_ACCEPT,
                                                  NULL);
  GtkWidget *table, *t1, *t2, *t3, *t4, *t5, *spin, *label;
  GtkWidget *pass_along[1];

  table = gtk_table_new ( 5, 2, FALSE );
  gtk_box_pack_start ( GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), table, TRUE, TRUE, 0 );

  label = gtk_label_new (label_text);

  t1 = gtk_radio_button_new_with_label ( NULL, _("1 min") );
  t2 = gtk_radio_button_new_with_label_from_widget ( GTK_RADIO_BUTTON(t1), _("1 hour") );
  t3 = gtk_radio_button_new_with_label_from_widget ( GTK_RADIO_BUTTON(t2), _("6 hours") );
  t4 = gtk_radio_button_new_with_label_from_widget ( GTK_RADIO_BUTTON(t3), _("1 day") );
  t5 = gtk_radio_button_new_with_label_from_widget ( GTK_RADIO_BUTTON(t4), _("Custom (in minutes):") );

  pass_along[0] = t5;

  spin = gtk_spin_button_new ( (GtkAdjustment *) gtk_adjustment_new ( *thr, 0, 65536, 1, 5, 0 ), 1, 0 );

  gtk_table_attach_defaults ( GTK_TABLE(table), label, 0, 2, 0, 1 );
  gtk_table_attach_defaults ( GTK_TABLE(table), t1, 0, 1, 1, 2 );
  gtk_table_attach_defaults ( GTK_TABLE(table), t2, 0, 1, 2, 3 );
  gtk_table_attach_defaults ( GTK_TABLE(table), t3, 0, 1, 3, 4 );
  gtk_table_attach_defaults ( GTK_TABLE(table), t4, 0, 1, 4, 5 );
  gtk_table_attach_defaults ( GTK_TABLE(table), t5, 0, 1, 5, 6 );
  gtk_table_attach_defaults ( GTK_TABLE(table), spin, 1, 2, 5, 6 );

  gtk_widget_show_all ( table );

  g_signal_connect ( G_OBJECT(spin), "grab-focus", G_CALLBACK(split_spin_focused), pass_along );

  g_signal_connect ( G_OBJECT(t1), "clicked", G_CALLBACK(time_1m_clicked_cb), spin );
  g_signal_connect ( G_OBJECT(t2), "clicked", G_CALLBACK(time_1h_clicked_cb), spin );
  g_signal_connect ( G_OBJECT(t3), "clicked", G_CALLBACK(time_6h_clicked_cb), spin );
  g_signal_connect ( G_OBJECT(t4), "clicked", G_CALLBACK(time_1d_clicked_cb), spin );

  gtk_dialog_set_default_response ( GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT );

  if ( gtk_dialog_run ( GTK_DIALOG(dialog) ) == GTK_RESPONSE_ACCEPT )
  {
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(t1))) {
      *thr = 1;
    } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(t2))) {
      *thr = 60;
    } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(t3))) {
      *thr = 60 * 6;
    } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(t4))) {
      *thr = 60 * 24;
    } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(t5))) {
      *thr = gtk_spin_button_get_value ( GTK_SPIN_BUTTON(spin) );
    }
    gtk_widget_destroy ( dialog );
    return TRUE;
  }
  gtk_widget_destroy ( dialog );
  return FALSE;
}

/**
 * a_dialog_get_positive_number:
 *
 * Dialog to return a positive number via a spinbox within the supplied limits
 *
 * Returns: A value of zero indicates the dialog was cancelled
 */
guint a_dialog_get_positive_number ( GtkWindow *parent, gchar *title_text, gchar *label_text, guint default_num, guint min, guint max, guint step )
{
  GtkWidget *dialog = gtk_dialog_new_with_buttons (title_text,
						   parent,
						   GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						   GTK_STOCK_CANCEL,
						   GTK_RESPONSE_REJECT,
						   GTK_STOCK_OK,
						   GTK_RESPONSE_ACCEPT,
						   NULL);
  gtk_dialog_set_default_response ( GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT );
  GtkWidget *response_w = gtk_dialog_get_widget_for_response ( GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT );

  GtkWidget *table, *spin, *label;
  guint result = default_num;

  table = gtk_table_new ( 2, 1, FALSE );
  gtk_box_pack_start ( GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), table, TRUE, TRUE, 0 );

  label = gtk_label_new (label_text);
  spin = gtk_spin_button_new ( (GtkAdjustment *) gtk_adjustment_new ( default_num, min, max, step, 5, 0 ), 1, 0 );

  gtk_table_attach_defaults ( GTK_TABLE(table), label, 0, 1, 0, 1 );
  gtk_table_attach_defaults ( GTK_TABLE(table), spin, 0, 1, 1, 2 );

  if ( response_w )
    gtk_widget_grab_focus ( response_w );

  gtk_widget_show_all ( table );

  if ( gtk_dialog_run ( GTK_DIALOG(dialog) ) == GTK_RESPONSE_ACCEPT )
  {
    result = gtk_spin_button_get_value ( GTK_SPIN_BUTTON(spin) );
    gtk_widget_destroy ( dialog );
    return result;
  }

  // Dialog cancelled
  gtk_widget_destroy ( dialog );
  return 0;
}

/**
 * a_dialog_get_non_zero_number:
 *
 * Dialog to return a non zero number via a spinbox within the supplied limits
 *
 * Returns: A value of zero indicates the dialog was cancelled
 */
gint a_dialog_get_non_zero_number ( GtkWindow *parent, gchar *title_text, gchar *label_text, gint default_num, gint min, gint max, guint step )
{
  GtkWidget *dialog = gtk_dialog_new_with_buttons (title_text,
						   parent,
						   GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						   GTK_STOCK_CANCEL,
						   GTK_RESPONSE_REJECT,
						   GTK_STOCK_OK,
						   GTK_RESPONSE_ACCEPT,
						   NULL);
  gtk_dialog_set_default_response ( GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT );
  GtkWidget *response_w = gtk_dialog_get_widget_for_response ( GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT );

  GtkWidget *table, *spin, *label;
  gint result = default_num;

  table = gtk_table_new ( 2, 1, FALSE );
  gtk_box_pack_start ( GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), table, TRUE, TRUE, 0 );

  label = gtk_label_new (label_text);
  spin = gtk_spin_button_new ( (GtkAdjustment *) gtk_adjustment_new ( default_num, min, max, step, 5, 0 ), 1, 0 );

  gtk_table_attach_defaults ( GTK_TABLE(table), label, 0, 1, 0, 1 );
  gtk_table_attach_defaults ( GTK_TABLE(table), spin, 0, 1, 1, 2 );

  if ( response_w )
    gtk_widget_grab_focus ( response_w );

  gtk_widget_show_all ( table );

  if ( gtk_dialog_run ( GTK_DIALOG(dialog) ) == GTK_RESPONSE_ACCEPT )
  {
    result = gtk_spin_button_get_value ( GTK_SPIN_BUTTON(spin) );
    gtk_widget_destroy ( dialog );
    return result;
  }

  // Dialog cancelled
  gtk_widget_destroy ( dialog );
  return 0;
}

#if (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION < 24)
static void about_url_hook (GtkAboutDialog *about,
                            const gchar    *link,
                            gpointer        data)
{
  open_url (GTK_WINDOW(about), link);
}

static void about_email_hook (GtkAboutDialog *about,
                              const gchar    *email,
                              gpointer        data)
{
  new_email (GTK_WINDOW(about), email);
}
#endif

/**
 *  Creates a dialog with list of text
 *  Mostly useful for longer messages that have several lines of information.
 */
void a_dialog_list ( GtkWindow *parent, const gchar *title, GArray *array, gint padding )
{
  GtkWidget *dialog = gtk_dialog_new_with_buttons ( title,
                                                    parent,
                                                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    GTK_STOCK_CLOSE,
                                                    GTK_RESPONSE_CLOSE,
                                                    NULL);

  GtkBox *vbox = GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog)));
  GtkWidget *label;

  for ( int i = 0; i < array->len; i++ ) {
    label = ui_label_new_selectable (NULL);
    gtk_label_set_markup ( GTK_LABEL(label), g_array_index(array,gchar*,i) );
    gtk_box_pack_start ( GTK_BOX(vbox), label, FALSE, TRUE, padding );
  }

  gtk_widget_show_all ( dialog );
  gtk_dialog_run ( GTK_DIALOG(dialog) );
  gtk_widget_destroy ( dialog );
}

void a_dialog_about ( GtkWindow *parent )
{
  const gchar *version = VIKING_VERSION;
  const gchar *website = VIKING_URL;
  gchar *copyright = g_strdup_printf(_("2003-2008, Evan Battaglia\n2008-%s, Viking's contributors"), THEYEAR);
  const gchar *comments = _("GPS Data and Topo Analyzer, Explorer, and Manager.");
  const gchar *license = _("This program is free software; you can redistribute it and/or modify "
			"it under the terms of the GNU General Public License as published by "
			"the Free Software Foundation; either version 2 of the License, or "
			"(at your option) any later version."
			"\n\n"
			"This program is distributed in the hope that it will be useful, "
			"but WITHOUT ANY WARRANTY; without even the implied warranty of "
			"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
			"GNU General Public License for more details."
			"\n\n"
			"You should have received a copy of the GNU General Public License "
			"along with this program; if not, write to the Free Software "
			"Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA");

  // Would be nice to use gtk_about_dialog_add_credit_section (), but that requires gtk 3.4
  // For now shove it in the 'artists' section so at least the information is easily visible
  const gchar *libs[] = {
    "Compiled in libraries:",
    // Default libs
    "libglib-2.0",
    "libgthread-2.0",
    "libgtk+-2.0",
    "libgio-2.0",
    // Potentially optional libs (but probably couldn't build without them)
#ifdef HAVE_LIBM
    "libm",
#endif
#ifdef HAVE_LIBZ
    "libz",
#endif
#ifdef HAVE_LIBCURL
    "libcurl",
#endif
#ifdef HAVE_EXPAT_H
    "libexpat",
#endif
    // Actually optional libs
#ifdef HAVE_LIBGPS
    "libgps",
#endif
#ifdef HAVE_LIBGEXIV2
    "libgexiv2",
#endif
#ifdef HAVE_LIBEXIF
    "libexif",
#endif
#ifdef HAVE_LIBX11
    "libX11",
#endif
#ifdef HAVE_LIBMAGIC
    "libmagic",
#endif
#ifdef HAVE_LIBBZ2
    "libbz2",
#endif
#ifdef HAVE_LIBZIP
    "libzip",
#endif
#ifdef HAVE_LIBSQLITE3
    "libsqlite3",
#endif
#ifdef HAVE_LIBMAPNIK
    "libmapnik",
#endif
#ifdef HAVE_LIBNETTLE
    "libnettle",
#endif
#ifdef HAVE_LIBGEOCLUE_2
    "libgeoclue2",
#endif
#ifdef HAVE_LIBOAUTH
    "liboauth",
#endif
#ifdef HAVE_LIBNOVA
    "libnova",
#endif
    NULL
  };
  // Newer versions of GTK 'just work', calling gtk_show_uri() on the URL or email and opens up the appropriate program
  // This is the old method:
#if (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION < 24)
  gtk_about_dialog_set_url_hook (about_url_hook, NULL, NULL);
  gtk_about_dialog_set_email_hook (about_email_hook, NULL, NULL);
#endif

  gtk_show_about_dialog (parent,
	"version", version,
	"website", website,
	"comments", comments,
	"copyright", copyright,
	"license", license,
	"wrap-license", TRUE,
	/* logo automatically retrieved via gtk_window_get_default_icon_list */
	"authors", AUTHORS,
	"documenters", DOCUMENTERS,
	"translator-credits", _("Translation is coordinated on http://launchpad.net/viking"),
	"artists", libs,
	NULL);
  g_free(copyright);
}

gboolean a_dialog_map_n_zoom(GtkWindow *parent, gchar *mapnames[], gint default_map, gchar *zoom_list[], gint default_zoom, gint *selected_map, gint *selected_zoom)
{
  gchar **s;

  GtkWidget *dialog = gtk_dialog_new_with_buttons ( _("Download along track"), parent,
                                                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL );
  gtk_dialog_set_default_response ( GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT );
  GtkWidget *response_w = gtk_dialog_get_widget_for_response ( GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT );

  GtkWidget *map_label = gtk_label_new(_("Map type:"));
  GtkWidget *map_combo = vik_combo_box_text_new();
  for (s = mapnames; *s; s++)
    vik_combo_box_text_append (GTK_COMBO_BOX(map_combo), *s);
  gtk_combo_box_set_active (GTK_COMBO_BOX(map_combo), default_map);

  GtkWidget *zoom_label = gtk_label_new(_("Zoom level:"));
  GtkWidget *zoom_combo = vik_combo_box_text_new();
  for (s = zoom_list; *s; s++)
    vik_combo_box_text_append (GTK_COMBO_BOX(zoom_combo), *s);
  gtk_combo_box_set_active (GTK_COMBO_BOX(zoom_combo), default_zoom);

  GtkTable *box = GTK_TABLE(gtk_table_new(2, 2, FALSE));
  gtk_table_attach_defaults(box, map_label, 0, 1, 0, 1);
  gtk_table_attach_defaults(box, map_combo, 1, 2, 0, 1);
  gtk_table_attach_defaults(box, zoom_label, 0, 1, 1, 2);
  gtk_table_attach_defaults(box, zoom_combo, 1, 2, 1, 2);

  gtk_box_pack_start ( GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), GTK_WIDGET(box), FALSE, FALSE, 5 );

  if ( response_w )
    gtk_widget_grab_focus ( response_w );

  gtk_widget_show_all ( dialog );
  if ( gtk_dialog_run ( GTK_DIALOG(dialog) ) != GTK_RESPONSE_ACCEPT ) {
    gtk_widget_destroy(dialog);
    return FALSE;
  }

  *selected_map = gtk_combo_box_get_active(GTK_COMBO_BOX(map_combo));
  *selected_zoom = gtk_combo_box_get_active(GTK_COMBO_BOX(zoom_combo));

  gtk_widget_destroy(dialog);
  return TRUE;
}

/**
 * Display a dialog presenting the license of a map.
 * Allow to read the license by launching a web browser.
 */
void a_dialog_license ( GtkWindow *parent, const gchar *map, const gchar *license, const gchar *url)
{
  GtkWidget *dialog = gtk_message_dialog_new (parent,
                                 GTK_DIALOG_DESTROY_WITH_PARENT,
                                 GTK_MESSAGE_INFO,
                                 GTK_BUTTONS_OK,
                                 _("The map data is licensed: %s."),
                                 license);
  gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (dialog),
    _("The data provided by '<b>%s</b>' are licensed under the following license: <b>%s</b>."),
    map, license);
#define RESPONSE_OPEN_LICENSE 600
  if (url != NULL) {
    gtk_dialog_add_button (GTK_DIALOG (dialog), _("Open license"), RESPONSE_OPEN_LICENSE);
  }
  gint response;
  do {
    response = gtk_dialog_run (GTK_DIALOG (dialog));
    if (response == RESPONSE_OPEN_LICENSE) {
      open_url (parent, url);
    }
  } while (response != GTK_RESPONSE_DELETE_EVENT && response != GTK_RESPONSE_OK);
  gtk_widget_destroy (dialog);
}

/**
 * Mainly for debug
 * Run Time or Build information that can be easily extracted from the libraries in use
 */
#ifdef HAVE_EXPAT_H
#include <expat.h>
#endif
#ifdef HAVE_LIBCURL
#include <curl/curl.h>
#endif
#ifdef HAVE_GPS_H
#include <gps.h>
#endif
#ifdef HAVE_LIBGEXIV2
#include <gexiv2/gexiv2.h>
#endif
#ifdef HAVE_MAGIC_H
#include <magic.h>
#endif
#ifdef HAVE_BZLIB_H
#include <bzlib.h>
#endif
#ifdef HAVE_ZIP_H
#include <zip.h>
#endif
#ifdef HAVE_LZMA_H
#include <lzma.h>
#endif
#ifdef HAVE_SQLITE3_H
#include "sqlite3.h"
#endif
#ifdef HAVE_LIBMAPNIK
#include <mapnik/version.hpp>
#endif
#ifdef HAVE_OAUTH_H
#include <oauth.h>
#endif
#ifdef HAVE_LIBNOVA_LIBNOVA_H
#include <libnova/libnova.h>
#endif

#include <json-glib/json-glib.h>

void a_dialog_build_info ( GtkWindow *parent )
{
  GString *msg = g_string_new ( "" );
  g_string_append_printf ( msg, "GLIB version: %d.%d.%d\n", glib_major_version, glib_minor_version, glib_micro_version  );
#if GTK_CHECK_VERSION(3,0,0)
  g_string_append_printf ( msg, "GTK version: %d.%d.%d\n", gtk_get_major_version(), gtk_get_minor_version(), gtk_get_micro_version() );
#else
  g_string_append_printf ( msg, "GTK API version: %d.%d.%d\n", GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION );
#endif
  g_string_append_printf ( msg, "GDK Pixbuf version: %s\n", gdk_pixbuf_version );
  g_string_append_printf ( msg, "Cairo version: %s\n", cairo_version_string() );
  g_string_append_printf ( msg, "Pango version: %s\n", pango_version_string() );
#ifdef HAVE_EXPAT_H
  g_string_append_printf ( msg, "Expat version: %s\n", XML_ExpatVersion()  );
#endif
#ifdef HAVE_LIBCURL
  g_string_append_printf ( msg, "libcurl version: %s\n", curl_version() );
#endif
#ifdef HAVE_GPS_H
  g_string_append_printf ( msg, "GPSD API version: %d.%d\n", GPSD_API_MAJOR_VERSION, GPSD_API_MINOR_VERSION );
#endif
#ifdef HAVE_LIBGEXIV2
  g_string_append_printf ( msg, "gexiv2 version: %06d\n", gexiv2_get_version() );
#endif
#ifdef HAVE_MAGIC_H
#ifdef MAGIC_VERSION
  g_string_append_printf ( msg, "magic API version: %d\n", MAGIC_VERSION );
#endif
#endif
#ifdef HAVE_BZLIB_H
  g_string_append_printf ( msg, "bzip2 version: %s\n", BZ2_bzlibVersion() );
#endif
#ifdef HAVE_ZIP_H
#ifdef LIBZIP_VERSION
  g_string_append_printf ( msg, "libzip version: %s\n", LIBZIP_VERSION );
#endif
#endif
#ifdef HAVE_LZMA_H
#ifdef LZMA_VERSION_STRING
  g_string_append_printf ( msg, "liblzma version: %s\n", LZMA_VERSION_STRING );
#endif
#endif
#ifdef HAVE_SQLITE3_H
#ifdef SQLITE_VERSION
  g_string_append_printf ( msg, "libsqlite version: %s\n", SQLITE_VERSION );
#endif
#endif
#ifdef HAVE_LIBMAPNIK
#ifdef MAPNIK_VERSION_STRING
  g_string_append_printf ( msg, "mapnik version: %s\n", MAPNIK_VERSION_STRING );
#endif
#endif
#ifdef HAVE_OAUTH_H
#ifdef LIBOAUTH_VERSION
  g_string_append_printf ( msg, "liboauth version: %s\n", LIBOAUTH_VERSION );
#endif
#endif
#ifdef JSON_VERSION_S
  g_string_append_printf ( msg, "json-glib version: %s\n", JSON_VERSION_S );
#endif
#ifdef HAVE_LIBNOVA_LIBNOVA_H
  g_string_append_printf ( msg, "libnova version: %s\n", ln_get_version() );
#endif

  // https://docs.flatpak.org/en/latest/flatpak-command-reference.html#flatpak-metadata
  if ( g_file_test("/.flatpak-info", G_FILE_TEST_EXISTS) ) {
    g_string_append ( msg, "\nRunning inside Flatpak sandbox\n" );
    gchar *appname = NULL;
    gchar *runtime = NULL;
    GKeyFile *kf = g_key_file_new ();
    g_key_file_load_from_file ( kf, "/.flatpak-info", G_KEY_FILE_NONE, NULL );
    if ( g_key_file_has_group(kf, "Application") ) {
      appname = g_key_file_get_string ( kf, "Application", "name", NULL );
      runtime = g_key_file_get_string ( kf, "Application", "runtime", NULL );
    }
    g_key_file_unref (kf);
    if ( appname || runtime )
      g_string_append_printf ( msg, "Flatpak info: name=%s runtime=%s\n", appname, runtime );
    g_free ( appname );
    g_free ( runtime );
  }

  a_dialog_info_msg ( parent, msg->str );
  g_string_free ( msg, TRUE );
}

#include "NEWS.h"
/**
 * News will only be available in English
 * (although the dialog title may be translated)
 *
 * ATM just a plain text dump of the current NEWS file.
 * Potential in the future to create/use an HTML version (or MarkDown),
 *  perhaps via webkit2gtk.
 *
 */
void a_dialog_news ( GtkWindow *parent )
{
  // Ensure NULL terminator added
  gchar *msg = { (gchar*)NEWS };
  msg[NEWS_len] = '\0';

  // Tailor a specific dialog, rather than simply using a_dialog_msg()
  //  as the text is rather long, thus want a scrollable window.
  GtkWidget *dialog = gtk_dialog_new_with_buttons ( _("News"),
                                                    parent,
                                                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                                                    NULL );
  GtkWidget *ln = ui_label_new_selectable ( msg );
  GtkWidget *sw = gtk_scrolled_window_new ( NULL, NULL );
  gtk_scrolled_window_set_policy ( GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );
  // Ensure it doesn't start off too small
  // Could pass in width+height - but we'll just use the default scale factor here
  gdouble scale = vik_viewport_get_scale ( NULL );
  gtk_widget_set_size_request ( sw, 480*scale, 640*scale );
  gtk_container_add ( GTK_CONTAINER(sw), ln );

  GtkBox *vbox = GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog)));
  gtk_box_pack_start ( vbox, sw, TRUE, TRUE, 0 );
  gtk_widget_show_all ( dialog );

  gtk_dialog_run ( GTK_DIALOG(dialog) );
  gtk_widget_destroy ( dialog );
}
