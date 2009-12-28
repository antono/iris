/* process-1.c
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
 
/* process-1: tests for iris-process.c. */
 
#include <iris/iris.h>

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
simple (void)
{
	gint counter = 0,
	     i;

	IrisProcess *process = iris_process_new_with_func (counter_callback, NULL, NULL);

	iris_process_run (process);

	for (i=0; i < 50; i++) {
		IrisMessage *work_item = iris_message_new (0);
		iris_message_set_pointer (work_item, "counter", &counter);
		iris_process_enqueue (process, work_item);
	}
	iris_process_no_more_work (process);

	while (!iris_process_is_finished (process))
		g_thread_yield ();

	g_assert_cmpint (counter, ==, 50);

	g_object_unref (process);
}


static void
recurse (void)
{
	int counter = 0;

	IrisProcess *recursive_process = iris_process_new_with_func
	                                   (recursive_counter_callback, NULL, NULL);

	iris_process_run (recursive_process);

	IrisMessage *work_item = iris_message_new (0);
	iris_message_set_pointer (work_item, "counter", &counter);
	iris_process_enqueue (recursive_process, work_item);

	iris_process_no_more_work (recursive_process);

	while (!iris_process_is_finished (recursive_process))
		g_thread_yield ();


	g_assert_cmpint (counter, ==, 101);

	g_object_unref (recursive_process);
}

/* Cancel a process and spin waiting for it to be freed. Test fails if this
 * takes over 1 second.
 */
static void
cancelling_1 (void)
{
	int i;

	IrisProcess *process;

	process = iris_process_new_with_func (time_waster_callback, NULL, NULL);

	iris_process_run (process);

	for (i=0; i < 50; i++)
		iris_process_enqueue (process, iris_message_new (0));

	iris_process_cancel (process);
	while (!iris_process_is_finished (process))
		g_thread_yield ();

	g_object_unref (process);
};

static void
chaining_1 (void)
{
	int counter = 0,
	    i;
	
	IrisProcess *head_process = iris_process_new_with_func
	                              (pre_counter_callback, NULL, NULL);
	IrisProcess *tail_process = iris_process_new_with_func
	                              (counter_callback, NULL, NULL);

	iris_process_connect (head_process, tail_process);

	iris_process_run (head_process);

	for (i=0; i < 50; i++) {
		/* Set pointer as data instead of "counter" property, to ensure
		 * pre_counter_callback () is called to change it. */
		IrisMessage *work_item = iris_message_new_data (0, G_TYPE_POINTER, &counter);
		iris_process_enqueue (head_process, work_item);
	}
	iris_process_no_more_work (head_process);

	while (!iris_process_is_finished (tail_process))
		g_thread_yield ();

	g_assert_cmpint (counter, ==, 50);

	g_object_unref (head_process);
	g_object_unref (tail_process);
}

static void
cancelling_2 (void) {
	int i;

	IrisProcess *head_process, *mid_process, *tail_process;

	head_process = iris_process_new_with_func (time_waster_callback, NULL,
	                                           NULL);
	mid_process  = iris_process_new_with_func (time_waster_callback, NULL,
	                                           NULL);
	tail_process = iris_process_new_with_func (time_waster_callback, NULL,
	                                           NULL);

	iris_process_connect (head_process, mid_process);
	iris_process_connect (mid_process, tail_process);

	iris_process_run (head_process);

	for (i=0; i < 50; i++)
		iris_process_enqueue (head_process, iris_message_new (0));
	iris_process_no_more_work (head_process);

	iris_process_cancel (tail_process);

	while (!iris_process_is_finished (tail_process))
		g_thread_yield ();

	g_object_unref (head_process);
	g_object_unref (tail_process);
};


int main(int argc, char *argv[]) {
	g_type_init ();
	g_test_init (&argc, &argv, NULL);
	g_thread_init (NULL);

	g_test_add_func ("/process/simple", simple);
	g_test_add_func ("/process/cancelling 1", cancelling_1);
	g_test_add_func ("/process/recurse", recurse);
	g_test_add_func ("/process/chaining 1", chaining_1);
	g_test_add_func ("/process/cancelling 2", cancelling_2);

  	return g_test_run();
}