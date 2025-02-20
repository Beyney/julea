/*
 * JULEA - Flexible storage framework
 * Copyright (C) 2010-2023 Michael Kuhn
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \file
 **/

#include <julea-config.h>

#include <glib.h>
#include <glib/gprintf.h>

#ifdef HAVE_OTF
#include <otf.h>
#endif

#include <jtrace.h>

/**
 * \addtogroup JTrace
 *
 * @{
 **/

/**
 * Flags used to enable specific trace back-ends.
 */
enum JTraceFlags
{
	J_TRACE_OFF = 0,
	J_TRACE_ECHO = 1 << 0,
	J_TRACE_OTF = 1 << 1,
	J_TRACE_SUMMARY = 1 << 2
};

typedef enum JTraceFlags JTraceFlags;

struct JTraceStack
{
	gchar* name;
	guint64 enter_time;
};

typedef struct JTraceStack JTraceStack;

struct JTraceTime
{
	gdouble time;
	guint count;
};

typedef struct JTraceTime JTraceTime;

/**
 * A trace thread.
 **/
struct JTraceThread
{
	/**
	 * Thread name.
	 * "Main thread" or "Thread %d".
	 **/
	gchar* thread_name;

	/**
	 * Function depth within the current thread.
	 **/
	guint function_depth;

	GArray* stack;

#ifdef HAVE_OTF
	/**
	 * OTF-specific structure.
	 **/
	struct
	{
		/**
		 * Thread's process ID.
		 **/
		guint32 process_id;
	} otf;
#endif
};

typedef struct JTraceThread JTraceThread;

struct JTrace
{
	gchar* name;
	guint64 enter_time;
};

static JTraceFlags j_trace_flags = J_TRACE_OFF;

static gchar* j_trace_name = NULL;
static gint j_trace_thread_id = 1;

static GPatternSpec** j_trace_function_patterns = NULL;

#ifdef HAVE_OTF
static OTF_FileManager* otf_manager = NULL;
static OTF_Writer* otf_writer = NULL;

static guint32 otf_process_id = 1;
static guint32 otf_function_id = 1;
static guint32 otf_file_id = 1;
static guint32 otf_counter_id = 1;

static GHashTable* otf_function_table = NULL;
static GHashTable* otf_file_table = NULL;
static GHashTable* otf_counter_table = NULL;

G_LOCK_DEFINE_STATIC(j_trace_otf);
#endif

static void j_trace_thread_default_free(gpointer);

static GPrivate j_trace_thread_default = G_PRIVATE_INIT(j_trace_thread_default_free);
static GHashTable* j_trace_summary_table = NULL;

G_LOCK_DEFINE_STATIC(j_trace_echo);
G_LOCK_DEFINE_STATIC(j_trace_summary);

/**
 * Creates a new trace thread.
 *
 * \private
 *
 * \code
 * \endcode
 *
 * \param thread A thread.
 *
 * \return A new trace thread. Should be freed with j_trace_thread_free().
 **/
static JTraceThread*
j_trace_thread_new(GThread* thread)
{
	JTraceThread* trace_thread;

	if (j_trace_flags == J_TRACE_OFF)
	{
		return NULL;
	}

	trace_thread = g_slice_new(JTraceThread);
	trace_thread->function_depth = 0;
	trace_thread->stack = g_array_new(FALSE, FALSE, sizeof(JTraceStack));

	if (thread == NULL)
	{
		trace_thread->thread_name = g_strdup("Main process");
	}
	else
	{
		guint thread_id;

		/// \todo use name?
		thread_id = g_atomic_int_add(&j_trace_thread_id, 1);
		trace_thread->thread_name = g_strdup_printf("Thread %d", thread_id);
	}

#ifdef HAVE_OTF
	if (j_trace_flags & J_TRACE_OTF)
	{
		trace_thread->otf.process_id = g_atomic_int_add(&otf_process_id, 1);

		OTF_Writer_writeDefProcess(otf_writer, 0, trace_thread->otf.process_id, trace_thread->thread_name, 0);
		OTF_Writer_writeBeginProcess(otf_writer, g_get_real_time(), trace_thread->otf.process_id);
	}
#endif

	return trace_thread;
}

