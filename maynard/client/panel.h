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

#ifndef __MAYNARD_PANEL_H__
#define __MAYNARD_PANEL_H__

#include <gtk/gtk.h>

#define MAYNARD_PANEL_TYPE                 (maynard_panel_get_type ())
#define MAYNARD_PANEL(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAYNARD_PANEL_TYPE, MaynardPanel))
#define MAYNARD_PANEL_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), MAYNARD_PANEL_TYPE, MaynardPanelClass))
#define MAYNARD_IS_PANEL(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MAYNARD_PANEL_TYPE))
#define MAYNARD_IS_PANEL_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), MAYNARD_PANEL_TYPE))
#define MAYNARD_PANEL_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), MAYNARD_PANEL_TYPE, MaynardPanelClass))

typedef struct MaynardPanel MaynardPanel;
typedef struct MaynardPanelClass MaynardPanelClass;
typedef struct MaynardPanelPrivate MaynardPanelPrivate;

struct MaynardPanel
{
  GtkWindow parent;

  MaynardPanelPrivate *priv;
};

struct MaynardPanelClass
{
  GtkWindowClass parent_class;
};

GType maynard_panel_get_type (void) G_GNUC_CONST;

GtkWidget * maynard_panel_new (void);

GtkWidget * maynard_panel_get_indicators_menu (MaynardPanel *self);

#endif /* __MAYNARD_PANEL_H__ */
