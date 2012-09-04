#include "purple_defs.h"

#if PURPLE_RUNTIME
static HMODULE f_hPurple = NULL;
purple_debug_set_ui_ops_wrapped_fnc purple_debug_set_ui_ops_wrapped = NULL;
purple_debug_set_verbose_wrapped_fnc purple_debug_set_verbose_wrapped = NULL;
purple_request_set_ui_ops_wrapped_fnc purple_request_set_ui_ops_wrapped = NULL;
purple_imgstore_get_data_wrapped_fnc purple_imgstore_get_data_wrapped = NULL;
purple_imgstore_get_size_wrapped_fnc purple_imgstore_get_size_wrapped = NULL;
purple_imgstore_unref_wrapped_fnc purple_imgstore_unref_wrapped = NULL;
purple_markup_escape_text_wrapped_fnc purple_markup_escape_text_wrapped = NULL;
purple_markup_strip_html_wrapped_fnc purple_markup_strip_html_wrapped = NULL;
purple_normalize_wrapped_fnc purple_normalize_wrapped = NULL;
purple_strdup_withhtml_wrapped_fnc purple_strdup_withhtml_wrapped = NULL;
purple_markup_html_to_xhtml_wrapped_fnc purple_markup_html_to_xhtml_wrapped = NULL;
purple_utf8_try_convert_wrapped_fnc purple_utf8_try_convert_wrapped = NULL;
purple_util_set_user_dir_wrapped_fnc purple_util_set_user_dir_wrapped = NULL;
purple_blist_node_get_type_wrapped_fnc purple_blist_node_get_type_wrapped = NULL;
purple_buddy_get_alias_wrapped_fnc purple_buddy_get_alias_wrapped = NULL;
purple_buddy_get_server_alias_wrapped_fnc purple_buddy_get_server_alias_wrapped = NULL;
purple_find_buddy_wrapped_fnc purple_find_buddy_wrapped = NULL;
purple_buddy_get_group_wrapped_fnc purple_buddy_get_group_wrapped = NULL;
purple_blist_remove_buddy_wrapped_fnc purple_blist_remove_buddy_wrapped = NULL;
purple_blist_alias_buddy_wrapped_fnc purple_blist_alias_buddy_wrapped = NULL;
purple_blist_server_alias_buddy_wrapped_fnc purple_blist_server_alias_buddy_wrapped = NULL;
purple_find_group_wrapped_fnc purple_find_group_wrapped = NULL;
purple_group_new_wrapped_fnc purple_group_new_wrapped = NULL;
purple_blist_add_contact_wrapped_fnc purple_blist_add_contact_wrapped = NULL;
purple_buddy_get_contact_wrapped_fnc purple_buddy_get_contact_wrapped = NULL;
purple_buddy_new_wrapped_fnc purple_buddy_new_wrapped = NULL;
purple_blist_add_buddy_wrapped_fnc purple_blist_add_buddy_wrapped = NULL;
purple_blist_find_chat_wrapped_fnc purple_blist_find_chat_wrapped = NULL;
purple_chat_get_components_wrapped_fnc purple_chat_get_components_wrapped = NULL;
purple_buddy_get_presence_wrapped_fnc purple_buddy_get_presence_wrapped = NULL;
purple_buddy_get_account_wrapped_fnc purple_buddy_get_account_wrapped = NULL;
purple_buddy_get_name_wrapped_fnc purple_buddy_get_name_wrapped = NULL;
purple_find_buddies_wrapped_fnc purple_find_buddies_wrapped = NULL;
purple_group_get_name_wrapped_fnc purple_group_get_name_wrapped = NULL;
purple_blist_set_ui_ops_wrapped_fnc purple_blist_set_ui_ops_wrapped = NULL;
purple_set_blist_wrapped_fnc purple_set_blist_wrapped = NULL;
purple_blist_new_wrapped_fnc purple_blist_new_wrapped = NULL;
purple_blist_load_wrapped_fnc purple_blist_load_wrapped = NULL;
purple_blist_get_handle_wrapped_fnc purple_blist_get_handle_wrapped = NULL;
purple_xfer_ui_ready_wrapped_fnc purple_xfer_ui_ready_wrapped = NULL;
purple_xfer_request_accepted_wrapped_fnc purple_xfer_request_accepted_wrapped = NULL;
purple_xfer_request_denied_wrapped_fnc purple_xfer_request_denied_wrapped = NULL;
purple_xfer_get_account_wrapped_fnc purple_xfer_get_account_wrapped = NULL;
purple_xfer_get_filename_wrapped_fnc purple_xfer_get_filename_wrapped = NULL;
purple_xfer_get_size_wrapped_fnc purple_xfer_get_size_wrapped = NULL;
purple_xfer_unref_wrapped_fnc purple_xfer_unref_wrapped = NULL;
purple_xfer_ref_wrapped_fnc purple_xfer_ref_wrapped = NULL;
purple_xfers_set_ui_ops_wrapped_fnc purple_xfers_set_ui_ops_wrapped = NULL;
purple_xfers_get_handle_wrapped_fnc purple_xfers_get_handle_wrapped = NULL;
purple_signal_connect_wrapped_fnc purple_signal_connect_wrapped = NULL;
purple_prefs_load_wrapped_fnc purple_prefs_load_wrapped = NULL;
purple_prefs_set_bool_wrapped_fnc purple_prefs_set_bool_wrapped = NULL;
purple_prefs_set_string_wrapped_fnc purple_prefs_set_string_wrapped = NULL;
purple_notify_user_info_new_wrapped_fnc purple_notify_user_info_new_wrapped = NULL;
purple_notify_user_info_destroy_wrapped_fnc purple_notify_user_info_destroy_wrapped = NULL;
purple_notify_user_info_get_entries_wrapped_fnc purple_notify_user_info_get_entries_wrapped = NULL;
purple_notify_user_info_entry_get_label_wrapped_fnc purple_notify_user_info_entry_get_label_wrapped = NULL;
purple_notify_user_info_entry_get_value_wrapped_fnc purple_notify_user_info_entry_get_value_wrapped = NULL;
purple_notify_set_ui_ops_wrapped_fnc purple_notify_set_ui_ops_wrapped = NULL;
purple_buddy_icons_set_account_icon_wrapped_fnc purple_buddy_icons_set_account_icon_wrapped = NULL;
purple_buddy_icons_find_wrapped_fnc purple_buddy_icons_find_wrapped = NULL;
purple_buddy_icon_get_full_path_wrapped_fnc purple_buddy_icon_get_full_path_wrapped = NULL;
purple_buddy_icon_unref_wrapped_fnc purple_buddy_icon_unref_wrapped = NULL;
purple_buddy_icons_find_account_icon_wrapped_fnc purple_buddy_icons_find_account_icon_wrapped = NULL;
purple_buddy_icon_get_data_wrapped_fnc purple_buddy_icon_get_data_wrapped = NULL;
purple_account_set_bool_wrapped_fnc purple_account_set_bool_wrapped = NULL;
purple_account_get_protocol_id_wrapped_fnc purple_account_get_protocol_id_wrapped = NULL;
purple_account_set_int_wrapped_fnc purple_account_set_int_wrapped = NULL;
purple_account_set_string_wrapped_fnc purple_account_set_string_wrapped = NULL;
purple_account_get_username_wrapped_fnc purple_account_get_username_wrapped = NULL;
purple_account_set_username_wrapped_fnc purple_account_set_username_wrapped = NULL;
purple_accounts_find_wrapped_fnc purple_accounts_find_wrapped = NULL;
purple_account_new_wrapped_fnc purple_account_new_wrapped = NULL;
purple_accounts_add_wrapped_fnc purple_accounts_add_wrapped = NULL;
purple_account_set_password_wrapped_fnc purple_account_set_password_wrapped = NULL;
purple_account_set_enabled_wrapped_fnc purple_account_set_enabled_wrapped = NULL;
purple_account_set_privacy_type_wrapped_fnc purple_account_set_privacy_type_wrapped = NULL;
purple_account_get_status_type_with_primitive_wrapped_fnc purple_account_get_status_type_with_primitive_wrapped = NULL;
purple_account_set_status_wrapped_fnc purple_account_set_status_wrapped = NULL;
purple_account_get_int_wrapped_fnc purple_account_get_int_wrapped = NULL;
purple_account_disconnect_wrapped_fnc purple_account_disconnect_wrapped = NULL;
purple_accounts_delete_wrapped_fnc purple_accounts_delete_wrapped = NULL;
purple_account_get_connection_wrapped_fnc purple_account_get_connection_wrapped = NULL;
purple_account_set_alias_wrapped_fnc purple_account_set_alias_wrapped = NULL;
purple_account_set_public_alias_wrapped_fnc purple_account_set_public_alias_wrapped = NULL;
purple_account_remove_buddy_wrapped_fnc purple_account_remove_buddy_wrapped = NULL;
purple_account_add_buddy_wrapped_fnc purple_account_add_buddy_wrapped = NULL;
purple_account_get_name_for_display_wrapped_fnc purple_account_get_name_for_display_wrapped = NULL;
purple_accounts_set_ui_ops_wrapped_fnc purple_accounts_set_ui_ops_wrapped = NULL;
purple_status_type_get_id_wrapped_fnc purple_status_type_get_id_wrapped = NULL;
purple_presence_get_active_status_wrapped_fnc purple_presence_get_active_status_wrapped = NULL;
purple_status_type_get_primitive_wrapped_fnc purple_status_type_get_primitive_wrapped = NULL;
purple_status_get_type_wrapped_fnc purple_status_get_type_wrapped = NULL;
purple_status_get_attr_string_wrapped_fnc purple_status_get_attr_string_wrapped = NULL;
serv_get_info_wrapped_fnc serv_get_info_wrapped = NULL;
serv_alias_buddy_wrapped_fnc serv_alias_buddy_wrapped = NULL;
serv_send_typing_wrapped_fnc serv_send_typing_wrapped = NULL;
serv_join_chat_wrapped_fnc serv_join_chat_wrapped = NULL;
purple_dnsquery_set_ui_ops_wrapped_fnc purple_dnsquery_set_ui_ops_wrapped = NULL;
purple_conversation_get_im_data_wrapped_fnc purple_conversation_get_im_data_wrapped = NULL;
purple_conversation_get_chat_data_wrapped_fnc purple_conversation_get_chat_data_wrapped = NULL;
purple_find_conversation_with_account_wrapped_fnc purple_find_conversation_with_account_wrapped = NULL;
purple_conversation_new_wrapped_fnc purple_conversation_new_wrapped = NULL;
purple_conversation_get_type_wrapped_fnc purple_conversation_get_type_wrapped = NULL;
purple_conv_im_send_wrapped_fnc purple_conv_im_send_wrapped = NULL;
purple_conv_chat_send_wrapped_fnc purple_conv_chat_send_wrapped = NULL;
purple_conversation_destroy_wrapped_fnc purple_conversation_destroy_wrapped = NULL;
purple_conversation_get_account_wrapped_fnc purple_conversation_get_account_wrapped = NULL;
purple_conversation_get_name_wrapped_fnc purple_conversation_get_name_wrapped = NULL;
purple_conversations_set_ui_ops_wrapped_fnc purple_conversations_set_ui_ops_wrapped = NULL;
purple_conversations_get_handle_wrapped_fnc purple_conversations_get_handle_wrapped = NULL;
purple_plugin_action_free_wrapped_fnc purple_plugin_action_free_wrapped = NULL;
purple_plugins_add_search_path_wrapped_fnc purple_plugins_add_search_path_wrapped = NULL;
purple_connection_get_state_wrapped_fnc purple_connection_get_state_wrapped = NULL;
purple_connection_get_account_wrapped_fnc purple_connection_get_account_wrapped = NULL;
purple_connection_get_display_name_wrapped_fnc purple_connection_get_display_name_wrapped = NULL;
purple_connections_set_ui_ops_wrapped_fnc purple_connections_set_ui_ops_wrapped = NULL;
purple_connections_get_handle_wrapped_fnc purple_connections_get_handle_wrapped = NULL;
purple_core_set_ui_ops_wrapped_fnc purple_core_set_ui_ops_wrapped = NULL;
purple_core_init_wrapped_fnc purple_core_init_wrapped = NULL;
purple_input_add_wrapped_fnc purple_input_add_wrapped = NULL;
purple_timeout_add_wrapped_fnc purple_timeout_add_wrapped = NULL;
purple_timeout_add_seconds_wrapped_fnc purple_timeout_add_seconds_wrapped = NULL;
purple_timeout_remove_wrapped_fnc purple_timeout_remove_wrapped = NULL;
purple_eventloop_set_ui_ops_wrapped_fnc purple_eventloop_set_ui_ops_wrapped = NULL;
purple_input_remove_wrapped_fnc purple_input_remove_wrapped = NULL;
purple_privacy_deny_wrapped_fnc purple_privacy_deny_wrapped = NULL;
purple_privacy_allow_wrapped_fnc purple_privacy_allow_wrapped = NULL;
purple_privacy_check_wrapped_fnc purple_privacy_check_wrapped = NULL;
purple_find_prpl_wrapped_fnc purple_find_prpl_wrapped = NULL;
purple_prpl_send_attention_wrapped_fnc purple_prpl_send_attention_wrapped = NULL;
purple_account_option_get_type_wrapped_fnc purple_account_option_get_type_wrapped = NULL;
purple_account_option_get_setting_wrapped_fnc purple_account_option_get_setting_wrapped = NULL;
wpurple_g_io_channel_win32_new_socket_wrapped_fnc wpurple_g_io_channel_win32_new_socket_wrapped = NULL;
#endif
bool resolvePurpleFunctions() {
#if PURPLE_RUNTIME
	f_hPurple = LoadLibrary("libpurple.dll");
	if (!f_hPurple)
			return false;
	purple_debug_set_ui_ops_wrapped = (purple_debug_set_ui_ops_wrapped_fnc)GetProcAddress(f_hPurple, "purple_debug_set_ui_ops");
	if (!purple_debug_set_ui_ops_wrapped)
		return false;

	purple_debug_set_verbose_wrapped = (purple_debug_set_verbose_wrapped_fnc)GetProcAddress(f_hPurple, "purple_debug_set_verbose");
	if (!purple_debug_set_verbose_wrapped)
		return false;

	purple_request_set_ui_ops_wrapped = (purple_request_set_ui_ops_wrapped_fnc)GetProcAddress(f_hPurple, "purple_request_set_ui_ops");
	if (!purple_request_set_ui_ops_wrapped)
		return false;

	purple_imgstore_get_data_wrapped = (purple_imgstore_get_data_wrapped_fnc)GetProcAddress(f_hPurple, "purple_imgstore_get_data");
	if (!purple_imgstore_get_data_wrapped)
		return false;

	purple_imgstore_get_size_wrapped = (purple_imgstore_get_size_wrapped_fnc)GetProcAddress(f_hPurple, "purple_imgstore_get_size");
	if (!purple_imgstore_get_size_wrapped)
		return false;

	purple_imgstore_unref_wrapped = (purple_imgstore_unref_wrapped_fnc)GetProcAddress(f_hPurple, "purple_imgstore_unref");
	if (!purple_imgstore_unref_wrapped)
		return false;

	purple_markup_escape_text_wrapped = (purple_markup_escape_text_wrapped_fnc)GetProcAddress(f_hPurple, "purple_markup_escape_text");
	if (!purple_markup_escape_text_wrapped)
		return false;

	purple_markup_strip_html_wrapped = (purple_markup_strip_html_wrapped_fnc)GetProcAddress(f_hPurple, "purple_markup_strip_html");
	if (!purple_markup_strip_html_wrapped)
		return false;

	purple_normalize_wrapped = (purple_normalize_wrapped_fnc)GetProcAddress(f_hPurple, "purple_normalize");
	if (!purple_normalize_wrapped)
		return false;

	purple_strdup_withhtml_wrapped = (purple_strdup_withhtml_wrapped_fnc)GetProcAddress(f_hPurple, "purple_strdup_withhtml");
	if (!purple_strdup_withhtml_wrapped)
		return false;

	purple_markup_html_to_xhtml_wrapped = (purple_markup_html_to_xhtml_wrapped_fnc)GetProcAddress(f_hPurple, "purple_markup_html_to_xhtml");
	if (!purple_markup_html_to_xhtml_wrapped)
		return false;

	purple_utf8_try_convert_wrapped = (purple_utf8_try_convert_wrapped_fnc)GetProcAddress(f_hPurple, "purple_utf8_try_convert");
	if (!purple_utf8_try_convert_wrapped)
		return false;

	purple_util_set_user_dir_wrapped = (purple_util_set_user_dir_wrapped_fnc)GetProcAddress(f_hPurple, "purple_util_set_user_dir");
	if (!purple_util_set_user_dir_wrapped)
		return false;

	purple_blist_node_get_type_wrapped = (purple_blist_node_get_type_wrapped_fnc)GetProcAddress(f_hPurple, "purple_blist_node_get_type");
	if (!purple_blist_node_get_type_wrapped)
		return false;

	purple_buddy_get_alias_wrapped = (purple_buddy_get_alias_wrapped_fnc)GetProcAddress(f_hPurple, "purple_buddy_get_alias");
	if (!purple_buddy_get_alias_wrapped)
		return false;

	purple_buddy_get_server_alias_wrapped = (purple_buddy_get_server_alias_wrapped_fnc)GetProcAddress(f_hPurple, "purple_buddy_get_server_alias");
	if (!purple_buddy_get_server_alias_wrapped)
		return false;

	purple_find_buddy_wrapped = (purple_find_buddy_wrapped_fnc)GetProcAddress(f_hPurple, "purple_find_buddy");
	if (!purple_find_buddy_wrapped)
		return false;

	purple_buddy_get_group_wrapped = (purple_buddy_get_group_wrapped_fnc)GetProcAddress(f_hPurple, "purple_buddy_get_group");
	if (!purple_buddy_get_group_wrapped)
		return false;

	purple_blist_remove_buddy_wrapped = (purple_blist_remove_buddy_wrapped_fnc)GetProcAddress(f_hPurple, "purple_blist_remove_buddy");
	if (!purple_blist_remove_buddy_wrapped)
		return false;

	purple_blist_alias_buddy_wrapped = (purple_blist_alias_buddy_wrapped_fnc)GetProcAddress(f_hPurple, "purple_blist_alias_buddy");
	if (!purple_blist_alias_buddy_wrapped)
		return false;

	purple_blist_server_alias_buddy_wrapped = (purple_blist_server_alias_buddy_wrapped_fnc)GetProcAddress(f_hPurple, "purple_blist_server_alias_buddy");
	if (!purple_blist_server_alias_buddy_wrapped)
		return false;

	purple_find_group_wrapped = (purple_find_group_wrapped_fnc)GetProcAddress(f_hPurple, "purple_find_group");
	if (!purple_find_group_wrapped)
		return false;

	purple_group_new_wrapped = (purple_group_new_wrapped_fnc)GetProcAddress(f_hPurple, "purple_group_new");
	if (!purple_group_new_wrapped)
		return false;

	purple_blist_add_contact_wrapped = (purple_blist_add_contact_wrapped_fnc)GetProcAddress(f_hPurple, "purple_blist_add_contact");
	if (!purple_blist_add_contact_wrapped)
		return false;

	purple_buddy_get_contact_wrapped = (purple_buddy_get_contact_wrapped_fnc)GetProcAddress(f_hPurple, "purple_buddy_get_contact");
	if (!purple_buddy_get_contact_wrapped)
		return false;

	purple_buddy_new_wrapped = (purple_buddy_new_wrapped_fnc)GetProcAddress(f_hPurple, "purple_buddy_new");
	if (!purple_buddy_new_wrapped)
		return false;

	purple_blist_add_buddy_wrapped = (purple_blist_add_buddy_wrapped_fnc)GetProcAddress(f_hPurple, "purple_blist_add_buddy");
	if (!purple_blist_add_buddy_wrapped)
		return false;

	purple_blist_find_chat_wrapped = (purple_blist_find_chat_wrapped_fnc)GetProcAddress(f_hPurple, "purple_blist_find_chat");
	if (!purple_blist_find_chat_wrapped)
		return false;

	purple_chat_get_components_wrapped = (purple_chat_get_components_wrapped_fnc)GetProcAddress(f_hPurple, "purple_chat_get_components");
	if (!purple_chat_get_components_wrapped)
		return false;

	purple_buddy_get_presence_wrapped = (purple_buddy_get_presence_wrapped_fnc)GetProcAddress(f_hPurple, "purple_buddy_get_presence");
	if (!purple_buddy_get_presence_wrapped)
		return false;

	purple_buddy_get_account_wrapped = (purple_buddy_get_account_wrapped_fnc)GetProcAddress(f_hPurple, "purple_buddy_get_account");
	if (!purple_buddy_get_account_wrapped)
		return false;

	purple_buddy_get_name_wrapped = (purple_buddy_get_name_wrapped_fnc)GetProcAddress(f_hPurple, "purple_buddy_get_name");
	if (!purple_buddy_get_name_wrapped)
		return false;

	purple_find_buddies_wrapped = (purple_find_buddies_wrapped_fnc)GetProcAddress(f_hPurple, "purple_find_buddies");
	if (!purple_find_buddies_wrapped)
		return false;

	purple_group_get_name_wrapped = (purple_group_get_name_wrapped_fnc)GetProcAddress(f_hPurple, "purple_group_get_name");
	if (!purple_group_get_name_wrapped)
		return false;

	purple_blist_set_ui_ops_wrapped = (purple_blist_set_ui_ops_wrapped_fnc)GetProcAddress(f_hPurple, "purple_blist_set_ui_ops");
	if (!purple_blist_set_ui_ops_wrapped)
		return false;

	purple_set_blist_wrapped = (purple_set_blist_wrapped_fnc)GetProcAddress(f_hPurple, "purple_set_blist");
	if (!purple_set_blist_wrapped)
		return false;

	purple_blist_new_wrapped = (purple_blist_new_wrapped_fnc)GetProcAddress(f_hPurple, "purple_blist_new");
	if (!purple_blist_new_wrapped)
		return false;

	purple_blist_load_wrapped = (purple_blist_load_wrapped_fnc)GetProcAddress(f_hPurple, "purple_blist_load");
	if (!purple_blist_load_wrapped)
		return false;

	purple_blist_get_handle_wrapped = (purple_blist_get_handle_wrapped_fnc)GetProcAddress(f_hPurple, "purple_blist_get_handle");
	if (!purple_blist_get_handle_wrapped)
		return false;

	purple_xfer_ui_ready_wrapped = (purple_xfer_ui_ready_wrapped_fnc)GetProcAddress(f_hPurple, "purple_xfer_ui_ready");
	if (!purple_xfer_ui_ready_wrapped)
		return false;

	purple_xfer_request_accepted_wrapped = (purple_xfer_request_accepted_wrapped_fnc)GetProcAddress(f_hPurple, "purple_xfer_request_accepted");
	if (!purple_xfer_request_accepted_wrapped)
		return false;

	purple_xfer_request_denied_wrapped = (purple_xfer_request_denied_wrapped_fnc)GetProcAddress(f_hPurple, "purple_xfer_request_denied");
	if (!purple_xfer_request_denied_wrapped)
		return false;

	purple_xfer_get_account_wrapped = (purple_xfer_get_account_wrapped_fnc)GetProcAddress(f_hPurple, "purple_xfer_get_account");
	if (!purple_xfer_get_account_wrapped)
		return false;

	purple_xfer_get_filename_wrapped = (purple_xfer_get_filename_wrapped_fnc)GetProcAddress(f_hPurple, "purple_xfer_get_filename");
	if (!purple_xfer_get_filename_wrapped)
		return false;

	purple_xfer_get_size_wrapped = (purple_xfer_get_size_wrapped_fnc)GetProcAddress(f_hPurple, "purple_xfer_get_size");
	if (!purple_xfer_get_size_wrapped)
		return false;

	purple_xfer_unref_wrapped = (purple_xfer_unref_wrapped_fnc)GetProcAddress(f_hPurple, "purple_xfer_unref");
	if (!purple_xfer_unref_wrapped)
		return false;

	purple_xfer_ref_wrapped = (purple_xfer_ref_wrapped_fnc)GetProcAddress(f_hPurple, "purple_xfer_ref");
	if (!purple_xfer_ref_wrapped)
		return false;

	purple_xfers_set_ui_ops_wrapped = (purple_xfers_set_ui_ops_wrapped_fnc)GetProcAddress(f_hPurple, "purple_xfers_set_ui_ops");
	if (!purple_xfers_set_ui_ops_wrapped)
		return false;

	purple_xfers_get_handle_wrapped = (purple_xfers_get_handle_wrapped_fnc)GetProcAddress(f_hPurple, "purple_xfers_get_handle");
	if (!purple_xfers_get_handle_wrapped)
		return false;

	purple_signal_connect_wrapped = (purple_signal_connect_wrapped_fnc)GetProcAddress(f_hPurple, "purple_signal_connect");
	if (!purple_signal_connect_wrapped)
		return false;

	purple_prefs_load_wrapped = (purple_prefs_load_wrapped_fnc)GetProcAddress(f_hPurple, "purple_prefs_load");
	if (!purple_prefs_load_wrapped)
		return false;

	purple_prefs_set_bool_wrapped = (purple_prefs_set_bool_wrapped_fnc)GetProcAddress(f_hPurple, "purple_prefs_set_bool");
	if (!purple_prefs_set_bool_wrapped)
		return false;

	purple_prefs_set_string_wrapped = (purple_prefs_set_string_wrapped_fnc)GetProcAddress(f_hPurple, "purple_prefs_set_string");
	if (!purple_prefs_set_string_wrapped)
		return false;

	purple_notify_user_info_new_wrapped = (purple_notify_user_info_new_wrapped_fnc)GetProcAddress(f_hPurple, "purple_notify_user_info_new");
	if (!purple_notify_user_info_new_wrapped)
		return false;

	purple_notify_user_info_destroy_wrapped = (purple_notify_user_info_destroy_wrapped_fnc)GetProcAddress(f_hPurple, "purple_notify_user_info_destroy");
	if (!purple_notify_user_info_destroy_wrapped)
		return false;

	purple_notify_user_info_get_entries_wrapped = (purple_notify_user_info_get_entries_wrapped_fnc)GetProcAddress(f_hPurple, "purple_notify_user_info_get_entries");
	if (!purple_notify_user_info_get_entries_wrapped)
		return false;

	purple_notify_user_info_entry_get_label_wrapped = (purple_notify_user_info_entry_get_label_wrapped_fnc)GetProcAddress(f_hPurple, "purple_notify_user_info_entry_get_label");
	if (!purple_notify_user_info_entry_get_label_wrapped)
		return false;

	purple_notify_user_info_entry_get_value_wrapped = (purple_notify_user_info_entry_get_value_wrapped_fnc)GetProcAddress(f_hPurple, "purple_notify_user_info_entry_get_value");
	if (!purple_notify_user_info_entry_get_value_wrapped)
		return false;

	purple_notify_set_ui_ops_wrapped = (purple_notify_set_ui_ops_wrapped_fnc)GetProcAddress(f_hPurple, "purple_notify_set_ui_ops");
	if (!purple_notify_set_ui_ops_wrapped)
		return false;

	purple_buddy_icons_set_account_icon_wrapped = (purple_buddy_icons_set_account_icon_wrapped_fnc)GetProcAddress(f_hPurple, "purple_buddy_icons_set_account_icon");
	if (!purple_buddy_icons_set_account_icon_wrapped)
		return false;

	purple_buddy_icons_find_wrapped = (purple_buddy_icons_find_wrapped_fnc)GetProcAddress(f_hPurple, "purple_buddy_icons_find");
	if (!purple_buddy_icons_find_wrapped)
		return false;

	purple_buddy_icon_get_full_path_wrapped = (purple_buddy_icon_get_full_path_wrapped_fnc)GetProcAddress(f_hPurple, "purple_buddy_icon_get_full_path");
	if (!purple_buddy_icon_get_full_path_wrapped)
		return false;

	purple_buddy_icon_unref_wrapped = (purple_buddy_icon_unref_wrapped_fnc)GetProcAddress(f_hPurple, "purple_buddy_icon_unref");
	if (!purple_buddy_icon_unref_wrapped)
		return false;

	purple_buddy_icons_find_account_icon_wrapped = (purple_buddy_icons_find_account_icon_wrapped_fnc)GetProcAddress(f_hPurple, "purple_buddy_icons_find_account_icon");
	if (!purple_buddy_icons_find_account_icon_wrapped)
		return false;

	purple_buddy_icon_get_data_wrapped = (purple_buddy_icon_get_data_wrapped_fnc)GetProcAddress(f_hPurple, "purple_buddy_icon_get_data");
	if (!purple_buddy_icon_get_data_wrapped)
		return false;

	purple_account_set_bool_wrapped = (purple_account_set_bool_wrapped_fnc)GetProcAddress(f_hPurple, "purple_account_set_bool");
	if (!purple_account_set_bool_wrapped)
		return false;

	purple_account_get_protocol_id_wrapped = (purple_account_get_protocol_id_wrapped_fnc)GetProcAddress(f_hPurple, "purple_account_get_protocol_id");
	if (!purple_account_get_protocol_id_wrapped)
		return false;

	purple_account_set_int_wrapped = (purple_account_set_int_wrapped_fnc)GetProcAddress(f_hPurple, "purple_account_set_int");
	if (!purple_account_set_int_wrapped)
		return false;

	purple_account_set_string_wrapped = (purple_account_set_string_wrapped_fnc)GetProcAddress(f_hPurple, "purple_account_set_string");
	if (!purple_account_set_string_wrapped)
		return false;

	purple_account_get_username_wrapped = (purple_account_get_username_wrapped_fnc)GetProcAddress(f_hPurple, "purple_account_get_username");
	if (!purple_account_get_username_wrapped)
		return false;

	purple_account_set_username_wrapped = (purple_account_set_username_wrapped_fnc)GetProcAddress(f_hPurple, "purple_account_set_username");
	if (!purple_account_set_username_wrapped)
		return false;

	purple_accounts_find_wrapped = (purple_accounts_find_wrapped_fnc)GetProcAddress(f_hPurple, "purple_accounts_find");
	if (!purple_accounts_find_wrapped)
		return false;

	purple_account_new_wrapped = (purple_account_new_wrapped_fnc)GetProcAddress(f_hPurple, "purple_account_new");
	if (!purple_account_new_wrapped)
		return false;

	purple_accounts_add_wrapped = (purple_accounts_add_wrapped_fnc)GetProcAddress(f_hPurple, "purple_accounts_add");
	if (!purple_accounts_add_wrapped)
		return false;

	purple_account_set_password_wrapped = (purple_account_set_password_wrapped_fnc)GetProcAddress(f_hPurple, "purple_account_set_password");
	if (!purple_account_set_password_wrapped)
		return false;

	purple_account_set_enabled_wrapped = (purple_account_set_enabled_wrapped_fnc)GetProcAddress(f_hPurple, "purple_account_set_enabled");
	if (!purple_account_set_enabled_wrapped)
		return false;

	purple_account_set_privacy_type_wrapped = (purple_account_set_privacy_type_wrapped_fnc)GetProcAddress(f_hPurple, "purple_account_set_privacy_type");
	if (!purple_account_set_privacy_type_wrapped)
		return false;

	purple_account_get_status_type_with_primitive_wrapped = (purple_account_get_status_type_with_primitive_wrapped_fnc)GetProcAddress(f_hPurple, "purple_account_get_status_type_with_primitive");
	if (!purple_account_get_status_type_with_primitive_wrapped)
		return false;

	purple_account_set_status_wrapped = (purple_account_set_status_wrapped_fnc)GetProcAddress(f_hPurple, "purple_account_set_status");
	if (!purple_account_set_status_wrapped)
		return false;

	purple_account_get_int_wrapped = (purple_account_get_int_wrapped_fnc)GetProcAddress(f_hPurple, "purple_account_get_int");
	if (!purple_account_get_int_wrapped)
		return false;

	purple_account_disconnect_wrapped = (purple_account_disconnect_wrapped_fnc)GetProcAddress(f_hPurple, "purple_account_disconnect");
	if (!purple_account_disconnect_wrapped)
		return false;

	purple_accounts_delete_wrapped = (purple_accounts_delete_wrapped_fnc)GetProcAddress(f_hPurple, "purple_accounts_delete");
	if (!purple_accounts_delete_wrapped)
		return false;

	purple_account_get_connection_wrapped = (purple_account_get_connection_wrapped_fnc)GetProcAddress(f_hPurple, "purple_account_get_connection");
	if (!purple_account_get_connection_wrapped)
		return false;

	purple_account_set_alias_wrapped = (purple_account_set_alias_wrapped_fnc)GetProcAddress(f_hPurple, "purple_account_set_alias");
	if (!purple_account_set_alias_wrapped)
		return false;

	purple_account_set_public_alias_wrapped = (purple_account_set_public_alias_wrapped_fnc)GetProcAddress(f_hPurple, "purple_account_set_public_alias");
	if (!purple_account_set_public_alias_wrapped)
		return false;

	purple_account_remove_buddy_wrapped = (purple_account_remove_buddy_wrapped_fnc)GetProcAddress(f_hPurple, "purple_account_remove_buddy");
	if (!purple_account_remove_buddy_wrapped)
		return false;

	purple_account_add_buddy_wrapped = (purple_account_add_buddy_wrapped_fnc)GetProcAddress(f_hPurple, "purple_account_add_buddy");
	if (!purple_account_add_buddy_wrapped)
		return false;

	purple_account_get_name_for_display_wrapped = (purple_account_get_name_for_display_wrapped_fnc)GetProcAddress(f_hPurple, "purple_account_get_name_for_display");
	if (!purple_account_get_name_for_display_wrapped)
		return false;

	purple_accounts_set_ui_ops_wrapped = (purple_accounts_set_ui_ops_wrapped_fnc)GetProcAddress(f_hPurple, "purple_accounts_set_ui_ops");
	if (!purple_accounts_set_ui_ops_wrapped)
		return false;

	purple_status_type_get_id_wrapped = (purple_status_type_get_id_wrapped_fnc)GetProcAddress(f_hPurple, "purple_status_type_get_id");
	if (!purple_status_type_get_id_wrapped)
		return false;

	purple_presence_get_active_status_wrapped = (purple_presence_get_active_status_wrapped_fnc)GetProcAddress(f_hPurple, "purple_presence_get_active_status");
	if (!purple_presence_get_active_status_wrapped)
		return false;

	purple_status_type_get_primitive_wrapped = (purple_status_type_get_primitive_wrapped_fnc)GetProcAddress(f_hPurple, "purple_status_type_get_primitive");
	if (!purple_status_type_get_primitive_wrapped)
		return false;

	purple_status_get_type_wrapped = (purple_status_get_type_wrapped_fnc)GetProcAddress(f_hPurple, "purple_status_get_type");
	if (!purple_status_get_type_wrapped)
		return false;

	purple_status_get_attr_string_wrapped = (purple_status_get_attr_string_wrapped_fnc)GetProcAddress(f_hPurple, "purple_status_get_attr_string");
	if (!purple_status_get_attr_string_wrapped)
		return false;

	serv_get_info_wrapped = (serv_get_info_wrapped_fnc)GetProcAddress(f_hPurple, "serv_get_info");
	if (!serv_get_info_wrapped)
		return false;

	serv_alias_buddy_wrapped = (serv_alias_buddy_wrapped_fnc)GetProcAddress(f_hPurple, "serv_alias_buddy");
	if (!serv_alias_buddy_wrapped)
		return false;

	serv_send_typing_wrapped = (serv_send_typing_wrapped_fnc)GetProcAddress(f_hPurple, "serv_send_typing");
	if (!serv_send_typing_wrapped)
		return false;

	serv_join_chat_wrapped = (serv_join_chat_wrapped_fnc)GetProcAddress(f_hPurple, "serv_join_chat");
	if (!serv_join_chat_wrapped)
		return false;

	purple_dnsquery_set_ui_ops_wrapped = (purple_dnsquery_set_ui_ops_wrapped_fnc)GetProcAddress(f_hPurple, "purple_dnsquery_set_ui_ops");
	if (!purple_dnsquery_set_ui_ops_wrapped)
		return false;

	purple_conversation_get_im_data_wrapped = (purple_conversation_get_im_data_wrapped_fnc)GetProcAddress(f_hPurple, "purple_conversation_get_im_data");
	if (!purple_conversation_get_im_data_wrapped)
		return false;

	purple_conversation_get_chat_data_wrapped = (purple_conversation_get_chat_data_wrapped_fnc)GetProcAddress(f_hPurple, "purple_conversation_get_chat_data");
	if (!purple_conversation_get_chat_data_wrapped)
		return false;

	purple_find_conversation_with_account_wrapped = (purple_find_conversation_with_account_wrapped_fnc)GetProcAddress(f_hPurple, "purple_find_conversation_with_account");
	if (!purple_find_conversation_with_account_wrapped)
		return false;

	purple_conversation_new_wrapped = (purple_conversation_new_wrapped_fnc)GetProcAddress(f_hPurple, "purple_conversation_new");
	if (!purple_conversation_new_wrapped)
		return false;

	purple_conversation_get_type_wrapped = (purple_conversation_get_type_wrapped_fnc)GetProcAddress(f_hPurple, "purple_conversation_get_type");
	if (!purple_conversation_get_type_wrapped)
		return false;

	purple_conv_im_send_wrapped = (purple_conv_im_send_wrapped_fnc)GetProcAddress(f_hPurple, "purple_conv_im_send");
	if (!purple_conv_im_send_wrapped)
		return false;

	purple_conv_chat_send_wrapped = (purple_conv_chat_send_wrapped_fnc)GetProcAddress(f_hPurple, "purple_conv_chat_send");
	if (!purple_conv_chat_send_wrapped)
		return false;

	purple_conversation_destroy_wrapped = (purple_conversation_destroy_wrapped_fnc)GetProcAddress(f_hPurple, "purple_conversation_destroy");
	if (!purple_conversation_destroy_wrapped)
		return false;

	purple_conversation_get_account_wrapped = (purple_conversation_get_account_wrapped_fnc)GetProcAddress(f_hPurple, "purple_conversation_get_account");
	if (!purple_conversation_get_account_wrapped)
		return false;

	purple_conversation_get_name_wrapped = (purple_conversation_get_name_wrapped_fnc)GetProcAddress(f_hPurple, "purple_conversation_get_name");
	if (!purple_conversation_get_name_wrapped)
		return false;

	purple_conversations_set_ui_ops_wrapped = (purple_conversations_set_ui_ops_wrapped_fnc)GetProcAddress(f_hPurple, "purple_conversations_set_ui_ops");
	if (!purple_conversations_set_ui_ops_wrapped)
		return false;

	purple_conversations_get_handle_wrapped = (purple_conversations_get_handle_wrapped_fnc)GetProcAddress(f_hPurple, "purple_conversations_get_handle");
	if (!purple_conversations_get_handle_wrapped)
		return false;

	purple_plugin_action_free_wrapped = (purple_plugin_action_free_wrapped_fnc)GetProcAddress(f_hPurple, "purple_plugin_action_free");
	if (!purple_plugin_action_free_wrapped)
		return false;

	purple_plugins_add_search_path_wrapped = (purple_plugins_add_search_path_wrapped_fnc)GetProcAddress(f_hPurple, "purple_plugins_add_search_path");
	if (!purple_plugins_add_search_path_wrapped)
		return false;

	purple_connection_get_state_wrapped = (purple_connection_get_state_wrapped_fnc)GetProcAddress(f_hPurple, "purple_connection_get_state");
	if (!purple_connection_get_state_wrapped)
		return false;

	purple_connection_get_account_wrapped = (purple_connection_get_account_wrapped_fnc)GetProcAddress(f_hPurple, "purple_connection_get_account");
	if (!purple_connection_get_account_wrapped)
		return false;

	purple_connection_get_display_name_wrapped = (purple_connection_get_display_name_wrapped_fnc)GetProcAddress(f_hPurple, "purple_connection_get_display_name");
	if (!purple_connection_get_display_name_wrapped)
		return false;

	purple_connections_set_ui_ops_wrapped = (purple_connections_set_ui_ops_wrapped_fnc)GetProcAddress(f_hPurple, "purple_connections_set_ui_ops");
	if (!purple_connections_set_ui_ops_wrapped)
		return false;

	purple_connections_get_handle_wrapped = (purple_connections_get_handle_wrapped_fnc)GetProcAddress(f_hPurple, "purple_connections_get_handle");
	if (!purple_connections_get_handle_wrapped)
		return false;

	purple_core_set_ui_ops_wrapped = (purple_core_set_ui_ops_wrapped_fnc)GetProcAddress(f_hPurple, "purple_core_set_ui_ops");
	if (!purple_core_set_ui_ops_wrapped)
		return false;

	purple_core_init_wrapped = (purple_core_init_wrapped_fnc)GetProcAddress(f_hPurple, "purple_core_init");
	if (!purple_core_init_wrapped)
		return false;

	purple_input_add_wrapped = (purple_input_add_wrapped_fnc)GetProcAddress(f_hPurple, "purple_input_add");
	if (!purple_input_add_wrapped)
		return false;

	purple_timeout_add_wrapped = (purple_timeout_add_wrapped_fnc)GetProcAddress(f_hPurple, "purple_timeout_add");
	if (!purple_timeout_add_wrapped)
		return false;

	purple_timeout_add_seconds_wrapped = (purple_timeout_add_seconds_wrapped_fnc)GetProcAddress(f_hPurple, "purple_timeout_add_seconds");
	if (!purple_timeout_add_seconds_wrapped)
		return false;

	purple_timeout_remove_wrapped = (purple_timeout_remove_wrapped_fnc)GetProcAddress(f_hPurple, "purple_timeout_remove");
	if (!purple_timeout_remove_wrapped)
		return false;

	purple_eventloop_set_ui_ops_wrapped = (purple_eventloop_set_ui_ops_wrapped_fnc)GetProcAddress(f_hPurple, "purple_eventloop_set_ui_ops");
	if (!purple_eventloop_set_ui_ops_wrapped)
		return false;

	purple_input_remove_wrapped = (purple_input_remove_wrapped_fnc)GetProcAddress(f_hPurple, "purple_input_remove");
	if (!purple_input_remove_wrapped)
		return false;

	purple_privacy_deny_wrapped = (purple_privacy_deny_wrapped_fnc)GetProcAddress(f_hPurple, "purple_privacy_deny");
	if (!purple_privacy_deny_wrapped)
		return false;

	purple_privacy_allow_wrapped = (purple_privacy_allow_wrapped_fnc)GetProcAddress(f_hPurple, "purple_privacy_allow");
	if (!purple_privacy_allow_wrapped)
		return false;

	purple_privacy_check_wrapped = (purple_privacy_check_wrapped_fnc)GetProcAddress(f_hPurple, "purple_privacy_check");
	if (!purple_privacy_check_wrapped)
		return false;

	purple_find_prpl_wrapped = (purple_find_prpl_wrapped_fnc)GetProcAddress(f_hPurple, "purple_find_prpl");
	if (!purple_find_prpl_wrapped)
		return false;

	purple_prpl_send_attention_wrapped = (purple_prpl_send_attention_wrapped_fnc)GetProcAddress(f_hPurple, "purple_prpl_send_attention");
	if (!purple_prpl_send_attention_wrapped)
		return false;

	purple_account_option_get_type_wrapped = (purple_account_option_get_type_wrapped_fnc)GetProcAddress(f_hPurple, "purple_account_option_get_type");
	if (!purple_account_option_get_type_wrapped)
		return false;

	purple_account_option_get_setting_wrapped = (purple_account_option_get_setting_wrapped_fnc)GetProcAddress(f_hPurple, "purple_account_option_get_setting");
	if (!purple_account_option_get_setting_wrapped)
		return false;

	wpurple_g_io_channel_win32_new_socket_wrapped = (wpurple_g_io_channel_win32_new_socket_wrapped_fnc)GetProcAddress(f_hPurple, "wpurple_g_io_channel_win32_new_socket");
	if (!wpurple_g_io_channel_win32_new_socket_wrapped)
		return false;

#endif
	return true;
}

