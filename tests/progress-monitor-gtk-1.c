/* progress-monitor-gtk-1.c
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
 
/* progress-monitor-gtk-1: tests implementations of IrisProgressMonitor. */
 
#include <gtk/gtk.h>
#include <iris/iris.h>
#include <iris/iris-gtk.h>

#include "iris/iris-process-private.h"

/* Work item to increment a global counter, so we can tell if the hole queue
 * gets executed propertly
 */
static void
counter_callback (IrisProcess *process,
                  IrisMessage *work_item,
                  gpointer     user_data)
{
	gint *counter_address = iris_message_get_pointer (work_item, "counter");
	g_atomic_int_inc (counter_address);
}

/* Work item that must be run before counter_callback() */
static void
pre_counter_callback (IrisProcess *process,
                      IrisMessage *work_item,
                      gpointer     user_data)
{
	IrisMessage *new_work_item;
	const GValue *data = iris_message_get_data (work_item);

	new_work_item = iris_message_new (0);
	iris_message_set_pointer (new_work_item, "counter", g_value_get_pointer (data));

	iris_process_forward (process, new_work_item);
}


static void
recursive_counter_callback (IrisProcess *process,
                            IrisMessage *work_item,
                            gpointer     user_data)
{
	gint *counter_address = iris_message_get_pointer (work_item, "counter"),
	      i;

	/* Add 2 new work items, up to 100 */
	if (*counter_address < 50) {
		for (i=0; i<2; i++) {
			IrisMessage *new_work_item = iris_message_new (0);
			iris_message_set_pointer (new_work_item, "counter", counter_address);
			iris_process_recurse (process, new_work_item);
		}
	}

	(*counter_address) ++;
}

static void
time_waster_callback (IrisProcess *process,
                      IrisMessage *work_item,
                      gpointer     user_data)
{
	g_usleep (1 * G_USEC_PER_SEC);
}

static void
count_sheep_func (IrisProcess *process,
                  IrisMessage *work_item,
                  gpointer     user_data)
{
	g_usleep (10000);
}


/* From examples/progress-tasks.c, code for a task that will send appropriate messages for
 * IrisProgressMonitor */
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
		g_usleep (1000);

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


typedef struct {
	GMainLoop           *main_loop;
	IrisProgressMonitor *monitor;
} ProgressFixture;

static void
iris_progress_dialog_fixture_setup (ProgressFixture *fixture,
                                    gconstpointer    user_data)
{
	GtkWidget *dialog;

	fixture->main_loop = g_main_loop_new (NULL, TRUE);

	dialog = iris_progress_dialog_new ("Test Process", NULL);

	g_assert (IRIS_IS_PROGRESS_DIALOG (dialog));

	fixture->monitor = IRIS_PROGRESS_MONITOR (dialog);

	/* Don't close the dialog automatically, so we can test its state */
	iris_progress_monitor_set_close_delay (fixture->monitor, -1);

	gtk_widget_show (dialog);
}

static void
iris_progress_dialog_fixture_teardown (ProgressFixture *fixture,
                                       gconstpointer    user_data)
{
	if (fixture->monitor != NULL)
		gtk_widget_destroy (GTK_WIDGET (fixture->monitor));

	g_main_loop_unref (fixture->main_loop);
}

static void
iris_progress_info_bar_fixture_setup (ProgressFixture *fixture,
                                      gconstpointer    user_data)
{
	GtkWidget *info_bar;

	fixture->main_loop = g_main_loop_new (NULL, TRUE);

	info_bar = iris_progress_info_bar_new ("Test Process");

	g_assert (IRIS_IS_PROGRESS_INFO_BAR (info_bar));

	fixture->monitor = IRIS_PROGRESS_MONITOR (info_bar);

	/* Don't close the bar automatically, so we can test its state */
	iris_progress_monitor_set_close_delay (fixture->monitor, -1);

	gtk_widget_show (info_bar);
}

static void
iris_progress_info_bar_fixture_teardown (ProgressFixture *fixture,
                                         gconstpointer    user_data)
{
	if (fixture->monitor != NULL)
		gtk_widget_destroy (GTK_WIDGET (fixture->monitor));

	g_main_loop_unref (fixture->main_loop);
}



static void
simple (ProgressFixture *fixture,
        gconstpointer data)
{
	gint counter = 0,
	     i;

	IrisProcess *process = iris_process_new_with_func (counter_callback, NULL,
	                                                   NULL);
	iris_progress_monitor_watch_process (fixture->monitor, process,
		                                 IRIS_PROGRESS_MONITOR_ITEMS);
	iris_process_run (process);

	for (i=0; i < 50; i++) {
		IrisMessage *work_item = iris_message_new (0);
		iris_message_set_pointer (work_item, "counter", &counter);
		iris_process_enqueue (process, work_item);
	}
	iris_process_no_more_work (process);

	while (!iris_process_is_finished (process)) {
		g_thread_yield ();
		g_main_context_iteration (NULL, FALSE);
	}

	g_object_unref (process);
}


