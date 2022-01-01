/*
 * Copyright (C) 2013-2014 Collabora Ltd.
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
 * Authors: Emilio Pozuelo Monfort <emilio.pozuelo@collabora.co.uk>
 *          Jonny Lamb <jonny.lamb@collabora.co.uk>
 */

#include "config.h"

#include "launcher.h"

#include "clock.h"
#include "panel.h"
#include "shell-app-system.h"

enum
{
  ICON_COLUMN = 0,
  NAME_COLUMN,
  INFO_COLUMN,
  N_COLUMNS
};

enum {
  APP_LAUNCHED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

struct MaynardLauncherPrivate {
  ShellAppSystem *app_system;

  GtkWidget *scrolled_window;
  GtkWidget *icon_view;
};

G_DEFINE_TYPE_WITH_PRIVATE(MaynardLauncher, maynard_launcher, GTK_TYPE_WINDOW)

static void
maynard_launcher_init (MaynardLauncher *self)
{
  self->priv = maynard_launcher_get_instance_private (self);
}

static gint
sort_apps (gconstpointer a,
    gconstpointer b)
{
  ShellAppInfo *info1 = (ShellAppInfo*) a;
  ShellAppInfo *info2 = (ShellAppInfo*) b;
  gchar *s1, *s2;
  gint ret;

  s1 = g_utf8_casefold (shell_app_info_get_display_name (info1), -1);
  s2 = g_utf8_casefold (shell_app_info_get_display_name (info2), -1);

  ret = g_strcmp0 (s1, s2);

  g_free (s1);
  g_free (s2);

  return ret;
}

static gboolean
app_launched_idle_cb (gpointer data)
{
  MaynardLauncher *self = data;
  GtkAdjustment *adjustment;

  /* make the scrolled window go back to the top */
  adjustment = gtk_scrolled_window_get_vadjustment (
      GTK_SCROLLED_WINDOW (self->priv->scrolled_window));

  gtk_adjustment_set_value (adjustment, 0.0);

  return G_SOURCE_REMOVE;
}

static void
item_activated_cb (GtkIconView *icon_view,
    GtkTreePath *path,
    gpointer user_data)
{
  MaynardLauncher *self = MAYNARD_LAUNCHER (user_data);
  GtkTreeModel *list_model = gtk_icon_view_get_model (icon_view);
  GtkTreeIter iter;
  ShellAppInfo *info = NULL;

  if (!gtk_tree_model_get_iter (GTK_TREE_MODEL (list_model), &iter, path)) {
      // TODO - warn
      return;
  }

  gtk_tree_model_get (list_model, &iter, INFO_COLUMN, &info, -1);
  if (!info) {
      // TODO - warn
      return;
  }

  shell_app_info_launch (info);

  g_signal_emit (self, signals[APP_LAUNCHED], 0);

  /* do this in an idle so it's not done so obviously onscreen */
  g_idle_add (app_launched_idle_cb, self);
}

static void
installed_changed_cb (ShellAppSystem *app_system,
    MaynardLauncher *self)
{
  GHashTable *entries;
  GList *l, *values;
  GtkListStore *list_model;

  entries = shell_app_system_get_entries (app_system);
  values = g_hash_table_get_values (entries);
  values = g_list_sort (values, sort_apps);

  list_model = GTK_LIST_STORE (gtk_icon_view_get_model (GTK_ICON_VIEW (self->priv->icon_view)));

  /* remove all children first */
  gtk_list_store_clear (list_model);

  /* insert new children */
  for (l = values; l; l = l->next)
    {
      ShellAppInfo *info = (ShellAppInfo*) l->data;
      GIcon *icon;
      GtkIconInfo *icon_info = NULL;
      GdkPixbuf *pixbuf = NULL;
      GtkTreeIter iter;

      icon = shell_app_info_get_icon (info);
      if (icon)
        icon_info = gtk_icon_theme_lookup_by_gicon (gtk_icon_theme_get_default (),
            icon, 96, GTK_ICON_LOOKUP_FORCE_SIZE);
      if (icon_info)
        pixbuf = gtk_icon_info_load_icon (icon_info, NULL);

      gtk_list_store_append (list_model, &iter);
      gtk_list_store_set (list_model, &iter,
              ICON_COLUMN, pixbuf,
              NAME_COLUMN, shell_app_info_get_display_name (info),
              INFO_COLUMN, info,
              -1);
    }
  g_list_free (values);
}

static GtkWidget *
icon_view_create (void)
{
  GtkListStore *list_model;
  GtkWidget *icon_view;
  GtkCellRenderer *text_cell;
  GtkCellRenderer *pixbuf_cell;

  list_model = gtk_list_store_new (N_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_POINTER);
  icon_view = gtk_icon_view_new_with_model (GTK_TREE_MODEL (list_model));
  gtk_icon_view_set_activate_on_single_click (GTK_ICON_VIEW (icon_view), TRUE);
  g_object_set (icon_view,
      "row-spacing", 48,
      "column-spacing", 48,
      NULL);
  gtk_style_context_add_class (
      gtk_widget_get_style_context (GTK_WIDGET (icon_view)),
      "maynard-launcher-icon-view");

  /* icon column */
  pixbuf_cell = gtk_cell_renderer_pixbuf_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (icon_view), pixbuf_cell, FALSE);
  gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (icon_view),
      pixbuf_cell, "pixbuf", ICON_COLUMN);

  /* name column */
  text_cell = gtk_cell_renderer_text_new ();
  g_object_set (G_OBJECT (text_cell),
      "ellipsize-set", TRUE, "ellipsize", PANGO_ELLIPSIZE_END,
      "align-set", TRUE, "alignment", PANGO_ALIGN_CENTER, "xalign", 0.5,
      NULL);
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (icon_view), text_cell, FALSE);
  gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (icon_view),
      text_cell, "text", NAME_COLUMN);

  /* tooltip for name column */
  //gtk_icon_view_set_tooltip_column (GTK_ICON_VIEW (icon_view), NAME_COLUMN);

  return icon_view;
}

