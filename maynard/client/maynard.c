/*
 * Copyright (c) 2013 Tiago Vignatti
 * Copyright (c) 2013-2014, 2019 Collabora Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkwayland.h>

#include "weston-desktop-shell-client-protocol.h"
#include "shell-helper-client-protocol.h"
#include "desktop-shell/shared/config-parser.h"

#include "maynard-resources.h"

#include "app-icon.h"
#include "favorites.h"
#include "launcher.h"
#include "panel.h"

extern char **environ; /* defined by libc */

struct element {
  GtkWidget *window;
  GdkPixbuf *pixbuf;
  struct wl_surface *surface;
};

struct desktop {
  struct wl_display *display;
  struct wl_registry *registry;
  struct weston_desktop_shell *wshell;
  struct wl_output *output;
  struct shell_helper *helper;

  struct wl_seat *seat;
  struct wl_pointer *pointer;

  GdkDisplay *gdk_display;

  struct element *background;
  struct element *panel;
  struct element *indicators_menu;
  struct element *launcher;
  struct element *curtain;
  struct element *grab;

  gboolean launcher_visible;
  gboolean indicators_menu_visible;
  gboolean pointer_out_of_panel;
};

static gboolean panel_window_enter_cb (GtkWidget *widget,
    GdkEvent *event, struct desktop *desktop);
static gboolean panel_window_leave_cb (GtkWidget *widget,
    GdkEvent *event, struct desktop *desktop);

static void indicators_menu_toggle (GtkWidget *widget,
    struct desktop *desktop);
static void launcher_toggle (GtkWidget *widget,
    struct desktop *desktop);

static gboolean
connect_enter_leave_signals (gpointer data)
{
  struct desktop *desktop = data;

  g_signal_connect (desktop->panel->window, "enter-notify-event",
      G_CALLBACK (panel_window_enter_cb), desktop);
  g_signal_connect (desktop->panel->window, "leave-notify-event",
      G_CALLBACK (panel_window_leave_cb), desktop);

  return G_SOURCE_REMOVE;
}

static void
shell_configure (struct desktop *desktop,
    uint32_t edges,
    struct wl_surface *surface,
    int32_t width, int32_t height)
{
  GtkRequisition panel_size;
  int panel_width, panel_height;
  GtkRequisition indicators_menu_size;
  int indicators_menu_width, indicators_menu_height;
  int launcher_width, launcher_height;

  gtk_widget_get_preferred_size (desktop->panel->window, &panel_size, NULL);
  gtk_widget_get_preferred_size (desktop->indicators_menu->window, &indicators_menu_size, NULL);

  panel_width = panel_size.width;
  panel_height = height;

  indicators_menu_width = indicators_menu_size.width;
  indicators_menu_height = indicators_menu_size.height;

  launcher_width = width - panel_width;
  launcher_height = height;

  gtk_widget_set_size_request (desktop->background->window,
      width, height);
  gtk_window_resize (GTK_WINDOW (desktop->panel->window),
      panel_width, panel_height);
  gtk_window_resize (GTK_WINDOW (desktop->indicators_menu->window),
      indicators_menu_width, indicators_menu_height);
  gtk_window_resize (GTK_WINDOW (desktop->launcher->window),
      launcher_width, launcher_height);

  shell_helper_move_surface (desktop->helper, desktop->panel->surface,
      0, 0);

  // start with launcher and indicators menu hidden
  shell_helper_move_surface (desktop->helper, desktop->launcher->surface,
      - launcher_width, 0);
  shell_helper_move_surface (desktop->helper, desktop->indicators_menu->surface,
      - indicators_menu_width, height - indicators_menu_height);

  weston_desktop_shell_desktop_ready (desktop->wshell);

  /* TODO: why does the panel signal leave on drawing for
   * startup? we don't want to have to have this silly
   * timeout. */
  g_timeout_add_seconds (1, connect_enter_leave_signals, desktop);
}

static void
weston_desktop_shell_configure (void *data,
    struct weston_desktop_shell *weston_desktop_shell,
    uint32_t edges,
    struct wl_surface *surface,
    int32_t width, int32_t height)
{
  shell_configure(data, edges, surface, width, height);
}

static void
weston_desktop_shell_prepare_lock_surface (void *data,
    struct weston_desktop_shell *weston_desktop_shell)
{
  weston_desktop_shell_unlock (weston_desktop_shell);
}