/* titles: test that NULL titles don't break anything */
static void
titles (ProgressFixture *fixture,
        gconstpointer data)
{
	gint counter = 0,
	     i;

	IrisProcess *process = iris_process_new_with_func (count_sheep_func, NULL,
	                                                   NULL);
	iris_progress_monitor_watch_process (fixture->monitor, process,
	                                     IRIS_PROGRESS_MONITOR_ITEMS);
	iris_progress_monitor_set_title (fixture->monitor, NULL);
	iris_process_run (process);

	for (i=0; i < 50; i++) {
		IrisMessage *work_item = iris_message_new (0);
		iris_message_set_pointer (work_item, "counter", &counter);
		iris_process_enqueue (process, work_item);
	}

	/* Check the title change message doesn't crash .. we don't test if the
	 * change is reflected in the UI (but it should be obvious in the examples)
	 */
	iris_process_set_title (process, "Test title");
	iris_process_no_more_work (process);

	while (!iris_process_is_finished (process)) {
		g_thread_yield ();
		g_main_context_iteration (NULL, FALSE);
	}

	g_object_unref (process);
}


static void
recurse_1 (ProgressFixture *fixture,
           gconstpointer data)
{
	int counter = 0;

	IrisProcess *recursive_process = iris_process_new_with_func
	                                   (recursive_counter_callback, NULL, NULL);
	iris_progress_monitor_watch_process (fixture->monitor,
	                                     recursive_process,
	                                     IRIS_PROGRESS_MONITOR_ITEMS);
	iris_process_run (recursive_process);

	IrisMessage *work_item = iris_message_new (0);
	iris_message_set_pointer (work_item, "counter", &counter);
	iris_process_enqueue (recursive_process, work_item);

	iris_process_no_more_work (recursive_process);

	while (!iris_process_is_finished (recursive_process)) {
		g_thread_yield ();
		g_main_context_iteration (NULL, FALSE);
	}

	/* Processing must have finished if _is_finished()==TRUE. In particular
	 * IRIS_PROGRESS_MESSAGE_FINISH is always sent before the process is flagged as finished,
	 * so the watch will not be left hanging.
	 */
	g_object_unref (recursive_process);
}

static void
chaining_1 (ProgressFixture *fixture,
            gconstpointer data)
{
	int counter = 0,
	    i;
	
	IrisProcess *head_process = iris_process_new_with_func
	                              (pre_counter_callback, NULL, NULL);
	IrisProcess *tail_process = iris_process_new_with_func
	                              (counter_callback, NULL, NULL);

	iris_process_connect (head_process, tail_process);
	iris_progress_monitor_watch_process_chain (fixture->monitor, head_process,
	                                           IRIS_PROGRESS_MONITOR_ITEMS);
	iris_process_run (head_process);

	for (i=0; i < 50; i++) {
		/* Set pointer as data instead of "counter" property, to ensure
		 * pre_counter_callback () is called to change it. */
		IrisMessage *work_item = iris_message_new_data (0, G_TYPE_POINTER, &counter);
		iris_process_enqueue (head_process, work_item);
	}
	iris_process_no_more_work (head_process);

	while (!iris_process_is_finished (tail_process)) {
		g_thread_yield ();
		g_main_context_iteration (NULL, FALSE);
	}

	g_object_unref (head_process);
	g_object_unref (tail_process);
}

/* cancelling: test the widget's cancel button */
/*static void
cancelling (ProgressFixture *fixture,
            gconstpointer data)
{
	int i;

	IrisProcess *process;

	process = iris_process_new_with_func (time_waster_callback, NULL, NULL);
	iris_progress_monitor_watch_process (fixture->monitor, process);
	iris_process_run (process);

	for (i=0; i < 50; i++)
		iris_process_enqueue (process, iris_message_new (0));

	progress_fixture_cancel (fixture);

	while (!iris_process_is_finished (process)) {
		g_thread_yield ();
		g_main_context_iteration (NULL, FALSE);
	}

	g_object_unref (process);
};
*/


/* completing 1: run several processes under the same monitor, all completing
 *               quickly. This tests the widget does not try to destroy itself
 *               multiple times or something silly.
 */
static void
completing_1 (ProgressFixture *fixture,
              gconstpointer    data)
{
	IrisProcess *process;
	int i, j;

	iris_progress_monitor_set_close_delay (fixture->monitor, 500);

	g_object_add_weak_pointer (G_OBJECT (fixture->monitor),
	                           (gpointer *) &fixture->monitor);

	for (i=0; i<4; i++) {
		process = iris_process_new_with_func (count_sheep_func, NULL, NULL);

		iris_progress_monitor_watch_process (fixture->monitor, process,
		                                     IRIS_PROGRESS_MONITOR_ITEMS);

		for (j=0; j<10; j++)
			iris_process_enqueue (process, iris_message_new (0));
		iris_process_no_more_work (process);

		iris_process_run (process);
	}

	while (fixture->monitor != NULL) {
		g_thread_yield ();
		g_main_context_iteration (NULL, FALSE);
	}
}

