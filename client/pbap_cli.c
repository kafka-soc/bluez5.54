#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <wordexp.h>

#include <glib.h>

#include "src/shared/shell.h"
#include "src/shared/util.h"
#include "gdbus/gdbus.h"
#include "agent.h"

#define PROMPT_ON	COLOR_BLUE "[pbap] # "
#define PROMPT_OFF	"Waiting to connect to obexd..."

static DBusConnection *dbus_conn;



struct pbap_client {
	GDBusProxy *create;
	GDBusProxy *pbap;
	GDBusProxy *session;
	
};
struct pbap_client pbap_cli;

static void connect_handler(DBusConnection *connection, void *user_data)
{
    printf("%s[%d] \n", __func__, __LINE__);
	bt_shell_set_prompt(PROMPT_ON);
}
static void disconnect_handler(DBusConnection *connection, void *user_data)
{
    printf("%s[%d] \n", __func__, __LINE__);
	bt_shell_detach();

	bt_shell_set_prompt(PROMPT_OFF);

}
static void message_handler(DBusConnection *connection, DBusMessage *message, void *user_data)
{
	bt_shell_printf("[SIGNAL] %s.%s\n", dbus_message_get_interface(message), dbus_message_get_member(message));
}

static void proxy_added(GDBusProxy *proxy, void *user_data)
{
	const char *interface;

	interface = g_dbus_proxy_get_interface(proxy);


    printf("%s[%d] %s\n", __func__, __LINE__, interface);

	if (!strcmp(interface, "org.bluez.obex.Client1")) {
		pbap_cli.create = proxy;
	} else if(!strcmp(interface, "org.bluez.obex.Session1")) {
		pbap_cli.session = proxy;
	} else if(!strcmp(interface ,"org.bluez.obex.PhonebookAccess1")) {
		pbap_cli.pbap = proxy;
	}
}

static void proxy_removed(GDBusProxy *proxy, void *user_data)
{
	const char *interface;

	interface = g_dbus_proxy_get_interface(proxy);

    printf("%s[%d] \n", __func__, __LINE__);

	// if (!strcmp(interface, "org.bluez.Device1")) {
	// 	device_removed(proxy);
	// }
}

static void property_changed(GDBusProxy *proxy, const char *name,
					DBusMessageIter *iter, void *user_data)
{
	const char *interface;

	interface = g_dbus_proxy_get_interface(proxy);

    printf("%s[%d]: intf %s\n", __func__, __LINE__, interface);

#if(0)
	if (!strcmp(interface, "org.bluez.Device1")) {
		if (default_ctrl && device_is_child(proxy,
					default_ctrl->proxy) == TRUE) {
			DBusMessageIter addr_iter;
			char *str;

			if (g_dbus_proxy_get_property(proxy, "Address",
							&addr_iter) == TRUE) {
				const char *address;

				dbus_message_iter_get_basic(&addr_iter,
								&address);
				str = g_strdup_printf("[" COLORED_CHG
						"] Device %s ", address);
			} else
				str = g_strdup("");

			if (strcmp(name, "Connected") == 0) {
				dbus_bool_t connected;

				dbus_message_iter_get_basic(iter, &connected);

				if (connected && default_dev == NULL)
					set_default_device(proxy, NULL);
				else if (!connected && default_dev == proxy)
					set_default_device(NULL, NULL);
			}

			print_iter(str, name, iter);
			g_free(str);
		}
	} else if (proxy == default_attr) {
		char *str;

		str = g_strdup_printf("[" COLORED_CHG "] Attribute %s ",
						g_dbus_proxy_get_path(proxy));

		print_iter(str, name, iter);
		g_free(str);
	}
#endif
}

static void client_ready(GDBusClient *client, void *user_data)
{
	bt_shell_attach(fileno(stdin));
}

static void pbap_create_setup(DBusMessageIter *iter, void *user_data)
{
	const char *dest = "90:F0:52:6E:0C:71";
	DBusMessageIter dict, entry, value;
	const char *str;

	dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &dest);

	dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY,
				DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
				DBUS_TYPE_STRING_AS_STRING
				DBUS_TYPE_VARIANT_AS_STRING
				DBUS_DICT_ENTRY_END_CHAR_AS_STRING, &dict);

	dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY,
							NULL, &entry);
	str = "Target";
	dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &str);
	dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT,
					DBUS_TYPE_STRING_AS_STRING, &value);
	str = "PBAP";
	dbus_message_iter_append_basic(&value, DBUS_TYPE_STRING, &str);
	dbus_message_iter_close_container(&entry, &value);
	dbus_message_iter_close_container(&dict, &entry);

	dbus_message_iter_close_container(iter, &dict);
}

static void pbap_create_reply(DBusMessage *message, void *user_data)
{
	DBusError error;

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, message) == TRUE) {
		g_print("Failed to register profile\n");
		return;
	}

	g_print("Profile registered\n");
}

static void cmd_create(int argc, char *argv[])
{

	printf("%s[%d] \n", __func__, __LINE__);

	if (g_dbus_proxy_method_call(pbap_cli.create, "CreateSession",
					pbap_create_setup, pbap_create_reply,
					NULL, NULL) == FALSE) {
		bt_shell_printf("Failed to set discovery filter\n");
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	return bt_shell_noninteractive_quit(EXIT_SUCCESS);
}

static void pbap_select_setup(DBusMessageIter *iter, void *user_data)
{
	const char *path = "int";
	const char *pbabook = "pb";
	// DBusMessageIter dict;

	dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &path);
	dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &pbabook);

	// dbus_message_iter_close_container(iter, &dict);
}

