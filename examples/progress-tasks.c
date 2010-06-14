/* process-tasks.c
 *
 * Copyright (C) 2009 Sam Thursfield <ssssam@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 
 * 02110-1301 USA
 */

/* progress-tasks: demonstrates using IrisProgressMonitor for something other
 *    than an IrisProcess. */

#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include <glib/gthread.h>

#include <iris/iris.h>
#include <iris/iris-gtk.h>

GtkWidget *demo_window = NULL,
          *progress_info_bar = NULL;

static void
create_progress_monitor () {
	GtkWidget *vbox;

	g_return_if_fail (demo_window != NULL);

	progress_info_bar = iris_progress_info_bar_new ("Contemplating ...");
	vbox = gtk_dialog_get_content_area (GTK_DIALOG (demo_window));
	gtk_box_pack_end (GTK_BOX (vbox), progress_info_bar, TRUE, FALSE, 0);

	gtk_widget_show (progress_info_bar);

	iris_progress_monitor_set_close_delay
	  (IRIS_PROGRESS_MONITOR (progress_info_bar), 500);
	g_object_add_weak_pointer (G_OBJECT (progress_info_bar),
	                           (gpointer *)&progress_info_bar);
}

static void
thinking_task_func (IrisTask *task,
                    gpointer  user_data)
{
	IrisMessage *status_message;
	IrisPort    *watch_port = g_object_get_data (G_OBJECT (task), "watch-port");
	const gint   count = GPOINTER_TO_INT (user_data);
	gboolean     cancelled = FALSE;
	gint         i;

	for (i=0; i<count; i++) {
		g_usleep (50000);

		if (iris_task_is_canceled (task)) {
			cancelled = TRUE;
			break;
		}

		status_message = iris_message_new_data (IRIS_PROGRESS_MESSAGE_FRACTION,
		                                        G_TYPE_FLOAT,
		                                        (float)i/(float)count);
		iris_port_post (watch_port, status_message);
	}

	if (cancelled) {
		status_message = iris_message_new (IRIS_PROGRESS_MESSAGE_CANCELLED);
	} else {
		status_message = iris_message_new_data (IRIS_PROGRESS_MESSAGE_FRACTION,
		                                        G_TYPE_FLOAT, 1.0);
		iris_port_post (watch_port, status_message);

		status_message = iris_message_new (IRIS_PROGRESS_MESSAGE_COMPLETE);
	}
	iris_port_post (watch_port, status_message);
}

static void
trigger_task (GtkButton *trigger,
              gpointer   user_data)
{
	IrisTask *task;
	IrisPort *watch_port;

	task = iris_task_new_with_func (thinking_task_func, user_data, NULL);

	if (progress_info_bar == NULL)
		create_progress_monitor ();

	watch_port = iris_progress_monitor_add_watch
	               (IRIS_PROGRESS_MONITOR (progress_info_bar),
	                task,
	                IRIS_PROGRESS_MONITOR_PERCENTAGE,
	                "Thinking");
	g_object_set_data (G_OBJECT (task), "watch-port", watch_port);

	iris_task_run (task);
}

static GtkWidget *
create_demo_dialog (void)
{
	GtkWidget *vbox,
	          *triggers_box,
	          *button;

	demo_window = gtk_dialog_new_with_buttons
	                ("Iris progress widgets demo", NULL, 0,
	                 GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
	                 NULL);

	vbox = gtk_dialog_get_content_area (GTK_DIALOG (demo_window));

	button = gtk_button_new_with_label ("Think about things");
	g_signal_connect (button, "clicked", G_CALLBACK (trigger_task),
	                  GINT_TO_POINTER (100));

	gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 8);
}

gint
main (gint argc, char *argv[])
{
	g_thread_init (NULL);
	gtk_init (&argc, &argv);

	create_demo_dialog ();
	g_signal_connect (demo_window, "response", gtk_main_quit, NULL);
	gtk_widget_show_all (demo_window);

	gtk_main ();
}

