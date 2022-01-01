/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/* Original code taken from gnome-shell */
#include "config.h"

#include "shell-app-system.h"

#ifdef HAVE_GNOME_MENU
#define GMENU_I_KNOW_THIS_IS_UNSTABLE
#include <gmenu-tree.h>
#endif

#include <gio/gio.h>

#include <string.h>

enum {
  INSTALLED_CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

struct _ShellAppSystemPrivate {
#ifdef HAVE_GNOME_MENU
  GMenuTree *apps_tree;
#endif

  GHashTable *id_to_info;
};

struct _ShellAppInfo
{
#ifdef HAVE_GNOME_MENU
  GAppInfo *gapp_info;
#endif
};

static void shell_app_system_finalize (GObject *object);
#ifdef HAVE_GNOME_MENU
static void on_apps_tree_changed_cb (GMenuTree *tree, gpointer user_data);
#endif

G_DEFINE_TYPE_WITH_PRIVATE(ShellAppSystem, shell_app_system, G_TYPE_OBJECT);

#ifdef HAVE_GNOME_MENU
static ShellAppInfo *
shell_app_info_new_from_app_info (GAppInfo *gapp_info)
{
  ShellAppInfo *info = g_new0 (ShellAppInfo, 1);
  info->gapp_info = g_object_ref (gapp_info);
  return info;
}
#endif

static void
shell_app_info_free (ShellAppInfo *info)
{
#ifdef HAVE_GNOME_MENU
  g_object_unref (info->gapp_info);
#endif
  g_free (info);
}

static void shell_app_system_class_init(ShellAppSystemClass *klass)
{
  GObjectClass *gobject_class = (GObjectClass *)klass;

  gobject_class->finalize = shell_app_system_finalize;

  signals[INSTALLED_CHANGED] =
    g_signal_new ("installed-changed",
        SHELL_TYPE_APP_SYSTEM,
        G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (ShellAppSystemClass, installed_changed),
        NULL, NULL, NULL,
        G_TYPE_NONE, 0);
}

static void
shell_app_system_init (ShellAppSystem *self)
{
  ShellAppSystemPrivate *priv;

  self->priv = priv = shell_app_system_get_instance_private (self);

  priv->id_to_info = g_hash_table_new_full (g_str_hash, g_str_equal,
                                           (GDestroyNotify)g_free,
                                           (GDestroyNotify)shell_app_info_free);

#ifdef HAVE_GNOME_MENU
  priv->apps_tree = gmenu_tree_new ("applications.menu", GMENU_TREE_FLAGS_NONE);
  g_signal_connect (priv->apps_tree, "changed", G_CALLBACK (on_apps_tree_changed_cb), self);

  on_apps_tree_changed_cb (priv->apps_tree, self);
#endif
}

static void
shell_app_system_finalize (GObject *object)
{
  ShellAppSystem *self = SHELL_APP_SYSTEM (object);
  ShellAppSystemPrivate *priv = self->priv;

#ifdef HAVE_GNOME_MENU
  g_object_unref (priv->apps_tree);
#endif

  g_hash_table_destroy (priv->id_to_info);

  G_OBJECT_CLASS (shell_app_system_parent_class)->finalize (object);
}

#ifdef HAVE_GNOME_MENU
static void
get_flattened_entries_recurse (GMenuTreeDirectory *dir,
                               GHashTable         *entry_set)
{
  GMenuTreeIter *iter = gmenu_tree_directory_iter (dir);
  GMenuTreeItemType next_type;

  while ((next_type = gmenu_tree_iter_next (iter)) != GMENU_TREE_ITEM_INVALID)
    {
      gpointer item = NULL;

      switch (next_type)
        {
        case GMENU_TREE_ITEM_ENTRY:
          {
            GMenuTreeEntry *entry;
            item = entry = gmenu_tree_iter_get_entry (iter);
            /* Key is owned by entry */
            g_hash_table_replace (entry_set,
                                  (char*)gmenu_tree_entry_get_desktop_file_id (entry),
                                  gmenu_tree_item_ref (entry));
          }
          break;
        case GMENU_TREE_ITEM_DIRECTORY:
          {
            item = gmenu_tree_iter_get_directory (iter);
            get_flattened_entries_recurse ((GMenuTreeDirectory*)item, entry_set);
          }
          break;
        default:
          break;
        }
      if (item != NULL)
        gmenu_tree_item_unref (item);
    }

  gmenu_tree_iter_unref (iter);
}

static GHashTable *
get_flattened_entries_from_tree (GMenuTree *tree)
{
  GHashTable *table;
  GMenuTreeDirectory *root;

  table = g_hash_table_new_full (g_str_hash, g_str_equal,
                                 (GDestroyNotify) NULL,
                                 (GDestroyNotify) gmenu_tree_item_unref);

  root = gmenu_tree_get_root_directory (tree);

  if (root != NULL)
    get_flattened_entries_recurse (root, table);

  gmenu_tree_item_unref (root);

  return table;
}

static void
on_apps_tree_changed_cb (GMenuTree *tree,
                         gpointer   user_data)
{
  ShellAppSystem *self = SHELL_APP_SYSTEM (user_data);
  GError *error = NULL;
  GHashTable *new_apps;
  GHashTableIter iter;
  gpointer key, value;

  g_assert (tree == self->priv->apps_tree);

  if (!gmenu_tree_load_sync (self->priv->apps_tree, &error))
    {
      if (error)
        {
          g_warning ("Failed to load apps: %s", error->message);
          g_error_free (error);
        }
      else
        {
          g_warning ("Failed to load apps");
        }
      return;
    }

  /* FIXME: Is this 'replace info, then removed those that are gone from
   * the menu tree' vs. 'empty hash table, add everything that's still here'
   * still useful (from a performance POV) ?
   */

  new_apps = get_flattened_entries_from_tree (self->priv->apps_tree);
  g_hash_table_iter_init (&iter, new_apps);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      const char *id = key;
      GMenuTreeEntry *entry = value;
      GDesktopAppInfo *info;

      info = gmenu_tree_entry_get_app_info (entry);

      g_hash_table_insert (self->priv->id_to_info, g_strdup (id),
          shell_app_info_new_from_app_info ((GAppInfo*) info));
    }