static void
maynard_launcher_constructed (GObject *object)
{
  MaynardLauncher *self = MAYNARD_LAUNCHER (object);

  G_OBJECT_CLASS (maynard_launcher_parent_class)->constructed (object);

  /* window properties */
  gtk_window_set_title (GTK_WINDOW (self), "maynard");
  gtk_window_set_decorated (GTK_WINDOW (self), FALSE);
  gtk_widget_realize (GTK_WIDGET (self));

  /* make it black and slightly alpha */
  gtk_style_context_add_class (
      gtk_widget_get_style_context (GTK_WIDGET (self)),
      "maynard-launcher");

  /* scroll it */
  self->priv->scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_style_context_add_class (
      gtk_widget_get_style_context (GTK_WIDGET (self->priv->scrolled_window)),
      "maynard-launcher-scrolled-window");
  g_object_set (self->priv->scrolled_window,
      "margin", 48,
      NULL);
  gtk_container_add (GTK_CONTAINER (self), self->priv->scrolled_window);

  self->priv->icon_view = icon_view_create ();
  g_signal_connect (self->priv->icon_view, "item-activated",
      G_CALLBACK (item_activated_cb), self);
  gtk_container_add (GTK_CONTAINER (self->priv->scrolled_window),
      self->priv->icon_view);

  /* fill the view with apps */
  self->priv->app_system = shell_app_system_get_default ();
  g_signal_connect (self->priv->app_system, "installed-changed",
      G_CALLBACK (installed_changed_cb), self);
  installed_changed_cb (self->priv->app_system, self);
}

static void
maynard_launcher_class_init (MaynardLauncherClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  object_class->constructed = maynard_launcher_constructed;

  signals[APP_LAUNCHED] = g_signal_new ("app-launched",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);
}

GtkWidget *
maynard_launcher_new (GtkWidget *background_widget)
{
  return g_object_new (MAYNARD_LAUNCHER_TYPE,
      NULL);
}