static void
weston_desktop_shell_grab_cursor (void *data,
    struct weston_desktop_shell *weston_desktop_shell,
    uint32_t cursor)
{
}

static const struct weston_desktop_shell_listener wshell_listener = {
  weston_desktop_shell_configure,
  weston_desktop_shell_prepare_lock_surface,
  weston_desktop_shell_grab_cursor
};

static void
launcher_toggle (GtkWidget *widget,
    struct desktop *desktop)
{
  if (desktop->indicators_menu_visible)
    indicators_menu_toggle (desktop->indicators_menu->window, desktop);

  if (desktop->launcher_visible)
    {
      GtkAllocation launcher_allocation;

      gtk_widget_get_allocation (desktop->launcher->window, &launcher_allocation);

      // TODO - fade surface instead
      shell_helper_move_surface (desktop->helper, desktop->launcher->surface,
          - launcher_allocation.width, 0);
    }
  else
    {
      GtkAllocation panel_allocation;

      gtk_widget_get_allocation (desktop->panel->window, &panel_allocation);

      // TODO - fade surface instead
      shell_helper_move_surface (desktop->helper,
          desktop->launcher->surface,
          panel_allocation.width, 0);
    }

  desktop->launcher_visible = !desktop->launcher_visible;
}

static void
launcher_create (struct desktop *desktop)
{
  struct element *launcher;
  GdkWindow *gdk_window;

  launcher = malloc (sizeof *launcher);
  memset (launcher, 0, sizeof *launcher);

  launcher->window = maynard_launcher_new (desktop->background->window);
  gdk_window = gtk_widget_get_window (launcher->window);
  launcher->surface = gdk_wayland_window_get_wl_surface (gdk_window);

  gdk_wayland_window_set_use_custom_surface (gdk_window);
  shell_helper_add_surface_to_layer (desktop->helper,
      launcher->surface,
      desktop->panel->surface);
  gtk_widget_show_all (launcher->window);

  desktop->launcher = launcher;
}

static void
indicators_menu_toggle (GtkWidget *widget,
    struct desktop *desktop)
{
  GtkAllocation background_allocation;
  GtkAllocation indicators_menu_allocation;

  gtk_widget_get_allocation (desktop->background->window, &background_allocation);
  gtk_widget_get_allocation (desktop->indicators_menu->window, &indicators_menu_allocation);

  if (desktop->launcher_visible)
    launcher_toggle (desktop->launcher->window, desktop);

  if (desktop->indicators_menu_visible)
    {
      // TODO - fade surface instead
      shell_helper_move_surface (desktop->helper,
          desktop->indicators_menu->surface,
          - indicators_menu_allocation.width,
          background_allocation.height - indicators_menu_allocation.height);
    }
  else
    {
      GtkAllocation panel_allocation;

      gtk_widget_get_allocation (desktop->panel->window, &panel_allocation);

      // TODO - fade surface instead
      shell_helper_move_surface (desktop->helper,
          desktop->indicators_menu->surface,
          panel_allocation.width,
          background_allocation.height - indicators_menu_allocation.height);
    }

  desktop->indicators_menu_visible = !desktop->indicators_menu_visible;
}

static void
indicators_menu_create (struct desktop *desktop)
{
  struct element *indicators_menu;
  GdkWindow *gdk_window;

  indicators_menu = malloc (sizeof *indicators_menu);
  memset (indicators_menu, 0, sizeof *indicators_menu);

  indicators_menu->window = maynard_panel_get_indicators_menu (
      MAYNARD_PANEL (desktop->panel->window));
  gdk_window = gtk_widget_get_window (indicators_menu->window);
  indicators_menu->surface = gdk_wayland_window_get_wl_surface (gdk_window);

  gdk_wayland_window_set_use_custom_surface (gdk_window);
  shell_helper_add_surface_to_layer (desktop->helper,
      indicators_menu->surface,
      desktop->panel->surface);

  gtk_widget_show_all (indicators_menu->window);

  desktop->indicators_menu = indicators_menu;
}

static gboolean
panel_window_enter_cb (GtkWidget *widget,
    GdkEvent *event,
    struct desktop *desktop)
{
  desktop->pointer_out_of_panel = FALSE;
  return FALSE;
}

static gboolean
panel_window_leave_cb (GtkWidget *widget,
    GdkEvent *event,
    struct desktop *desktop)
{
  desktop->pointer_out_of_panel = TRUE;
  return FALSE;
}

