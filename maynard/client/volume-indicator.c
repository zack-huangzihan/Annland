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

#include "volume-indicator.h"

#include <alsa/asoundlib.h>

struct _MaynardVolumeIndicatorPrivate {
  GtkWidget *icon;
  GtkWidget *menu_icon;
  GtkWidget *menu_scale;

  snd_mixer_t *mixer_handle;
  snd_mixer_elem_t *mixer;
  glong min_volume, max_volume;
  gboolean handle_mixer_events;
  GThread *handle_mixer_events_thread;
};

G_DEFINE_TYPE_WITH_PRIVATE (MaynardVolumeIndicator, maynard_volume_indicator, MAYNARD_INDICATOR_TYPE)

static void
maynard_volume_indicator_init (MaynardVolumeIndicator *self)
{
  self->priv = maynard_volume_indicator_get_instance_private (self);
}

static gdouble
alsa_volume_to_percentage (MaynardVolumeIndicator *self,
    glong value)
{
  glong range;

  /* min volume isn't always zero unfortunately */
  range = self->priv->max_volume - self->priv->min_volume;

  value -= self->priv->min_volume;

  return (value / (gdouble) range) * 100;
}

static glong
percentage_to_alsa_volume (MaynardVolumeIndicator *self,
    gdouble value)
{
  glong range;

  /* min volume isn't always zero unfortunately */
  range = self->priv->max_volume - self->priv->min_volume;

  return (range * value / 100) + self->priv->min_volume;
}

static void
update_icons (MaynardVolumeIndicator *self,
    gdouble volume)
{
  const gchar *icon_name;
  /* update the icon */
  if (volume > 70)
    icon_name = "audio-volume-high-symbolic";
  else if (volume > 30)
    icon_name = "audio-volume-medium-symbolic";
  else if (volume > 0)
    icon_name = "audio-volume-low-symbolic";
  else
    icon_name = "audio-volume-muted-symbolic";

  gtk_image_set_from_icon_name (GTK_IMAGE (self->priv->menu_icon),
      icon_name, GTK_ICON_SIZE_LARGE_TOOLBAR);

  gtk_image_set_from_icon_name (GTK_IMAGE (self->priv->icon),
      icon_name, GTK_ICON_SIZE_LARGE_TOOLBAR);
}

static void
volume_changed_cb (GtkRange *range,
    MaynardVolumeIndicator *self)
{
  gdouble value;

  value = gtk_range_get_value (range);

  if (self->priv->mixer != NULL)
    {
      snd_mixer_selem_set_playback_volume_all (self->priv->mixer,
          percentage_to_alsa_volume (self, value));
    }

  update_icons (self, value);
}

static gboolean
update_volume_cb (MaynardVolumeIndicator *self)
{
  if (self->priv->mixer != NULL)
    {
      glong volume;
      gdouble volume_percentage;
      int unmuted = 0;

      snd_mixer_selem_get_playback_switch (self->priv->mixer, 0, &unmuted);
      if (unmuted)
        {
          snd_mixer_selem_get_playback_volume (self->priv->mixer, 0, &volume);
          volume_percentage = alsa_volume_to_percentage (self, volume);
        }
      else
        {
          volume = 0;
          volume_percentage = 0;
        }

      g_signal_handlers_disconnect_by_func (self->priv->menu_scale,
          G_CALLBACK (volume_changed_cb), self);
      gtk_range_set_value (GTK_RANGE (self->priv->menu_scale),
          volume_percentage);
      update_icons (self, volume_percentage);
      g_signal_connect (self->priv->menu_scale, "value-changed",
          G_CALLBACK (volume_changed_cb), self);
    }

  return G_SOURCE_REMOVE;
}

static int
mixer_cb (snd_mixer_elem_t *mixer, unsigned int mask)
{
  MaynardVolumeIndicator *self = MAYNARD_VOLUME_INDICATOR (
      snd_mixer_elem_get_callback_private (mixer));
  g_idle_add ((GSourceFunc) update_volume_cb, self);

  return 0;
}

gpointer
mixer_handle_events (MaynardVolumeIndicator *self)
{
  int res;
  while (self->priv->handle_mixer_events)
    {
      /* poll every 1s such that we don't block
       * forever on exit if no event is pending */
      res = snd_mixer_wait (self->priv->mixer_handle, 1000);
      if (res >= 0)
        snd_mixer_handle_events (self->priv->mixer_handle);
    }
  self->priv->handle_mixer_events_thread = NULL;
  return NULL;
}

