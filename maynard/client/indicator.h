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

#ifndef __MAYNARD_INDICATOR_H__
#define __MAYNARD_INDICATOR_H__

#include <gtk/gtk.h>

#define MAYNARD_INDICATOR_TYPE                 (maynard_indicator_get_type ())
#define MAYNARD_INDICATOR(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAYNARD_INDICATOR_TYPE, MaynardIndicator))
#define MAYNARD_INDICATOR_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), MAYNARD_INDICATOR_TYPE, MaynardIndicatorClass))
#define MAYNARD_IS_INDICATOR(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MAYNARD_INDICATOR_TYPE))
#define MAYNARD_IS_INDICATOR_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), MAYNARD_INDICATOR_TYPE))
#define MAYNARD_INDICATOR_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), MAYNARD_INDICATOR_TYPE, MaynardIndicatorClass))

typedef struct _MaynardIndicator MaynardIndicator;
typedef struct _MaynardIndicatorClass MaynardIndicatorClass;
typedef struct _MaynardIndicatorPrivate MaynardIndicatorPrivate;

struct _MaynardIndicator
{
  GtkBin parent;

  MaynardIndicatorPrivate *priv;
};

struct _MaynardIndicatorClass
{
  GtkBinClass parent_class;
};

GType maynard_indicator_get_type (void) G_GNUC_CONST;

void maynard_indicator_set_widget (MaynardIndicator *self,
    GtkWidget *widget);
GtkWidget *maynard_indicator_get_widget (MaynardIndicator *self);

void maynard_indicator_set_menu (MaynardIndicator *self,
    GtkWidget *menu);
GtkWidget *maynard_indicator_get_menu (MaynardIndicator *self);

#endif /* __MAYNARD_INDICATOR_H__ */