static void
favorite_launched_cb (MaynardPanel *panel,
    struct desktop *desktop)
{
  if (desktop->launcher_visible)
    launcher_toggle (desktop->launcher->window, desktop);

  panel_window_leave_cb (NULL, NULL, desktop);
}

static void
panel_create (struct desktop *desktop)
{
  struct element *panel;
  GdkWindow *gdk_window;

  panel = malloc (sizeof *panel);
  memset (panel, 0, sizeof *panel);

  panel->window = maynard_panel_new ();

  g_signal_connect (panel->window, "app-menu-toggled",
      G_CALLBACK (launcher_toggle), desktop);
  g_signal_connect (panel->window, "indicators-menu-toggled",
      G_CALLBACK (indicators_menu_toggle), desktop);
  g_signal_connect (panel->window, "favorite-launched",
      G_CALLBACK (favorite_launched_cb), desktop);

  /* set it up as the panel */
  gdk_window = gtk_widget_get_window (panel->window);
  gdk_wayland_window_set_use_custom_surface (gdk_window);

  panel->surface = gdk_wayland_window_get_wl_surface (gdk_window);
  weston_desktop_shell_set_user_data (desktop->wshell, desktop);
  weston_desktop_shell_set_panel (desktop->wshell, desktop->output,
      panel->surface);
  weston_desktop_shell_set_panel_position (desktop->wshell,
      WESTON_DESKTOP_SHELL_PANEL_POSITION_LEFT);
  shell_helper_set_panel (desktop->helper, panel->surface);

  gtk_widget_show_all (panel->window);

  desktop->panel = panel;
}

/* Expose callback for the drawing area */
static gboolean
draw_cb (GtkWidget *widget,
    cairo_t *cr,
    gpointer data)
{
  struct desktop *desktop = data;

  gdk_cairo_set_source_pixbuf (cr, desktop->background->pixbuf, 0, 0);
  cairo_paint (cr);

  return TRUE;
}

/* Destroy handler for the window */
static void
destroy_cb (GObject *object,
    gpointer data)
{
  gtk_main_quit ();
}

static GdkPixbuf *
scale_background (GdkPixbuf *original_pixbuf)
{
  /* Scale original_pixbuf so it mostly fits on the screen.
   * If the aspect ratio is different than a bit on the right or on the
   * bottom could be cropped out. */
  GdkScreen *screen = gdk_screen_get_default ();
  gint screen_width, screen_height;
  gint original_width, original_height;
  gint final_width, final_height;
  gdouble ratio_horizontal, ratio_vertical, ratio;

  screen_width = gdk_screen_get_width (screen);
  screen_height = gdk_screen_get_height (screen);
  original_width = gdk_pixbuf_get_width (original_pixbuf);
  original_height = gdk_pixbuf_get_height (original_pixbuf);

  ratio_horizontal = (double) screen_width / original_width;
  ratio_vertical = (double) screen_height / original_height;
  ratio = MAX (ratio_horizontal, ratio_vertical);

  final_width = ceil (ratio * original_width);
  final_height = ceil (ratio * original_height);

  return gdk_pixbuf_scale_simple (original_pixbuf,
      final_width, final_height, GDK_INTERP_BILINEAR);
}

static void
background_create (struct desktop *desktop, char* filename)
{
  GdkWindow *gdk_window;
  struct element *background;
  GdkPixbuf *unscaled_background;
  const gchar *xpm_data[] = {"1 1 1 1", "_ c SteelBlue", "_"};

  background = malloc (sizeof *background);
  memset (background, 0, sizeof *background);

  if (filename && filename[0] != '\0')
    unscaled_background = gdk_pixbuf_new_from_file (filename, NULL);
  else
    unscaled_background = gdk_pixbuf_new_from_xpm_data (xpm_data);

  if (!unscaled_background)
    {
      g_message ("Could not load background (%s).",
          filename ? filename : "built-in");
      exit (EXIT_FAILURE);
    }

  background->pixbuf = scale_background (unscaled_background);
  g_object_unref (unscaled_background);

  background->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  g_signal_connect (background->window, "destroy",
      G_CALLBACK (destroy_cb), NULL);

  g_signal_connect (background->window, "draw",
      G_CALLBACK (draw_cb), desktop);

  gtk_window_set_title (GTK_WINDOW (background->window), "maynard");
  gtk_window_set_decorated (GTK_WINDOW (background->window), FALSE);
  gtk_widget_realize (background->window);

  gdk_window = gtk_widget_get_window (background->window);
  gdk_wayland_window_set_use_custom_surface (gdk_window);

  background->surface = gdk_wayland_window_get_wl_surface (gdk_window);
  weston_desktop_shell_set_user_data (desktop->wshell, desktop);
  weston_desktop_shell_set_background (desktop->wshell, desktop->output,
      background->surface);

  desktop->background = background;

  gtk_widget_show_all (background->window);
}

