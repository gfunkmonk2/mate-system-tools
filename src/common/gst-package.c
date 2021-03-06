/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* 
 * Copyright (C) 2004 Vincent Untz
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors:   Vincent Untz <vincent@vuntz.net>
 *            Guillaume Desmottes <cass@skynet.be>
 *            Carlos Garnacho <carlosg@gnome.org>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gdk/gdkspawn.h>
#include <gdk/gdkx.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkwindow.h>
#include <glib/gi18n.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "gst-package.h"

static void
show_error_dialog (GtkWindow   *window,
		   const gchar *secondary_text)
{
      GtkWidget *dialog;

      dialog = gtk_message_dialog_new (window,
				       GTK_DIALOG_MODAL,
				       GTK_MESSAGE_ERROR,
				       GTK_BUTTONS_CLOSE,
				       _("Could not install package"));

      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), secondary_text);
      gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);
}

static gboolean
find_app (GtkWindow   *window,
	  const gchar *app)
{
  gchar *path;

  path = g_find_program_in_path (app);

  if (!path)
    {
      show_error_dialog (window,
			 _("The necessary applications to install"
			   " the package could not be found."));
      return FALSE;
    }

  g_free (path);

  return TRUE;
}

static gchar *
create_temp_file (const gchar *packages[])
{
  int fd;
  gchar *path, *str;

  path = g_strdup_printf ("/tmp/packages.XXXXXX");
  fd = mkstemp (path);

  while (*packages)
    {
      str = g_strdup_printf ("%s\ti\n", *packages);
      write (fd, str, strlen (str));
      g_free (str);
      packages++;
    }

  close (fd);
  return path;
}

static gchar*
get_synaptic_command_line (GtkWindow   *window,
			   const gchar *path)
{
  gchar *synaptic_path, *command;

  synaptic_path = g_find_program_in_path ("synaptic");
  command = g_strdup_printf ("%s --hide-main-window --non-interactive "
			     "--set-selections-file %s --parent-window-id %d",
			     synaptic_path, path, GDK_WINDOW_XID (GTK_WIDGET (window)->window));
  g_free (synaptic_path);
  return command;
}

static gboolean
spawn_synaptic (GtkWindow   *window,
		const gchar *path,
		gint        *child_pid)
{
  gchar **argv;
  GError *error = NULL;
  gboolean retval = TRUE;
  gint i = 0;

  argv = g_new0 (gchar*, 6);
  argv[i++] = g_find_program_in_path ("gksudo");
  argv[i++] = g_strdup ("--desktop");
  argv[i++] = g_strdup ("/usr/share/applications/synaptic.desktop");
  argv[i++] = g_strdup ("--disable-grab");
  argv[i++] = get_synaptic_command_line (window, path);
  argv[i++] = NULL;

  if (!gdk_spawn_on_screen (gtk_window_get_screen (window),
			    NULL, argv, NULL,
			    G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
			    NULL, NULL, child_pid, &error))
    {
      show_error_dialog (window, (error) ? error->message : "");
      g_error_free (error);
      retval = FALSE;
    }

  g_strfreev (argv);

  return retval;
}

gboolean
on_wait_timeout (gpointer data)
{
  gint pid, status;

  pid = GPOINTER_TO_INT (data);

  if (waitpid (pid, &status, WNOHANG) > 0)
    {
      /* FIXME: should show an error dialog if the installation fails,
       * but funnily I'm getting always an exit status 0 here... maybe gksudo fault? */
      gtk_main_quit ();
      return FALSE;
    }

  return TRUE;
}

static gboolean
wait_for_synaptic (GtkWindow *window, gint pid)
{
  GdkCursor *cursor;
  gint timer;

  cursor = gdk_cursor_new (GDK_WATCH);
  gdk_window_set_cursor (GTK_WIDGET (window)->window, cursor);
  gtk_widget_set_sensitive (GTK_WIDGET (window), FALSE);

  /* wait here a bit until the process has exited */
  timer = g_timeout_add (500, on_wait_timeout, GINT_TO_POINTER (pid));
  gtk_main ();

  g_source_remove (timer);
  gtk_widget_set_sensitive (GTK_WIDGET (window), TRUE);
  gdk_window_set_cursor (GTK_WIDGET (window)->window, NULL);

  gdk_cursor_unref (cursor);

  /* keep this until we can get status
   * info from the launched process */
  return TRUE;
}

gboolean
gst_packages_install (GtkWindow   *window,
		      const gchar *packages[])
{
  gchar *path;
  pid_t pid;
  gboolean retval = FALSE;

  g_return_val_if_fail (GTK_IS_WINDOW (window), FALSE);

  if (!find_app (window, "synaptic") || !find_app (window, "gksudo"))
    return FALSE;

  path = create_temp_file (packages);

  if (spawn_synaptic (window, path, &pid))
    retval = wait_for_synaptic (window, pid);

  unlink (path);
  g_free (path);

  return retval;
}