/**
 * Decreases a trace thread's reference count.
 * When the reference count reaches zero, frees the memory allocated for the trace thread.
 *
 * \private
 *
 * \code
 * \endcode
 *
 * \param trace_thread A trace thread.
 **/
static void
j_trace_thread_free(JTraceThread* trace_thread)
{
	if (j_trace_flags == J_TRACE_OFF)
	{
		return;
	}

	g_return_if_fail(trace_thread != NULL);

#ifdef HAVE_OTF
	if (j_trace_flags & J_TRACE_OTF)
	{
		OTF_Writer_writeEndProcess(otf_writer, g_get_real_time(), trace_thread->otf.process_id);
	}
#endif

	g_free(trace_thread->thread_name);
	g_array_free(trace_thread->stack, TRUE);
	g_slice_free(JTraceThread, trace_thread);
}

/**
 * Returns the default trace thread.
 *
 * \private
 *
 * \return The default trace thread.
 **/
static JTraceThread*
j_trace_thread_get_default(void)
{
	JTraceThread* trace_thread;

	trace_thread = g_private_get(&j_trace_thread_default);

	if (G_UNLIKELY(trace_thread == NULL))
	{
		trace_thread = j_trace_thread_new(g_thread_self());
		g_private_replace(&j_trace_thread_default, trace_thread);
	}

	return trace_thread;
}

static void
j_trace_thread_default_free(gpointer data)
{
	JTraceThread* trace_thread = data;

	j_trace_thread_free(trace_thread);
}

/**
 * Prints a common header to stderr.
 *
 * \private
 *
 * \code
 * \endcode
 *
 * \param trace_thread A trace thread.
 * \param timestamp    A timestamp.
 **/
static void
j_trace_echo_printerr(JTraceThread* trace_thread, guint64 timestamp)
{
	guint i;

	g_printerr("[%" G_GUINT64_FORMAT ".%06" G_GUINT64_FORMAT "] %s %s: ", timestamp / G_USEC_PER_SEC, timestamp % G_USEC_PER_SEC, j_trace_name, trace_thread->thread_name);

	for (i = 0; i < trace_thread->function_depth; i++)
	{
		g_printerr("  ");
	}
}

/**
 * Returns the name of a file operation.
 *
 * \private
 *
 * \code
 * \endcode
 *
 * \param op A file operation.
 *
 * \return A name.
 **/
static gchar const*
j_trace_file_operation_name(JTraceFileOperation op)
{
	switch (op)
	{
		case J_TRACE_FILE_CLOSE:
			return "close";
		case J_TRACE_FILE_CREATE:
			return "create";
		case J_TRACE_FILE_DELETE:
			return "delete";
		case J_TRACE_FILE_OPEN:
			return "open";
		case J_TRACE_FILE_READ:
			return "read";
		case J_TRACE_FILE_SEEK:
			return "seek";
		case J_TRACE_FILE_STATUS:
			return "status";
		case J_TRACE_FILE_SYNC:
			return "sync";
		case J_TRACE_FILE_WRITE:
			return "write";
		default:
			g_warn_if_reached();
			return NULL;
	}
}

/**
 * Checks whether a function should be traced.
 *
 * \private
 *
 * \param name A function name.
 *
 * \return TRUE if the function should be traced, FALSE otherwise.
 **/
static gboolean
j_trace_function_check(gchar const* name)
{
	if (j_trace_function_patterns != NULL)
	{
		guint i;

		for (i = 0; j_trace_function_patterns[i] != NULL; i++)
		{
#if GLIB_CHECK_VERSION(2, 70, 0)
			if (g_pattern_spec_match_string(j_trace_function_patterns[i], name))
#else
			if (g_pattern_match_string(j_trace_function_patterns[i], name))
#endif
			{
				return TRUE;
			}
		}

		return FALSE;
	}

	return TRUE;
}

