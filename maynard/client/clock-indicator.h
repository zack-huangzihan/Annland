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

#ifndef __MAYNARD_CLOCK_INDICATOR_H__
#define __MAYNARD_CLOCK_INDICATOR_H__

#include <gtk/gtk.h>

#include "indicator.h"

#define MAYNARD_CLOCK_INDICATOR_TYPE                 (maynard_clock_indicator_get_type ())
#define MAYNARD_CLOCK_INDICATOR(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAYNARD_CLOCK_INDICATOR_TYPE, MaynardClockIndicator))
#define MAYNARD_CLOCK_INDICATOR_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), MAYNARD_CLOCK_INDICATOR_TYPE, MaynardClockIndicatorClass))
#define MAYNARD_IS_CLOCK_INDICATOR(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MAYNARD_CLOCK_INDICATOR_TYPE))
#define MAYNARD_IS_CLOCK_INDICATOR_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), MAYNARD_CLOCK_INDICATOR_TYPE))
#define MAYNARD_CLOCK_INDICATOR_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), MAYNARD_CLOCK_INDICATOR_TYPE, MaynardClockIndicatorClass))

typedef struct _MaynardClockIndicator MaynardClockIndicator;
typedef struct _MaynardClockIndicatorClass MaynardClockIndicatorClass;
typedef struct _MaynardClockIndicatorPrivate MaynardClockIndicatorPrivate;

struct _MaynardClockIndicator
{
  MaynardIndicator parent;

  MaynardClockIndicatorPrivate *priv;
};

struct _MaynardClockIndicatorClass
{
  MaynardIndicatorClass parent_class;
};

GType maynard_clock_indicator_get_type (void) G_GNUC_CONST;

GtkWidget *maynard_clock_indicator_new (void);

#endif /* __MAYNARD_CLOCK_INDICATOR_H__ */