static void
setup_mixer (MaynardVolumeIndicator *self)
{
  snd_mixer_selem_id_t *sid;
  gint ret;

  /* TODO - use pulseaudio? */
  if ((ret = snd_mixer_open (&self->priv->mixer_handle, 0)) < 0)
    goto error;

  if ((ret = snd_mixer_attach (self->priv->mixer_handle, "default")) < 0)
    goto error;

  if ((ret = snd_mixer_selem_register (self->priv->mixer_handle, NULL, NULL)) < 0)
    goto error;

  if ((ret = snd_mixer_load (self->priv->mixer_handle)) < 0)
    goto error;

  snd_mixer_selem_id_alloca (&sid);
  snd_mixer_selem_id_set_index (sid, 0);
  snd_mixer_selem_id_set_name (sid, "PCM");
  self->priv->mixer = snd_mixer_find_selem (self->priv->mixer_handle, sid);

  /* fallback to mixer "Master" */
  if (self->priv->mixer == NULL)
    {
      snd_mixer_selem_id_set_name (sid, "Master");
      self->priv->mixer = snd_mixer_find_selem (self->priv->mixer_handle, sid);
      if (self->priv->mixer == NULL)
        goto error;
    }

  if ((ret = snd_mixer_selem_get_playback_volume_range (self->priv->mixer,
              &self->priv->min_volume, &self->priv->max_volume)) < 0)
    goto error;

  snd_mixer_elem_set_callback (self->priv->mixer, mixer_cb);
  snd_mixer_elem_set_callback_private (self->priv->mixer, self);
  self->priv->handle_mixer_events = TRUE;
  self->priv->handle_mixer_events_thread = g_thread_new ("handle-mixer-events",
       (GThreadFunc) mixer_handle_events, self);

  return;

error:
  g_warning ("failed to setup mixer: %s", snd_strerror (ret));

  if (self->priv->mixer_handle != NULL)
    snd_mixer_close (self->priv->mixer_handle);
  self->priv->mixer_handle = NULL;
  self->priv->mixer = NULL;
}

static GtkWidget *
create_volume_menu (MaynardVolumeIndicator *self)
{
  GtkWidget *box;
  GtkWidget *icon;
  GtkWidget *scale;

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_style_context_add_class (gtk_widget_get_style_context (box),
      "maynard-volume-indicator-menu-item");

  icon = gtk_image_new_from_icon_name (
      "audio-volume-muted-symbolic",
      GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_style_context_add_class (gtk_widget_get_style_context (icon),
      "maynard-volume-indicator-menu-item-icon");
  gtk_box_pack_start (GTK_BOX (box), icon, FALSE, FALSE, 0);
  self->priv->menu_icon = icon;

  scale = gtk_scale_new_with_range (
      GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
  gtk_style_context_add_class (gtk_widget_get_style_context (scale),
      "maynard-volume-indicator-menu-item-scale");
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
  gtk_widget_set_size_request (scale, 100, -1);
  gtk_box_pack_start (GTK_BOX (box), scale, TRUE, TRUE, 0);
  g_signal_connect (scale, "value-changed",
      G_CALLBACK (volume_changed_cb), self);
  self->priv->menu_scale = scale;

  return box;
}

static void
maynard_volume_indicator_constructed (GObject *object)
{
  MaynardVolumeIndicator *self = MAYNARD_VOLUME_INDICATOR (object);
  GtkWidget *icon, *menu;

  G_OBJECT_CLASS (maynard_volume_indicator_parent_class)->constructed (object);

  gtk_style_context_add_class (
      gtk_widget_get_style_context (GTK_WIDGET (self)),
      "maynard-volume-indicator");

  /* the indicator widget */
  icon = gtk_image_new_from_icon_name (
      "audio-volume-muted-symbolic",
      GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_style_context_add_class (gtk_widget_get_style_context (icon),
      "maynard-volume-indicator-icon");
  self->priv->icon = icon;
  maynard_indicator_set_widget (MAYNARD_INDICATOR (self), icon);

  /* the menu */
  menu = create_volume_menu (self);
  maynard_indicator_set_menu (MAYNARD_INDICATOR (self), menu);

  setup_mixer (self);

  update_volume_cb (self);
}

static void
maynard_volume_indicator_dispose (GObject *object)
{
  MaynardVolumeIndicator *self = MAYNARD_VOLUME_INDICATOR (object);

  if (self->priv->handle_mixer_events_thread)
    {
      self->priv->handle_mixer_events = FALSE;
      g_thread_join (self->priv->handle_mixer_events_thread);
      self->priv->handle_mixer_events_thread = NULL;
    }
  if (self->priv->mixer_handle != NULL)
    snd_mixer_close (self->priv->mixer_handle);
  self->priv->mixer_handle = NULL;
  self->priv->mixer = NULL;

  G_OBJECT_CLASS (maynard_volume_indicator_parent_class)->dispose (object);
}

static void
maynard_volume_indicator_class_init (MaynardVolumeIndicatorClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  object_class->constructed = maynard_volume_indicator_constructed;
  object_class->dispose = maynard_volume_indicator_dispose;
}

GtkWidget *
maynard_volume_indicator_new (void)
{
  return g_object_new (MAYNARD_VOLUME_INDICATOR_TYPE, NULL);
}