static void
curtain_create (struct desktop *desktop)
{
  GdkWindow *gdk_window;
  struct element *curtain;

  curtain = malloc (sizeof *curtain);
  memset (curtain, 0, sizeof *curtain);

  curtain->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_window_set_title (GTK_WINDOW (curtain->window), "maynard");
  gtk_window_set_decorated (GTK_WINDOW (curtain->window), FALSE);
  gtk_widget_set_size_request (curtain->window, 8192, 8192);
  gtk_widget_realize (curtain->window);

  gdk_window = gtk_widget_get_window (curtain->window);
  gdk_wayland_window_set_use_custom_surface (gdk_window);

  curtain->surface = gdk_wayland_window_get_wl_surface (gdk_window);

  desktop->curtain = curtain;

  gtk_widget_show_all (curtain->window);
}

static void
css_setup (struct desktop *desktop)
{
  GtkCssProvider *provider;
  GFile *file;
  GError *error = NULL;

  provider = gtk_css_provider_new ();

  file = g_file_new_for_uri ("resource:///org/maynard/style.css");

  if (!gtk_css_provider_load_from_file (provider, file, &error))
    {
      g_warning ("Failed to load CSS file: %s", error->message);
      g_clear_error (&error);
      g_object_unref (file);
      return;
    }

  gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
      GTK_STYLE_PROVIDER (provider), 600);

  g_object_unref (file);
}


static void
pointer_handle_enter (void *data,
    struct wl_pointer *pointer,
    uint32_t serial,
    struct wl_surface *surface,
    wl_fixed_t sx_w,
    wl_fixed_t sy_w)
{
}

static void
pointer_handle_leave (void *data,
    struct wl_pointer *pointer,
    uint32_t serial,
    struct wl_surface *surface)
{
}

static void
pointer_handle_motion (void *data,
    struct wl_pointer *pointer,
    uint32_t time,
    wl_fixed_t sx_w,
    wl_fixed_t sy_w)
{
}

static void
pointer_handle_button (void *data,
    struct wl_pointer *pointer,
    uint32_t serial,
    uint32_t time,
    uint32_t button,
    uint32_t state_w)
{
  struct desktop *desktop = data;

  if (state_w != WL_POINTER_BUTTON_STATE_RELEASED)
    return;

  if (!desktop->pointer_out_of_panel)
    return;

  if (desktop->launcher_visible)
    launcher_toggle (desktop->launcher->window, desktop);
  if (desktop->indicators_menu_visible)
    indicators_menu_toggle (desktop->indicators_menu->window, desktop);
}

static void
pointer_handle_axis (void *data,
    struct wl_pointer *pointer,
    uint32_t time,
    uint32_t axis,
    wl_fixed_t value)
{
}

static const struct wl_pointer_listener pointer_listener = {
  pointer_handle_enter,
  pointer_handle_leave,
  pointer_handle_motion,
  pointer_handle_button,
  pointer_handle_axis,
};

static void
seat_handle_capabilities (void *data,
    struct wl_seat *seat,
    enum wl_seat_capability caps)
{
  struct desktop *desktop = data;

  if ((caps & WL_SEAT_CAPABILITY_POINTER) && !desktop->pointer) {
    desktop->pointer = wl_seat_get_pointer(seat);
    wl_pointer_set_user_data (desktop->pointer, desktop);
    wl_pointer_add_listener(desktop->pointer, &pointer_listener,
          desktop);
  } else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && desktop->pointer) {
    wl_pointer_destroy(desktop->pointer);
    desktop->pointer = NULL;
  }

  /* TODO: keyboard and touch */
}

static void
seat_handle_name (void *data,
    struct wl_seat *seat,
    const char *name)
{
}

static const struct wl_seat_listener seat_listener = {
  seat_handle_capabilities,
  seat_handle_name
};

