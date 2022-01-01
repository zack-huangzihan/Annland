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

#include "indicator.h"

struct _MaynardIndicatorPrivate {
  GtkWidget *menu;
};

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (MaynardIndicator, maynard_indicator, GTK_TYPE_BIN)

static void maynard_indicator_add (GtkContainer *container,
    GtkWidget *child);
static void maynard_indicator_remove (GtkContainer *container,
    GtkWidget *child);

static void
maynard_indicator_init (MaynardIndicator *self)
{
  self->priv = maynard_indicator_get_instance_private (self);
}

static void
maynard_indicator_add (GtkContainer *container,
    GtkWidget *child)
{
  gtk_style_context_add_class (
      gtk_widget_get_style_context (child),
      "maynard-indicator");

  GTK_CONTAINER_CLASS (maynard_indicator_parent_class)->add (container, child);
}

static void
maynard_indicator_remove (GtkContainer *container,
    GtkWidget *child)
{
  gtk_style_context_remove_class (
      gtk_widget_get_style_context (child),
      "maynard-indicator");

  GTK_CONTAINER_CLASS (maynard_indicator_parent_class)->remove (container, child);
}

static void
maynard_indicator_dispose (GObject *object)
{
  MaynardIndicator *self = MAYNARD_INDICATOR (object);

  g_clear_object (&self->priv->menu);

  G_OBJECT_CLASS (maynard_indicator_parent_class)->dispose (object);
}

static void
maynard_indicator_class_init (MaynardIndicatorClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  GtkContainerClass *container_class = (GtkContainerClass *)klass;

  object_class->dispose = maynard_indicator_dispose;

  container_class->add = maynard_indicator_add;
  container_class->remove = maynard_indicator_remove;
}

/*
 * Convenience function to set the widget for the indicator.
 *
 * It is the same as invoking gtk_container_remove on the previous
 * widget if any and gtk_container_add on the new one if existing.
 */
void
maynard_indicator_set_widget (MaynardIndicator *self,
    GtkWidget *widget)
{
  GtkWidget *cur_widget;

  g_return_if_fail (MAYNARD_IS_INDICATOR (self));
  g_return_if_fail (widget == NULL || GTK_IS_WIDGET (widget));

  cur_widget = maynard_indicator_get_widget (self);
  if (cur_widget)
    gtk_container_remove (GTK_CONTAINER (self),
        cur_widget);
  if (widget)
    gtk_container_add (GTK_CONTAINER (self), widget);
}

/*
 * Convenience function to set the widget for the indicator.
 *
 * It is the same as invoking gtk_bin_get_child.
 *
 * Returns: (nullable) (transfer none): the indicator widget
 */
GtkWidget *
maynard_indicator_get_widget (MaynardIndicator *self)
{
  g_return_val_if_fail (MAYNARD_IS_INDICATOR (self), NULL);

  return gtk_bin_get_child (GTK_BIN (self));
}

void
maynard_indicator_set_menu (MaynardIndicator *self,
    GtkWidget *menu)
{
  g_return_if_fail (MAYNARD_IS_INDICATOR (self));
  g_return_if_fail (menu == NULL || GTK_IS_WIDGET (menu));

  g_clear_object (&self->priv->menu);
  if (menu)
    self->priv->menu = g_object_ref (menu);
}

/**
 * Returns: (nullable) (transfer none): the indicator menu
 */
GtkWidget *
maynard_indicator_get_menu (MaynardIndicator *self)
{
  g_return_val_if_fail (MAYNARD_IS_INDICATOR (self), NULL);

  return self->priv->menu;
}
