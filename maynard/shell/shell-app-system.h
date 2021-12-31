/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
#ifndef __SHELL_APP_SYSTEM_H__
#define __SHELL_APP_SYSTEM_H__

#include <gio/gio.h>

#define SHELL_TYPE_APP_SYSTEM                 (shell_app_system_get_type ())
#define SHELL_APP_SYSTEM(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), SHELL_TYPE_APP_SYSTEM, ShellAppSystem))
#define SHELL_APP_SYSTEM_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), SHELL_TYPE_APP_SYSTEM, ShellAppSystemClass))
#define SHELL_IS_APP_SYSTEM(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SHELL_TYPE_APP_SYSTEM))
#define SHELL_IS_APP_SYSTEM_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), SHELL_TYPE_APP_SYSTEM))
#define SHELL_APP_SYSTEM_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), SHELL_TYPE_APP_SYSTEM, ShellAppSystemClass))

typedef struct _ShellAppSystem ShellAppSystem;
typedef struct _ShellAppSystemClass ShellAppSystemClass;
typedef struct _ShellAppSystemPrivate ShellAppSystemPrivate;
typedef struct _ShellAppInfo ShellAppInfo;

struct _ShellAppSystem
{
  GObject parent;

  ShellAppSystemPrivate *priv;
};

struct _ShellAppSystemClass
{
  GObjectClass parent_class;

  void (*installed_changed)(ShellAppSystem *appsys, gpointer user_data);
  void (*favorites_changed)(ShellAppSystem *appsys, gpointer user_data);
};

GType           shell_app_system_get_type       (void) G_GNUC_CONST;
ShellAppSystem *shell_app_system_get_default    (void);

GHashTable     *shell_app_system_get_entries    (ShellAppSystem *self);

ShellAppInfo   *shell_app_system_get_app_info   (ShellAppSystem *self,
                                                 const gchar *id);

const gchar    *shell_app_info_get_id           (ShellAppInfo *info);
const gchar    *shell_app_info_get_display_name (ShellAppInfo *info);
GIcon          *shell_app_info_get_icon         (ShellAppInfo *info);
void            shell_app_info_launch           (ShellAppInfo *info);

#endif /* __SHELL_APP_SYSTEM_H__ */