void
j_trace_init(gchar const* name)
{
	gchar const* j_trace;
	gchar const* j_trace_function;
	g_auto(GStrv) trace_parts = NULL;
	guint trace_len;

	g_return_if_fail(name != NULL);
	g_return_if_fail(j_trace_flags == J_TRACE_OFF);

	if ((j_trace = g_getenv("JULEA_TRACE")) == NULL)
	{
		return;
	}

	trace_parts = g_strsplit(j_trace, ",", 0);
	trace_len = g_strv_length(trace_parts);

	for (guint i = 0; i < trace_len; i++)
	{
		if (g_strcmp0(trace_parts[i], "echo") == 0)
		{
			j_trace_flags |= J_TRACE_ECHO;
		}
		else if (g_strcmp0(trace_parts[i], "otf") == 0)
		{
			j_trace_flags |= J_TRACE_OTF;
		}
		else if (g_strcmp0(trace_parts[i], "summary") == 0)
		{
			j_trace_flags |= J_TRACE_SUMMARY;
		}
	}

	if (j_trace_flags == J_TRACE_OFF)
	{
		return;
	}

	if ((j_trace_function = g_getenv("JULEA_TRACE_FUNCTION")) != NULL)
	{
		g_auto(GStrv) p = NULL;
		guint l;

		p = g_strsplit(j_trace_function, ",", 0);
		l = g_strv_length(p);

		j_trace_function_patterns = g_new(GPatternSpec*, l + 1);

		for (guint i = 0; i < l; i++)
		{
			j_trace_function_patterns[i] = g_pattern_spec_new(p[i]);
		}

		j_trace_function_patterns[l] = NULL;
	}

#ifdef HAVE_OTF
	if (j_trace_flags & J_TRACE_OTF)
	{
		otf_manager = OTF_FileManager_open(8);
		g_assert(otf_manager != NULL);

		otf_writer = OTF_Writer_open(name, 0, otf_manager);
		g_assert(otf_writer != NULL);

		OTF_Writer_writeDefCreator(otf_writer, 0, "JTrace");
		OTF_Writer_writeDefTimerResolution(otf_writer, 0, G_USEC_PER_SEC);

		otf_function_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
		otf_file_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
		otf_counter_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	}
#endif

	if (j_trace_flags & J_TRACE_SUMMARY)
	{
		j_trace_summary_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	}

	g_free(j_trace_name);
	j_trace_name = g_strdup(name);
}

void
j_trace_fini(void)
{
	if (j_trace_flags == J_TRACE_OFF)
	{
		return;
	}

#ifdef HAVE_OTF
	if (j_trace_flags & J_TRACE_OTF)
	{
		g_hash_table_unref(otf_function_table);
		otf_function_table = NULL;

		g_hash_table_unref(otf_file_table);
		otf_file_table = NULL;

		g_hash_table_unref(otf_counter_table);
		otf_counter_table = NULL;

		OTF_Writer_close(otf_writer);
		otf_writer = NULL;

		OTF_FileManager_close(otf_manager);
		otf_manager = NULL;
	}
#endif

	if (j_trace_flags & J_TRACE_SUMMARY)
	{
		GHashTableIter iter;
		gchar* key;
		JTraceTime* value;

		g_hash_table_iter_init(&iter, j_trace_summary_table);

		g_printerr("# stack duration[s] count\n");

		while (g_hash_table_iter_next(&iter, (gpointer*)&key, (gpointer*)&value))
		{
			g_printerr("%s %f %d\n", key, value->time, value->count);
		}

		g_hash_table_unref(j_trace_summary_table);
	}

	j_trace_flags = J_TRACE_OFF;

	if (j_trace_function_patterns != NULL)
	{
		guint i;

		for (i = 0; j_trace_function_patterns[i] != NULL; i++)
		{
			g_pattern_spec_free(j_trace_function_patterns[i]);
		}
	}

	g_free(j_trace_function_patterns);
	g_free(j_trace_name);
}

