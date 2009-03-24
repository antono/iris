/* iris-message.h
 *
 * Copyright (C) 2009 Christian Hergert <chris@dronelabs.com>
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

#ifndef __IRIS_MESSAGE_H__
#define __IRIS_MESSAGE_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define IRIS_TYPE_MESSAGE (iris_message_get_type())

typedef struct _IrisMessage IrisMessage;

struct _IrisMessage
{
	gint            what;

	/*< private >*/
	volatile gint   ref_count;
	GHashTable     *items;
};

IrisMessage* iris_message_new        (gint what);

IrisMessage* iris_message_ref        (IrisMessage *message);
void         iris_message_unref      (IrisMessage *message);
IrisMessage* iris_message_copy       (IrisMessage *message);

void         iris_message_get_value  (IrisMessage *message, const gchar *name, GValue *value);
void         iris_message_set_value  (IrisMessage *message, const gchar *name, const GValue *value);

const gchar* iris_message_get_string (IrisMessage *message, const gchar *name);
void         iris_message_set_string (IrisMessage *message, const gchar *name, const gchar *value);

gint         iris_message_get_int    (IrisMessage *message, const gchar *name);
void         iris_message_set_int    (IrisMessage *message, const gchar *name, gint value);

G_END_DECLS

#endif /* __IRIS_MESSAGE_H__ */