  g_hash_table_iter_init (&iter, self->priv->id_to_info);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      const char *id = key;

      if (!g_hash_table_lookup (new_apps, id))
        g_hash_table_iter_remove (&iter);
    }

  g_hash_table_destroy (new_apps);

  g_signal_emit (self, signals[INSTALLED_CHANGED], 0);
}
#endif

/**
 * shell_app_system_get_default:
 *
 * Return Value: (transfer none): The global #ShellAppSystem singleton
 */
ShellAppSystem *
shell_app_system_get_default ()
{
  static ShellAppSystem *instance = NULL;

  if (instance == NULL)
    instance = g_object_new (SHELL_TYPE_APP_SYSTEM, NULL);

  return instance;
}

ShellAppInfo *
shell_app_system_get_app_info (ShellAppSystem *self,
    const gchar *id)
{
  return g_hash_table_lookup (self->priv->id_to_info, id);
}

GHashTable *
shell_app_system_get_entries (ShellAppSystem *self)
{
  return self->priv->id_to_info;
}

const gchar *
shell_app_info_get_id (ShellAppInfo *info)
{
#ifdef HAVE_GNOME_MENU
  if (info->gapp_info)
    return g_app_info_get_id (info->gapp_info);
#endif
  return NULL;
}

const gchar *
shell_app_info_get_display_name (ShellAppInfo *info)
{
#ifdef HAVE_GNOME_MENU
  if (info->gapp_info)
  return g_app_info_get_display_name (info->gapp_info);
#endif
  return NULL;
}

GIcon *
shell_app_info_get_icon (ShellAppInfo *info)
{
#ifdef HAVE_GNOME_MENU
  if (info->gapp_info)
    return g_app_info_get_icon (info->gapp_info);
#endif
  return NULL;
}

void
shell_app_info_launch (ShellAppInfo *info)
{
#ifdef HAVE_GNOME_MENU
  if (info->gapp_info)
    g_app_info_launch (info->gapp_info, NULL, NULL, NULL);
#endif
}