JTrace*
j_trace_enter(gchar const* name, gchar const* format, ...)
{
	JTraceThread* trace_thread;
	JTrace* trace;
	guint64 timestamp;
	va_list args;

	if (j_trace_flags == J_TRACE_OFF)
	{
		return NULL;
	}

	g_return_val_if_fail(name != NULL, NULL);

	trace_thread = j_trace_thread_get_default();

	if (!j_trace_function_check(name))
	{
		/// \todo also blacklist nested functions
		return NULL;
	}

	timestamp = g_get_real_time();

	trace = g_slice_new(JTrace);
	trace->name = g_strdup(name);
	trace->enter_time = timestamp;

	va_start(args, format);

	if (j_trace_flags & J_TRACE_ECHO)
	{
		G_LOCK(j_trace_echo);
		j_trace_echo_printerr(trace_thread, timestamp);

		if (format != NULL)
		{
			g_autofree gchar* arguments = NULL;

			arguments = g_strdup_vprintf(format, args);
			g_printerr("ENTER %s (%s)\n", name, arguments);
		}
		else
		{
			g_printerr("ENTER %s\n", name);
		}

		G_UNLOCK(j_trace_echo);
	}

#ifdef HAVE_OTF
	if (j_trace_flags & J_TRACE_OTF)
	{
		gpointer value;
		guint32 function_id;

		G_LOCK(j_trace_otf);

		if ((value = g_hash_table_lookup(otf_function_table, name)) == NULL)
		{
			function_id = g_atomic_int_add(&otf_function_id, 1);

			OTF_Writer_writeDefFunction(otf_writer, 0, function_id, name, 0, 0);
			g_hash_table_insert(otf_function_table, g_strdup(name), GUINT_TO_POINTER(function_id));
		}
		else
		{
			function_id = GPOINTER_TO_UINT(value);
		}

		G_UNLOCK(j_trace_otf);

		OTF_Writer_writeEnter(otf_writer, timestamp, function_id, trace_thread->otf.process_id, 0);
	}
#endif

	if (j_trace_flags & J_TRACE_SUMMARY)
	{
		JTraceStack current_stack;

		if (trace_thread->stack->len == 0)
		{
			current_stack.name = g_strdup(name);
		}
		else
		{
			JTraceStack* top_stack;

			top_stack = &g_array_index(trace_thread->stack, JTraceStack, trace_thread->stack->len - 1);
			current_stack.name = g_strdup_printf("%s/%s", top_stack->name, name);
		}

		current_stack.enter_time = timestamp;
		g_array_append_val(trace_thread->stack, current_stack);
	}

	va_end(args);

	trace_thread->function_depth++;

	return trace;
}

