/*
 * Copyright (C) 2019 Collabora Ltd.
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

#include "clock-indicator.h"

#define GNOME_DESKTOP_USE_UNSTABLE_API
#include <libgnome-desktop/gnome-wall-clock.h>

struct _MaynardClockIndicatorPrivate {
  GtkWidget *label;
  GtkWidget *menu_label_weekday;
  GtkWidget *menu_label_date;

  GnomeWallClock *wall_clock;
};

G_DEFINE_TYPE_WITH_PRIVATE (MaynardClockIndicator, maynard_clock_indicator, MAYNARD_INDICATOR_TYPE)

static void
maynard_clock_indicator_init (MaynardClockIndicator *self)
{
  self->priv = maynard_clock_indicator_get_instance_private (self);
}

static void
wall_clock_notify_cb (GnomeWallClock *wall_clock,
    GParamSpec *pspec,
    MaynardClockIndicator *self)
{
  GDateTime *datetime;
  gchar *str;

  datetime = g_date_time_new_now_local ();

  str = g_date_time_format (datetime, "%H:%M");
  gtk_label_set_markup (GTK_LABEL (self->priv->label), str);
  g_free (str);

  str = g_date_time_format (datetime, "%A");
  gtk_label_set_text (GTK_LABEL (self->priv->menu_label_weekday), str);
  g_free (str);

  str = g_date_time_format (datetime, "%e %B %Y");
  gtk_label_set_text (GTK_LABEL (self->priv->menu_label_date), str);
  g_free (str);

  g_date_time_unref (datetime);
}

static GtkWidget *
create_clock_menu (MaynardClockIndicator *self)
{
  GtkWidget *box;
  GtkWidget *label_weekday;
  GtkWidget *label_date;

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_style_context_add_class (gtk_widget_get_style_context (box),
      "maynard-clock-indicator-menu-item");

  label_weekday = gtk_label_new ("");
  gtk_label_set_justify (GTK_LABEL (label_weekday), GTK_JUSTIFY_LEFT);
  gtk_label_set_xalign (GTK_LABEL (label_weekday), 0);
  gtk_style_context_add_class (gtk_widget_get_style_context (label_weekday),
      "maynard-clock-indicator-menu-item-weekday");
  gtk_box_pack_start (GTK_BOX (box), label_weekday, FALSE, FALSE, 0);
  self->priv->menu_label_weekday = label_weekday;

  label_date = gtk_label_new ("");
  gtk_label_set_justify (GTK_LABEL (label_date), GTK_JUSTIFY_LEFT);
  gtk_label_set_xalign (GTK_LABEL (label_date), 0);
  gtk_style_context_add_class (gtk_widget_get_style_context (label_date),
      "maynard-clock-indicator-menu-item-date");
  gtk_box_pack_start (GTK_BOX (box), label_date, FALSE, FALSE, 0);
  self->priv->menu_label_date = label_date;

  return box;
}

static void
maynard_clock_indicator_constructed (GObject *object)
{
  MaynardClockIndicator *self = MAYNARD_CLOCK_INDICATOR (object);
  GtkWidget *label, *menu;

  G_OBJECT_CLASS (maynard_clock_indicator_parent_class)->constructed (object);

  gtk_style_context_add_class (
      gtk_widget_get_style_context (GTK_WIDGET (self)),
      "maynard-clock-indicator");

  self->priv->wall_clock = g_object_new (GNOME_TYPE_WALL_CLOCK, NULL);
  g_signal_connect (self->priv->wall_clock, "notify::clock",
      G_CALLBACK (wall_clock_notify_cb), self);

  label = gtk_label_new ("");
  gtk_style_context_add_class (gtk_widget_get_style_context (label),
      "maynard-clock-indicator-label");
  self->priv->label = label;
  maynard_indicator_set_widget (MAYNARD_INDICATOR (self), label);

  /* the menu */
  menu = create_clock_menu (self);
  maynard_indicator_set_menu (MAYNARD_INDICATOR (self), menu);

  wall_clock_notify_cb (self->priv->wall_clock, NULL, self);
}

static void
maynard_clock_indicator_dispose (GObject *object)
{
  MaynardClockIndicator *self = MAYNARD_CLOCK_INDICATOR (object);

  g_clear_object (&self->priv->wall_clock);

  G_OBJECT_CLASS (maynard_clock_indicator_parent_class)->dispose (object);
}

static void
maynard_clock_indicator_class_init (MaynardClockIndicatorClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  object_class->constructed = maynard_clock_indicator_constructed;
  object_class->dispose = maynard_clock_indicator_dispose;
}

GtkWidget *
maynard_clock_indicator_new (void)
{
  return g_object_new (MAYNARD_CLOCK_INDICATOR_TYPE, NULL);
}