static void
registry_handle_global (void *data,
    struct wl_registry *registry,
    uint32_t name,
    const char *interface,
    uint32_t version)
{
  struct desktop *d = data;

  if (!strcmp (interface, "weston_desktop_shell"))
    {
      d->wshell = wl_registry_bind (registry, name,
          &weston_desktop_shell_interface, MIN(version, 1));
      weston_desktop_shell_add_listener (d->wshell, &wshell_listener, d);
      weston_desktop_shell_set_user_data (d->wshell, d);
    }
  else if (!strcmp (interface, "wl_output"))
    {
      /* TODO: create multiple outputs */
      d->output = wl_registry_bind (registry, name,
          &wl_output_interface, 1);
    }
  else if (!strcmp (interface, "wl_seat"))
    {
      d->seat = wl_registry_bind (registry, name,
          &wl_seat_interface, 1);
      wl_seat_add_listener (d->seat, &seat_listener, d);
    }
  else if (!strcmp (interface, "shell_helper"))
    {
      d->helper = wl_registry_bind (registry, name,
          &shell_helper_interface, 1);
    }
}

static void
registry_handle_global_remove (void *data,
    struct wl_registry *registry,
    uint32_t name)
{
}

static const struct wl_registry_listener registry_listener = {
  registry_handle_global,
  registry_handle_global_remove
};

static void grab_surface_create(struct desktop *desktop)
{
  GdkWindow *gdk_window;
  struct element *grab;

  grab = malloc (sizeof *grab);
  memset (grab, 0, sizeof *grab);

  grab->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_window_set_title (GTK_WINDOW (grab->window), "maynard2");
  gtk_window_set_decorated (GTK_WINDOW (grab->window), FALSE);
  gtk_widget_set_size_request (grab->window, 8192, 8192);
  gtk_widget_realize (grab->window);

  gdk_window = gtk_widget_get_window (grab->window);
  gdk_wayland_window_set_use_custom_surface (gdk_window);

  grab->surface = gdk_wayland_window_get_wl_surface (gdk_window);

  desktop->grab = grab;

  gtk_widget_show_all (grab->window);
  weston_desktop_shell_set_grab_surface(desktop->wshell, grab->surface);
}

const char *
maynard_config_get_name_from_env(void)
{
  const char *name;

  name = getenv("WESTON_CONFIG_FILE");
  if (name) {
    printf("config file name = %s\n", name);
    return name;
  }

  return "weston.ini";
}

int
main (int argc,
    char *argv[])
{
  struct desktop *desktop;
  char *config_name;
  char *background_image_src;
  struct weston_config *maynard_config;
  struct weston_config_section *shell_section;

  config_name = maynard_config_get_name_from_env();
  maynard_config = weston_config_parse(config_name);
  shell_section = weston_config_get_section(maynard_config, "shell", NULL, NULL);
  weston_config_section_get_string(shell_section, "background-image", &background_image_src, "");

  gdk_set_allowed_backends ("wayland");

  gtk_init (&argc, &argv);

  g_resources_register (maynard_get_resource ());

  desktop = malloc (sizeof *desktop);
  desktop->output = NULL;
  desktop->helper = NULL;
  desktop->seat = NULL;
  desktop->pointer = NULL;

  desktop->gdk_display = gdk_display_get_default ();
  desktop->display =
    gdk_wayland_display_get_wl_display (desktop->gdk_display);
  if (desktop->display == NULL)
    {
      fprintf (stderr, "failed to get display: %m\n");
      return -1;
    }

  desktop->registry = wl_display_get_registry (desktop->display);
  wl_registry_add_listener (desktop->registry,
      &registry_listener, desktop);

  /* Wait until we have been notified about the compositor,
   * shell, and shell helper objects */
  if (!desktop->output || !desktop->wshell ||
      !desktop->helper)
    wl_display_roundtrip (desktop->display);
  if (!desktop->output || !desktop->wshell ||
      !desktop->helper)
    {
      fprintf (stderr, "could not find output, shell or helper modules\n");
      return -1;
    }

  desktop->indicators_menu_visible = FALSE;
  desktop->launcher_visible = FALSE;
  desktop->pointer_out_of_panel = FALSE;

  css_setup (desktop);
  background_create (desktop, background_image_src);
  curtain_create (desktop);

  /* panel needs to be first so the launcher grid can
   * be added to its layer */
  panel_create (desktop);
  indicators_menu_create (desktop);
  launcher_create (desktop);
  grab_surface_create (desktop);

  gtk_main ();

  /* TODO cleanup */
  return EXIT_SUCCESS;
}