void
j_trace_leave(JTrace* trace)
{
	JTraceThread* trace_thread;
	guint64 timestamp;

	if (trace == NULL)
	{
		return;
	}

	if (j_trace_flags == J_TRACE_OFF)
	{
		goto end;
	}

	trace_thread = j_trace_thread_get_default();

	if (!j_trace_function_check(trace->name))
	{
		goto end;
	}

	/// \todo
	if (trace_thread->function_depth == 0)
	{
		goto end;
	}

	trace_thread->function_depth--;
	timestamp = g_get_real_time();

	if (j_trace_flags & J_TRACE_ECHO)
	{
		guint64 duration;

		duration = timestamp - trace->enter_time;

		G_LOCK(j_trace_echo);
		j_trace_echo_printerr(trace_thread, timestamp);
		g_printerr("LEAVE %s", trace->name);
		g_printerr(" [%" G_GUINT64_FORMAT ".%06" G_GUINT64_FORMAT "s]\n", duration / G_USEC_PER_SEC, duration % G_USEC_PER_SEC);
		G_UNLOCK(j_trace_echo);
	}

#ifdef HAVE_OTF
	if (j_trace_flags & J_TRACE_OTF)
	{
		gpointer value;
		guint32 function_id;

		G_LOCK(j_trace_otf);

		value = g_hash_table_lookup(otf_function_table, trace->name);
		g_assert(value != NULL);
		function_id = GPOINTER_TO_UINT(value);

		G_UNLOCK(j_trace_otf);

		OTF_Writer_writeLeave(otf_writer, timestamp, function_id, trace_thread->otf.process_id, 0);
	}
#endif

	if (j_trace_flags & J_TRACE_SUMMARY)
	{
		JTraceTime* combined_duration;
		JTraceStack* top_stack;
		guint64 duration;

		g_assert(trace_thread->stack->len > 0);

		top_stack = &g_array_index(trace_thread->stack, JTraceStack, trace_thread->stack->len - 1);
		duration = timestamp - top_stack->enter_time;

		G_LOCK(j_trace_summary);

		combined_duration = g_hash_table_lookup(j_trace_summary_table, top_stack->name);

		if (combined_duration == NULL)
		{
			combined_duration = g_new(JTraceTime, 1);
			combined_duration->time = ((gdouble)duration) / ((gdouble)G_USEC_PER_SEC);
			combined_duration->count = 1;

			g_hash_table_insert(j_trace_summary_table, g_strdup(top_stack->name), combined_duration);
		}
		else
		{
			combined_duration->time += ((gdouble)duration) / ((gdouble)G_USEC_PER_SEC);
			combined_duration->count++;
		}

		G_UNLOCK(j_trace_summary);

		g_free(top_stack->name);
		g_array_set_size(trace_thread->stack, trace_thread->stack->len - 1);
	}

end:
	g_free(trace->name);
	g_slice_free(JTrace, trace);
}

void
j_trace_file_begin(gchar const* path, JTraceFileOperation op)
{
	JTraceThread* trace_thread;
	guint64 timestamp;

	g_return_if_fail(path != NULL);

	if (j_trace_flags == J_TRACE_OFF)
	{
		return;
	}

	trace_thread = j_trace_thread_get_default();
	timestamp = g_get_real_time();

	if (j_trace_flags & J_TRACE_ECHO)
	{
		G_LOCK(j_trace_echo);
		j_trace_echo_printerr(trace_thread, timestamp);
		g_printerr("BEGIN %s %s\n", j_trace_file_operation_name(op), path);
		G_UNLOCK(j_trace_echo);
	}

#ifdef HAVE_OTF
	if (j_trace_flags & J_TRACE_OTF)
	{
		gpointer value;
		guint32 file_id;

		G_LOCK(j_trace_otf);

		if ((value = g_hash_table_lookup(otf_file_table, path)) == NULL)
		{
			file_id = g_atomic_int_add(&otf_file_id, 1);

			OTF_Writer_writeDefFile(otf_writer, 0, file_id, path, 0);
			g_hash_table_insert(otf_file_table, g_strdup(path), GUINT_TO_POINTER(file_id));
		}
		else
		{
			file_id = GPOINTER_TO_UINT(value);
		}

		G_UNLOCK(j_trace_otf);

		OTF_Writer_writeBeginFileOperation(otf_writer, timestamp, trace_thread->otf.process_id, 1, 0);
	}
#endif

	return;
}

