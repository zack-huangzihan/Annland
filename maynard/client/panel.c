/*
 * Copyright (C) 2014, 2019 Collabora Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include "config.h"

#include "panel.h"

#include "app-icon.h"
#include "favorites.h"
#include "clock-indicator.h"
#include "volume-indicator.h"

enum {
  INDICATORS_MENU_TOGGLED,
  APP_MENU_TOGGLED,
  FAVORITE_LAUNCHED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

struct MaynardPanelPrivate {
  GList *indicators;
  GtkWidget *indicators_menu;
};

G_DEFINE_TYPE_WITH_PRIVATE(MaynardPanel, maynard_panel, GTK_TYPE_WINDOW)

static void
maynard_panel_init (MaynardPanel *self)
{
  self->priv = maynard_panel_get_instance_private (self);
}

static gboolean
widget_enter_notify_event_cb (GtkWidget *widget,
    GdkEventCrossing *event,
    MaynardPanel *self)
{
  gboolean handled;
  g_signal_emit_by_name (self, "enter-notify-event", event, &handled);
  return handled;
}

static void
indicator_set_state_flags (MaynardIndicator *indicator,
    gpointer user_data)
{
  GtkWidget *indicator_widget = maynard_indicator_get_widget (indicator);
  GtkStateFlags flags = GPOINTER_TO_UINT (user_data);

  if (indicator_widget)
    gtk_widget_set_state_flags (indicator_widget, flags, FALSE);
}

static void
indicator_unset_state_flags (MaynardIndicator *indicator,
    gpointer user_data)
{
  GtkWidget *indicator_widget = maynard_indicator_get_widget (indicator);
  GtkStateFlags flags = GPOINTER_TO_UINT (user_data);

  if (indicator_widget)
    gtk_widget_unset_state_flags (indicator_widget, flags);
}

static gboolean
indicators_enter_leave_notify_event_cb (GtkWidget *widget,
    GdkEventCrossing *event,
    MaynardPanel *self)
{
  gboolean handled;

  if (event->type == GDK_ENTER_NOTIFY)
    {
      g_list_foreach (self->priv->indicators,
        (GFunc) indicator_set_state_flags,
        GUINT_TO_POINTER (GTK_STATE_FLAG_PRELIGHT));
      g_signal_emit_by_name (self, "enter-notify-event", event, &handled);
    }
  else
    {
      g_list_foreach (self->priv->indicators,
        (GFunc) indicator_unset_state_flags,
        GUINT_TO_POINTER (GTK_STATE_FLAG_PRELIGHT));
      g_signal_emit_by_name (self, "leave-notify-event", event, &handled);
    }
  return handled;
}

static void
indicators_button_press_cb (GtkWidget *widget,
    GdkEvent *event,
    MaynardPanel *self)
{
  g_signal_emit (self, signals[INDICATORS_MENU_TOGGLED], 0);
}

static void
widget_connect_enter_signal (MaynardPanel *self,
    GtkWidget *widget)
{
  g_signal_connect (widget, "enter-notify-event",
      G_CALLBACK (widget_enter_notify_event_cb), self);
}

static void
app_menu_button_clicked_cb (GtkButton *button,
    MaynardPanel *self)
{
  g_signal_emit (self, signals[APP_MENU_TOGGLED], 0);
}

static void
favorite_launched_cb (MaynardFavorites *favorites,
    MaynardPanel *self)
{
  g_signal_emit (self, signals[FAVORITE_LAUNCHED], 0);
}

static void
maynard_panel_constructed (GObject *object)
{
  MaynardPanel *self = MAYNARD_PANEL (object);
  GtkWidget *main_box;
  GtkWidget *ebox;
  GtkWidget *separator;
  GtkWidget *launcher_button;
  GtkWidget *favorites;
  GtkWidget *indicators_box;
  GtkWidget *volume_indicator;
  GtkWidget *clock_indicator;
  GtkWidget *indicators_menu;
  GtkWidget *indicators_menu_box;

  G_OBJECT_CLASS (maynard_panel_parent_class)->constructed (object);

  /* window properties */
  gtk_window_set_title (GTK_WINDOW (self), "maynard");
  gtk_window_set_decorated (GTK_WINDOW (self), FALSE);
  gtk_widget_realize (GTK_WIDGET (self));

  gtk_style_context_add_class (
      gtk_widget_get_style_context (GTK_WIDGET (self)),
      "maynard-panel");

  /* main vbox */
  main_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_style_context_add_class (
      gtk_widget_get_style_context (GTK_WIDGET (main_box)),
      "maynard-panel-box");
  gtk_container_add (GTK_CONTAINER (self), main_box);

  /* launcher */
  ebox = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (main_box), ebox, FALSE, FALSE, 0);
  launcher_button = maynard_app_icon_new ("view-grid-symbolic");
  gtk_style_context_add_class (
      gtk_widget_get_style_context (launcher_button),
      "maynard-launcher-button");
  gtk_container_add (GTK_CONTAINER (ebox), launcher_button);
  widget_connect_enter_signal (self, ebox);
  g_signal_connect (launcher_button, "clicked",
      G_CALLBACK (app_menu_button_clicked_cb), self);

  /* launcher/favorites separator */
  separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_style_context_add_class (
      gtk_widget_get_style_context (GTK_WIDGET (separator)),
      "maynard-panel-separator");
  gtk_box_pack_start (GTK_BOX (main_box), separator, FALSE, FALSE, 0);

  /* favorites */
  ebox = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (main_box), ebox, FALSE, FALSE, 0);
  favorites = maynard_favorites_new ();
  gtk_container_add (GTK_CONTAINER (ebox), favorites);
  widget_connect_enter_signal (self, ebox);
  g_signal_connect (favorites, "app-launched",
      G_CALLBACK (favorite_launched_cb), self);

  /* indicators */
  ebox = gtk_event_box_new ();
  gtk_box_pack_end (GTK_BOX (main_box), ebox, FALSE, FALSE, 0);

  /* indicators box */
  indicators_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_style_context_add_class (
      gtk_widget_get_style_context (GTK_WIDGET (indicators_box)),
      "maynard-indicators-box");
  gtk_container_add (GTK_CONTAINER (ebox), indicators_box);

  volume_indicator = maynard_volume_indicator_new ();
  gtk_box_pack_start (GTK_BOX (indicators_box), volume_indicator, FALSE, FALSE, 0);
  self->priv->indicators = g_list_append (self->priv->indicators, volume_indicator);

  clock_indicator = maynard_clock_indicator_new ();
  gtk_box_pack_start (GTK_BOX (indicators_box), clock_indicator, FALSE, FALSE, 0);
  self->priv->indicators = g_list_append (self->priv->indicators, clock_indicator);

  g_signal_connect (ebox, "enter-notify-event",
      G_CALLBACK (indicators_enter_leave_notify_event_cb), self);
  g_signal_connect (ebox, "leave-notify-event",
      G_CALLBACK (indicators_enter_leave_notify_event_cb), self);
  g_signal_connect (ebox, "button-press-event",
      G_CALLBACK (indicators_button_press_cb), self);

  /* indicators menu */

  /* TODO: use a widget instead of window and let the compositor create the window
   */
  indicators_menu = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (indicators_menu), "maynard-indicators-menu");
  gtk_window_set_decorated (GTK_WINDOW (indicators_menu), FALSE);
  gtk_widget_realize (indicators_menu);

  indicators_menu_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_style_context_add_class (
      gtk_widget_get_style_context (GTK_WIDGET (indicators_menu_box)),
      "maynard-indicators-menu");
  gtk_container_add (GTK_CONTAINER (indicators_menu), indicators_menu_box);

  gtk_box_pack_start (GTK_BOX (indicators_menu_box),
      maynard_indicator_get_menu (MAYNARD_INDICATOR (clock_indicator)),
      FALSE, FALSE, 0);
  separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_style_context_add_class (
      gtk_widget_get_style_context (GTK_WIDGET (separator)),
      "maynard-indicators-menu-separator");
  gtk_box_pack_start (GTK_BOX (indicators_menu_box),
      separator, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (indicators_menu_box),
      maynard_indicator_get_menu (MAYNARD_INDICATOR (volume_indicator)),
      FALSE, FALSE, 0);
  self->priv->indicators_menu = indicators_menu;
}

static void
maynard_panel_dispose (GObject *object)
{
  G_OBJECT_CLASS (maynard_panel_parent_class)->dispose (object);
}

static void
maynard_panel_class_init (MaynardPanelClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  object_class->constructed = maynard_panel_constructed;
  object_class->dispose = maynard_panel_dispose;

  signals[INDICATORS_MENU_TOGGLED] = g_signal_new ("indicators-menu-toggled",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);

  signals[APP_MENU_TOGGLED] = g_signal_new ("app-menu-toggled",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);

  signals[FAVORITE_LAUNCHED] = g_signal_new ("favorite-launched",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);
}

GtkWidget *
maynard_panel_new (void)
{
  return g_object_new (MAYNARD_PANEL_TYPE, NULL);
}

GtkWidget *
maynard_panel_get_indicators_menu (MaynardPanel *self)
{
  g_return_val_if_fail (MAYNARD_IS_PANEL (self), NULL);

  return self->priv->indicators_menu;
}
