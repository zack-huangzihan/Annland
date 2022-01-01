/*
 * Copyright (C) 2013 Collabora Ltd.
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
 *
 * Author: Emilio Pozuelo Monfort <emilio.pozuelo@collabora.co.uk>
 */

#include "config.h"

#include "favorites.h"
#include "shell-app-system.h"

#include <gio/gio.h>
#include <gtk/gtk.h>

#include "app-icon.h"

enum {
  APP_LAUNCHED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

struct MaynardFavoritesPrivate {
  GSettings *settings;
};

G_DEFINE_TYPE_WITH_PRIVATE(MaynardFavorites, maynard_favorites, GTK_TYPE_BOX)

static void
favorite_clicked (GtkButton *button,
    MaynardFavorites *self)
{
  ShellAppInfo *info = g_object_get_data (G_OBJECT(button), "info");

  shell_app_info_launch (info);
  g_signal_emit (self, signals[APP_LAUNCHED], 0);
}

static void
add_favorite (MaynardFavorites *self,
    const gchar *favorite)
{
  ShellAppInfo *info;
  GtkWidget *button;
  GIcon *icon;

  info = shell_app_system_get_app_info (shell_app_system_get_default(), favorite);

  if (!info)
    return;

  icon = shell_app_info_get_icon (info);

  button = maynard_app_icon_new_from_gicon (icon);

  g_object_set_data_full (G_OBJECT (button), "info", info, g_object_unref);

  g_signal_connect (button, "clicked", G_CALLBACK (favorite_clicked), self);

  gtk_box_pack_end (GTK_BOX (self), button, FALSE, FALSE, 0);
}

static void
remove_favorite (GtkWidget *favorite,
    gpointer user_data)
{
  gtk_widget_destroy (favorite);
}

static void
favorites_changed (GSettings *settings,
    const gchar *key,
    MaynardFavorites *self)
{
  gchar **favorites = g_settings_get_strv (settings, key);
  guint i;

  /* Remove all favorites first */
  gtk_container_foreach (GTK_CONTAINER (self), remove_favorite, NULL);

  for (i = 0; i < g_strv_length (favorites); i++)
    {
      gchar *fav = favorites[i];

      add_favorite (self, fav);
    }

  g_strfreev (favorites);
}

static void
maynard_favorites_dispose (GObject *object)
{
  MaynardFavorites *self = MAYNARD_FAVORITES (object);

  g_clear_object (&self->priv->settings);

  G_OBJECT_CLASS (maynard_favorites_parent_class)->dispose (object);
}

static void
maynard_favorites_init (MaynardFavorites *self)
{
  self->priv = maynard_favorites_get_instance_private (self);

  self->priv->settings = g_settings_new ("org.maynard");
  g_signal_connect (self->priv->settings, "changed::favorites",
                    G_CALLBACK (favorites_changed), self);
  favorites_changed (self->priv->settings, "favorites", self);

  gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_HORIZONTAL);
}

static void
maynard_favorites_class_init (MaynardFavoritesClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  object_class->dispose = maynard_favorites_dispose;

  signals[APP_LAUNCHED] = g_signal_new ("app-launched",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);
}

GtkWidget *
maynard_favorites_new (void)
{
  return g_object_new (MAYNARD_TYPE_FAVORITES,
      "orientation", GTK_ORIENTATION_VERTICAL,
      NULL);
}
