/*
 * Copyright (c) 2010-2011 Michael Kuhn
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**
 * \file
 **/

#include <glib.h>

#include <jbackground-operation-internal.h>

/**
 * \defgroup JBackgroundOperation Background Operation
 * @{
 **/

struct JBackgroundOperation
{
	JBackgroundOperationFunc func;
	gpointer data;
	gpointer result;

	gboolean completed;

	GMutex* mutex;
	GCond* cond;

	gint ref_count;
};

static GThreadPool* j_thread_pool = NULL;

static
void
j_background_operation_thread (gpointer data, gpointer user_data)
{
	JBackgroundOperation* background_operation = data;

	background_operation->result = (*(background_operation->func))(background_operation->data);

	g_mutex_lock(background_operation->mutex);
	background_operation->completed = TRUE;
	g_cond_signal(background_operation->cond);
	g_mutex_unlock(background_operation->mutex);

	j_background_operation_unref(background_operation);
}

void
j_background_operation_init (void)
{
	GThreadPool* thread_pool;

	g_return_if_fail(j_thread_pool == NULL);

	thread_pool = g_thread_pool_new(j_background_operation_thread, NULL, /*FIXME*/16, FALSE, NULL);

	g_atomic_pointer_set(&j_thread_pool, thread_pool);
}

void
j_background_operation_fini (void)
{
	GThreadPool* thread_pool;

	g_return_if_fail(j_thread_pool != NULL);

	thread_pool = g_atomic_pointer_get(&j_thread_pool);
	g_atomic_pointer_set(&j_thread_pool, NULL);

	g_thread_pool_free(thread_pool, FALSE, TRUE);
}

JBackgroundOperation*
j_background_operation_new (JBackgroundOperationFunc func, gpointer data)
{
	JBackgroundOperation* background_operation;

	g_return_val_if_fail(func != NULL, NULL);

	background_operation = g_slice_new(JBackgroundOperation);
	background_operation->func = func;
	background_operation->data = data;
	background_operation->result = NULL;
	background_operation->completed = FALSE;
	background_operation->mutex = g_mutex_new();
	background_operation->cond = g_cond_new();
	background_operation->ref_count = 2;

	g_thread_pool_push(j_thread_pool, background_operation, NULL);

	return background_operation;
}

JBackgroundOperation*
j_background_operation_ref (JBackgroundOperation* background_operation)
{
	g_return_val_if_fail(background_operation != NULL, NULL);

	g_atomic_int_inc(&(background_operation->ref_count));

	return background_operation;
}

void
j_background_operation_unref (JBackgroundOperation* background_operation)
{
	g_return_if_fail(background_operation != NULL);

	if (g_atomic_int_dec_and_test(&(background_operation->ref_count)))
	{
		g_cond_free(background_operation->cond);
		g_mutex_free(background_operation->mutex);

		g_slice_free(JBackgroundOperation, background_operation);
	}
}

gpointer
j_background_operation_wait (JBackgroundOperation* background_operation)
{
	g_return_val_if_fail(background_operation != NULL, NULL);

	g_mutex_lock(background_operation->mutex);

	while (!background_operation->completed)
	{
		g_cond_wait(background_operation->cond, background_operation->mutex);
	}

	g_mutex_unlock(background_operation->mutex);

	return background_operation->result;
}

/**
 * @}
 **/