/* recurse_2: potentially recursive process with only 1 work item */
static void
recurse_2_head_callback (IrisProcess *process,
                         IrisMessage *work_item,
                         gpointer     user_data) {
	IrisMessage *new_work_item;

	g_assert (iris_process_has_successor (process));

	new_work_item = iris_message_new (2);
	iris_process_forward (process, new_work_item);
}

static void
recurse_2 (ProgressFixture *fixture,
           gconstpointer    data) {
	IrisProcess *head_process, *tail_process;
	IrisMessage *item;

	g_object_add_weak_pointer (G_OBJECT (fixture->monitor),
	                           (gpointer *) &fixture->monitor);
	iris_progress_monitor_set_close_delay (fixture->monitor, 500);

	// MUSIC_SEARCH_PROCESS
	head_process = iris_process_new_with_func (recurse_2_head_callback, NULL, NULL);

	item = iris_message_new (1);
	iris_process_enqueue (head_process, item);

	// FILE IMPORT PROCESS
	tail_process = iris_process_new_with_func (count_sheep_func, NULL, NULL);

	iris_process_connect (head_process, tail_process);
	iris_process_no_more_work (head_process);
	iris_process_run (head_process);

	iris_progress_monitor_watch_process_chain (IRIS_PROGRESS_MONITOR(fixture->monitor),
	                                           head_process,
	                                           IRIS_PROGRESS_MONITOR_ITEMS);

	while (fixture->monitor != NULL) {
		g_thread_yield ();
		g_main_context_iteration (NULL, FALSE);
	}
}

/* tasks: check that progress monitors don't only work for IrisProcess */
static void
tasks (ProgressFixture *fixture,
       gconstpointer    data)
{
	IrisTask *task;
	IrisPort *watch_port;

	task = iris_task_new_with_func (thinking_task_func, GINT_TO_POINTER (100), NULL);

	watch_port = iris_progress_monitor_add_watch
	               (IRIS_PROGRESS_MONITOR (fixture->monitor),
	                task,
	                IRIS_PROGRESS_MONITOR_PERCENTAGE,
	                "Test");
	g_object_set_data (G_OBJECT (task), "watch-port", watch_port);

	g_object_add_weak_pointer (G_OBJECT (fixture->monitor),
	                           (gpointer *) &fixture->monitor);
	iris_progress_monitor_set_close_delay (fixture->monitor, 500);

	iris_task_run (task);

	while (fixture->monitor != NULL) {
		g_thread_yield ();
		g_main_context_iteration (NULL, FALSE);
	}
}

static void
add_tests_with_fixture (void (*setup) (ProgressFixture *, gconstpointer),
                        void (*teardown) (ProgressFixture *, gconstpointer),
                        const char *name)
{
	char buf[256];

	g_snprintf (buf, 255, "/progress-monitor/%s/simple", name);
	g_test_add (buf, ProgressFixture, NULL, setup, simple, teardown);

	g_snprintf (buf, 255, "/progress-monitor/%s/titles", name);
	g_test_add (buf, ProgressFixture, NULL, setup, titles, teardown);

	g_snprintf (buf, 255, "/progress-monitor/%s/recurse 1", name);
	g_test_add (buf, ProgressFixture, NULL, setup, recurse_1, teardown);

	g_snprintf (buf, 255, "/progress-monitor/%s/chaining 1", name);
	g_test_add (buf, ProgressFixture, NULL, setup, chaining_1, teardown);

	g_snprintf (buf, 255, "/progress-monitor/%s/completing 1", name);
	g_test_add (buf, ProgressFixture, NULL, setup, completing_1, teardown);

	g_snprintf (buf, 255, "/progress-monitor/%s/recurse 2", name);
	g_test_add (buf, ProgressFixture, NULL, setup, recurse_2, teardown);

	g_snprintf (buf, 255, "/progress-monitor/%s/tasks", name);
	g_test_add (buf, ProgressFixture, NULL, setup, tasks, teardown);

	/*g_snprintf (buf, 255, "/process/%s/cancelling", name);
	g_test_add (buf, ProgressFixture, NULL, setup, cancelling, teardown);*/
}

int main(int argc, char *argv[]) {
	g_thread_init (NULL);
	gtk_test_init (&argc, &argv, NULL);

	add_tests_with_fixture (iris_progress_dialog_fixture_setup,
	                        iris_progress_dialog_fixture_teardown,
	                        "dialog");
	add_tests_with_fixture (iris_progress_info_bar_fixture_setup,
	                        iris_progress_info_bar_fixture_teardown,
	                        "info-bar");

  	return g_test_run();
}