void
j_trace_file_end(gchar const* path, JTraceFileOperation op, guint64 length, guint64 offset)
{
	JTraceThread* trace_thread;
	guint64 timestamp;

	if (j_trace_flags == J_TRACE_OFF)
	{
		return;
	}

	trace_thread = j_trace_thread_get_default();
	timestamp = g_get_real_time();

	if (j_trace_flags & J_TRACE_ECHO)
	{
		G_LOCK(j_trace_echo);
		j_trace_echo_printerr(trace_thread, timestamp);
		g_printerr("END %s %s", j_trace_file_operation_name(op), path);

		if (op == J_TRACE_FILE_READ || op == J_TRACE_FILE_WRITE)
		{
			g_printerr(" (length=%" G_GUINT64_FORMAT ", offset=%" G_GUINT64_FORMAT ")", length, offset);
		}

		g_printerr("\n");
		G_UNLOCK(j_trace_echo);
	}

#ifdef HAVE_OTF
	if (j_trace_flags & J_TRACE_OTF)
	{
		gpointer value;
		guint32 file_id;
		guint32 otf_op;

		switch (op)
		{
			case J_TRACE_FILE_CLOSE:
				otf_op = OTF_FILEOP_CLOSE;
				break;
			case J_TRACE_FILE_CREATE:
				otf_op = OTF_FILEOP_OTHER;
				break;
			case J_TRACE_FILE_DELETE:
				otf_op = OTF_FILEOP_UNLINK;
				break;
			case J_TRACE_FILE_OPEN:
				otf_op = OTF_FILEOP_OPEN;
				break;
			case J_TRACE_FILE_READ:
				otf_op = OTF_FILEOP_READ;
				break;
			case J_TRACE_FILE_SEEK:
				otf_op = OTF_FILEOP_SEEK;
				break;
			case J_TRACE_FILE_STATUS:
				otf_op = OTF_FILEOP_OTHER;
				break;
			case J_TRACE_FILE_SYNC:
				otf_op = OTF_FILEOP_SYNC;
				break;
			case J_TRACE_FILE_WRITE:
				otf_op = OTF_FILEOP_WRITE;
				break;
			default:
				otf_op = OTF_FILEOP_OTHER;
				g_warn_if_reached();
				break;
		}

		G_LOCK(j_trace_otf);

		value = g_hash_table_lookup(otf_file_table, path);
		g_assert(value != NULL);
		file_id = GPOINTER_TO_UINT(value);

		G_UNLOCK(j_trace_otf);

		OTF_Writer_writeEndFileOperation(otf_writer, timestamp, trace_thread->otf.process_id, file_id, 1, 0, otf_op, length, 0);
	}
#endif
}

void
j_trace_counter(gchar const* name, guint64 counter_value)
{
	JTraceThread* trace_thread;
	guint64 timestamp;

	if (j_trace_flags == J_TRACE_OFF)
	{
		return;
	}

	g_return_if_fail(name != NULL);

	trace_thread = j_trace_thread_get_default();
	timestamp = g_get_real_time();

	if (j_trace_flags & J_TRACE_ECHO)
	{
		G_LOCK(j_trace_echo);
		j_trace_echo_printerr(trace_thread, timestamp);
		g_printerr("COUNTER %s %" G_GUINT64_FORMAT "\n", name, counter_value);
		G_UNLOCK(j_trace_echo);
	}

#ifdef HAVE_OTF
	if (j_trace_flags & J_TRACE_OTF)
	{
		gpointer value;
		guint32 counter_id;

		G_LOCK(j_trace_otf);

		if ((value = g_hash_table_lookup(otf_counter_table, name)) == NULL)
		{
			counter_id = g_atomic_int_add(&otf_counter_id, 1);

			OTF_Writer_writeDefCounter(otf_writer, 0, counter_id, name, OTF_COUNTER_TYPE_ACC + OTF_COUNTER_SCOPE_START, 0, NULL);
			g_hash_table_insert(otf_counter_table, g_strdup(name), GUINT_TO_POINTER(counter_id));
		}
		else
		{
			counter_id = GPOINTER_TO_UINT(value);
		}

		G_UNLOCK(j_trace_otf);

		OTF_Writer_writeCounter(otf_writer, timestamp, trace_thread->otf.process_id, counter_id, counter_value);
	}
#endif
}

/**
 * @}
 **/