static void pbap_select_reply(DBusMessage *message, void *user_data)
{
	DBusError error;

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, message) == TRUE) {
		printf("%s[%d] failed\n", __func__, __LINE__);
		return;
	}

	printf("%s[%d] success\n", __func__, __LINE__);
}

static void cmd_select(int argc, char *argv[])
{

	printf("%s[%d] \n", __func__, __LINE__);

	if (g_dbus_proxy_method_call(pbap_cli.pbap, "Select",
					pbap_select_setup, pbap_select_reply,
					NULL, NULL) == FALSE) {
			printf("%s[%d] failed\n", __func__, __LINE__);
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	return bt_shell_noninteractive_quit(EXIT_SUCCESS);
}

static void pbap_getsize_reply(DBusMessage *message, void *user_data)
{
	DBusError error;
	DBusMessageIter iter;
	int size;

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, message) == TRUE) {
		g_print("Failed to register profile\n");
		return;
	}

	dbus_message_iter_init(message, &iter);
	dbus_message_iter_get_basic(&iter, &size);

	printf("%s[%d] success, size %d\n", __func__, __LINE__, size);
}

static void cmd_getsize(int argc, char *argv[])
{

	printf("%s[%d] \n", __func__, __LINE__);

	if (g_dbus_proxy_method_call(pbap_cli.pbap, "GetSize",
					NULL, pbap_getsize_reply,
					NULL, NULL) == FALSE) {
		printf("%s[%d] failed\n", __func__, __LINE__);
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	return bt_shell_noninteractive_quit(EXIT_SUCCESS);
}

static void pbap_list_setup(DBusMessageIter *iter, void *user_data)
{
	DBusMessageIter dict, entry, value;
	const char *str;

	dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY,
				DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
				DBUS_TYPE_STRING_AS_STRING
				DBUS_TYPE_VARIANT_AS_STRING
				DBUS_DICT_ENTRY_END_CHAR_AS_STRING, &dict);

	dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY,
							NULL, &entry);
	str = "Order";
	dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &str);
	dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT,
					DBUS_TYPE_STRING_AS_STRING, &value);
	str = "indexed";
	dbus_message_iter_append_basic(&value, DBUS_TYPE_STRING, &str);
	dbus_message_iter_close_container(&entry, &value);
	dbus_message_iter_close_container(&dict, &entry);

	dbus_message_iter_close_container(iter, &dict);

}

static void pbap_list_reply(DBusMessage *message, void *user_data)
{
	DBusError error;
	DBusMessageIter iter,sub, info;
	struct info {
		char *a;
		char *b;
	} pbap_infos;
	char *str;
	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, message) == TRUE) {
		printf("%s[%d] failed\n", __func__, __LINE__);
		return;
	}
	dbus_message_iter_init(message, &iter);
	if(DBUS_TYPE_ARRAY == dbus_message_iter_get_arg_type(&iter) ) {
		printf("%s[%d]: type: %c\n", __func__, __LINE__, dbus_message_iter_get_arg_type(&iter));
		dbus_message_iter_recurse(&iter, &sub);
		printf("%s[%d]: type: %c\n", __func__, __LINE__, dbus_message_iter_get_arg_type(&sub));
	}
	while (DBUS_TYPE_STRUCT == dbus_message_iter_get_arg_type(&sub)) {
		dbus_message_iter_recurse(&sub, &info);
		dbus_message_iter_get_basic(&info, &pbap_infos.a);
		dbus_message_iter_next(&info);
		dbus_message_iter_get_basic(&info, &pbap_infos.b);
		printf("%s[%d] %s %s\n", __func__, __LINE__, pbap_infos.a, pbap_infos.b);
		dbus_message_iter_next(&sub);
	}

	printf("%s[%d] success\n", __func__, __LINE__);
}

static void cmd_list(int argc, char *argv[])
{

	printf("%s[%d] \n", __func__, __LINE__);

	if (g_dbus_proxy_method_call(pbap_cli.pbap, "List",
					pbap_list_setup, pbap_list_reply,
					NULL, NULL) == FALSE) {
			printf("%s[%d] failed\n", __func__, __LINE__);
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	return bt_shell_noninteractive_quit(EXIT_SUCCESS);
}

static const struct bt_shell_menu main_menu = {
	.name = "main",
	.entries = {
	{ "create",         NULL,       cmd_create, "List available controllers" },
	{ "select",         NULL,       cmd_select, "List available controllers" },
	{ "getsize",        NULL,      	cmd_getsize, "List available controllers" },
	{ "list",         	NULL,      	cmd_list, "List available controllers" },
	{ } },
};



int main(int argc, char *argv[])
{
	GDBusClient *client;
	int status;

	bt_shell_init(argc, argv, NULL);
	bt_shell_set_prompt(PROMPT_OFF);
	bt_shell_set_menu(&main_menu);

	dbus_conn = g_dbus_setup_bus(DBUS_BUS_SESSION, NULL, NULL);
	g_dbus_attach_object_manager(dbus_conn);

	client = g_dbus_client_new(dbus_conn, "org.bluez.obex", "/org/bluez/obex");

	g_dbus_client_set_connect_watch(client, connect_handler, NULL);
	g_dbus_client_set_disconnect_watch(client, disconnect_handler, NULL);
	g_dbus_client_set_signal_watch(client, message_handler, NULL);

	g_dbus_client_set_proxy_handlers(client, proxy_added, proxy_removed,
							property_changed, NULL);

	g_dbus_client_set_ready_watch(client, client_ready, NULL);

	status = bt_shell_run();

	g_dbus_client_unref(client);

	dbus_connection_unref(dbus_conn);

	return status;
}
