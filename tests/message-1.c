#include <iris/iris.h>
#include <string.h>

static void
ref_count1 (void)
{
	IrisMessage *msg;
	
	msg = iris_message_new (1);
	g_assert (msg != NULL);
	g_assert_cmpint (msg->ref_count, ==, 1);

	iris_message_unref (msg);
	g_assert_cmpint (msg->ref_count, ==, 0);
}

static void
ref_count2 (void)
{
	IrisMessage *msg;
	
	msg = iris_message_new (1);
	g_assert (msg != NULL);
	g_assert_cmpint (msg->ref_count, ==, 1);

	iris_message_ref (msg);
	g_assert_cmpint (msg->ref_count, ==, 2);

	iris_message_unref (msg);
	g_assert_cmpint (msg->ref_count, ==, 1);

	iris_message_unref (msg);
	g_assert_cmpint (msg->ref_count, ==, 0);
}

static void
set_string1 (void)
{
	IrisMessage *msg;

	msg = iris_message_new (1);
	g_assert (msg != NULL);

	iris_message_set_string (msg, "id", "1234567890");
	g_assert_cmpstr (iris_message_get_string (msg, "id"), ==, "1234567890");
}

static void
set_int1 (void)
{
	IrisMessage *msg;

	msg = iris_message_new (1);
	g_assert (msg != NULL);

	iris_message_set_int (msg, "id", 1234567890);
	g_assert_cmpint (iris_message_get_int (msg, "id"), ==, 1234567890);
}

static void
copy1 (void)
{
	IrisMessage *msg;
	IrisMessage *msg2;

	msg = iris_message_new (1);
	g_assert (msg != NULL);

	iris_message_set_int (msg, "id", 1234567890);
	g_assert_cmpint (iris_message_get_int (msg, "id"), ==, 1234567890);

	msg2 = iris_message_copy (msg);
	g_assert (msg2 != NULL);
	g_assert_cmpint (iris_message_get_int (msg2, "id"), ==, 1234567890);
}

static void
count_names1 (void)
{
	IrisMessage *msg;

	msg = iris_message_new (1);
	g_assert (msg != NULL);
	g_assert_cmpint (iris_message_count_names (msg), ==, 0);

	iris_message_set_int (msg, "id", 1234567890);
	g_assert_cmpint (iris_message_count_names (msg), ==, 1);
}

static void
get_type1 (void)
{
	g_assert_cmpint (IRIS_TYPE_MESSAGE, !=, G_TYPE_INVALID);
}

static void
is_empty1 (void)
{
	IrisMessage *msg;

	msg = iris_message_new (1);
	g_assert (msg != NULL);
	g_assert (iris_message_is_empty (msg));

	iris_message_set_int (msg, "id", 1234567890);
	g_assert (!iris_message_is_empty (msg));
}

static void
new_full1 (void)
{
	IrisMessage *msg;

	msg = iris_message_new_full (1,
	                             "id", G_TYPE_INT, 1234567890,
	                             "name", G_TYPE_STRING, "Christian",
	                             NULL);
	g_assert (msg != NULL);

	g_assert_cmpint (iris_message_get_int (msg, "id"), ==, 1234567890);
	g_assert_cmpstr (iris_message_get_string (msg, "name"), ==, "Christian");
}

static void
flattened_size1 (void)
{
	IrisMessage *msg;
	gsize length;

	msg = iris_message_new_full (1,
	                             "id", G_TYPE_INT, 1234567890,
	                             "name", G_TYPE_STRING, "Christian",
	                             NULL);
	g_assert (msg != NULL);

	length = iris_message_flattened_size (msg);
	g_assert_cmpint (length, ==, 4         /* Message Type */
	                           + 2 + 4     /* Id Value Type and Size */
	                           + 2 + 4 + 9 /* Name Value Type and Size and Content */
	                           + 4 + strlen ("id")
	                           + 4 + strlen ("name"));
}

gint
main (int   argc,
      char *argv[])
{
	g_type_init ();
	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/message/get_type1", get_type1);
	g_test_add_func ("/message/ref_count1", ref_count1);
	g_test_add_func ("/message/ref_count2", ref_count2);
	g_test_add_func ("/message/set_string1", set_string1);
	g_test_add_func ("/message/set_int1", set_int1);
	g_test_add_func ("/message/copy1", copy1);
	g_test_add_func ("/message/count_names1", count_names1);
	g_test_add_func ("/message/is_empty1", count_names1);
	g_test_add_func ("/message/new_full1", count_names1);
	g_test_add_func ("/message/flattened_size1", flattened_size1);

	return g_test_run ();
}
