/* iris-progress.h
 *
 * Copyright (C) 2009-11 Sam Thursfield <ssssam@gmail.com>
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

#ifndef __IRIS_PROGRESS_H__
#define __IRIS_PROGRESS_H__

/**
 * IrisProgressMessageType:
 * @IRIS_PROGRESS_MESSAGE_COMPLETE: the task has completed.
 * @IRIS_PROGRESS_MESSAGE_CANCELLED: the task was cancelled.
 * @IRIS_PROGRESS_MESSAGE_PROCESSED_ITEMS: integer; number of items completed.
 * @IRIS_PROGRESS_MESSAGE_TOTAL_ITEMS: integer; sent when new items are enqueued.
 * @IRIS_PROGRESS_MESSAGE_FRACTION: should contain a float between 0 and 1.
 *                                  This message should only be sent if the
 *                                  above two are not sent, in the case that
 *                                  progress is better represented as a
 *                                  fraction.
 * @IRIS_PROGRESS_MESSAGE_TITLE: string; sent when the title of the process changes
 *
 * An #IrisProgressMonitor listens for these messages to update its UI. It's
 * recommended you don't send status messages more than once every 250ms or so;
 * there's no point.
 *
 * No messages should be sent after IRIS_PROGRESS_MESSAGE_CANCELLED or
 * IRIS_PROGRESS_MESSAGE_COMPLETE.
 *
 * FIXME: is this still TRUE?
 * The same message could be sent more than once; any listeners must be able to
 * handle for example @IRIS_PROGRESS_MESSAGE_COMPLETE being received twice.
 **/
typedef enum
{
	IRIS_PROGRESS_MESSAGE_COMPLETE,
	IRIS_PROGRESS_MESSAGE_CANCELLED,
	IRIS_PROGRESS_MESSAGE_PROCESSED_ITEMS,
	IRIS_PROGRESS_MESSAGE_TOTAL_ITEMS,
	IRIS_PROGRESS_MESSAGE_FRACTION,
	IRIS_PROGRESS_MESSAGE_TITLE
} IrisProgressMessageType;

#endif
