#pragma once
#if PURPLE_RUNTIME

#include <Windows.h>
#include <purple.h>

#define PURPLE_BLIST_NODE_IS_CHAT_WRAPPED(n)    (purple_blist_node_get_type_wrapped(n) == PURPLE_BLIST_CHAT_NODE)
#define PURPLE_BLIST_NODE_IS_BUDDY_WRAPPED(n)   (purple_blist_node_get_type_wrapped(n) == PURPLE_BLIST_BUDDY_NODE)
#define PURPLE_BLIST_NODE_IS_CONTACT_WRAPPED(n) (purple_blist_node_get_type_wrapped(n) == PURPLE_BLIST_CONTACT_NODE)
#define PURPLE_BLIST_NODE_IS_GROUP_WRAPPED(n)   (purple_blist_node_get_type_wrapped(n) == PURPLE_BLIST_GROUP_NODE)

#define PURPLE_CONV_IM_WRAPPED(c) (purple_conversation_get_im_data_wrapped(c))
#define PURPLE_CONV_CHAT_WRAPPED(c) (purple_conversation_get_chat_data_wrapped(c))

#define PURPLE_CONNECTION_IS_CONNECTED_WRAPPED(gc) 	(purple_connection_get_state_wrapped(gc) == PURPLE_CONNECTED)

typedef void  (_cdecl * purple_account_set_bool_wrapped_fnc)(PurpleAccount *account, const char *name, gboolean value);
extern purple_account_set_bool_wrapped_fnc purple_account_set_bool_wrapped;

typedef const char * (_cdecl * purple_account_get_protocol_id_wrapped_fnc)(const PurpleAccount *account);
extern purple_account_get_protocol_id_wrapped_fnc purple_account_get_protocol_id_wrapped;

typedef void  (_cdecl * purple_account_set_int_wrapped_fnc)(PurpleAccount *account, const char *name, int value);
extern purple_account_set_int_wrapped_fnc purple_account_set_int_wrapped;

typedef const char * (_cdecl * purple_account_get_string_wrapped_fnc)(PurpleAccount *account, const char *name, const char *default_value);
extern purple_account_get_string_wrapped_fnc purple_account_get_string_wrapped;

typedef void  (_cdecl * purple_account_set_string_wrapped_fnc)(PurpleAccount *account, const char *name, const char *value);
extern purple_account_set_string_wrapped_fnc purple_account_set_string_wrapped;

typedef const char * (_cdecl * purple_account_get_username_wrapped_fnc)(const PurpleAccount *account);
extern purple_account_get_username_wrapped_fnc purple_account_get_username_wrapped;

typedef void  (_cdecl * purple_account_set_username_wrapped_fnc)(PurpleAccount *account, const char *username);
extern purple_account_set_username_wrapped_fnc purple_account_set_username_wrapped;

typedef void  (_cdecl * purple_account_set_proxy_info_wrapped_fnc)(PurpleAccount *account, PurpleProxyInfo *info);
extern purple_account_set_proxy_info_wrapped_fnc purple_account_set_proxy_info_wrapped;

typedef PurpleAccount * (_cdecl * purple_accounts_find_wrapped_fnc)(const char *name, const char *protocol);
extern purple_accounts_find_wrapped_fnc purple_accounts_find_wrapped;

typedef PurpleAccount * (_cdecl * purple_account_new_wrapped_fnc)(const char *username, const char *protocol_id);
extern purple_account_new_wrapped_fnc purple_account_new_wrapped;

typedef void  (_cdecl * purple_accounts_add_wrapped_fnc)(PurpleAccount *account);
extern purple_accounts_add_wrapped_fnc purple_accounts_add_wrapped;

typedef const char *  (_cdecl * purple_account_get_password_wrapped_fnc)(const PurpleAccount *account);
extern purple_account_get_password_wrapped_fnc purple_account_get_password_wrapped;

typedef void  (_cdecl * purple_account_set_password_wrapped_fnc)(PurpleAccount *account, const char *password);
extern purple_account_set_password_wrapped_fnc purple_account_set_password_wrapped;

typedef void  (_cdecl * purple_account_set_enabled_wrapped_fnc)(PurpleAccount *account, const char *ui, gboolean value);
extern purple_account_set_enabled_wrapped_fnc purple_account_set_enabled_wrapped;

typedef void  (_cdecl * purple_account_set_privacy_type_wrapped_fnc)(PurpleAccount *account, PurplePrivacyType privacy_type);
extern purple_account_set_privacy_type_wrapped_fnc purple_account_set_privacy_type_wrapped;

typedef PurpleStatusType * (_cdecl * purple_account_get_status_type_with_primitive_wrapped_fnc)( const PurpleAccount *account, PurpleStatusPrimitive primitive);
extern purple_account_get_status_type_with_primitive_wrapped_fnc purple_account_get_status_type_with_primitive_wrapped;

typedef void  (_cdecl * purple_account_set_status_wrapped_fnc)(PurpleAccount *account, const char *status_id, gboolean active, ...);
extern purple_account_set_status_wrapped_fnc purple_account_set_status_wrapped;

typedef int  (_cdecl * purple_account_get_int_wrapped_fnc)(const PurpleAccount *account, const char *name, int default_value);
extern purple_account_get_int_wrapped_fnc purple_account_get_int_wrapped;

typedef void  (_cdecl * purple_account_disconnect_wrapped_fnc)(PurpleAccount *account);
extern purple_account_disconnect_wrapped_fnc purple_account_disconnect_wrapped;

typedef void  (_cdecl * purple_accounts_delete_wrapped_fnc)(PurpleAccount *account);
extern purple_accounts_delete_wrapped_fnc purple_accounts_delete_wrapped;

typedef PurpleConnection * (_cdecl * purple_account_get_connection_wrapped_fnc)(const PurpleAccount *account);
extern purple_account_get_connection_wrapped_fnc purple_account_get_connection_wrapped;

typedef void  (_cdecl * purple_account_set_alias_wrapped_fnc)(PurpleAccount *account, const char *alias);
extern purple_account_set_alias_wrapped_fnc purple_account_set_alias_wrapped;

typedef void  (_cdecl * purple_account_set_public_alias_wrapped_fnc)(PurpleAccount *account, const char *alias, PurpleSetPublicAliasSuccessCallback success_cb, PurpleSetPublicAliasFailureCallback failure_cb);
extern purple_account_set_public_alias_wrapped_fnc purple_account_set_public_alias_wrapped;

typedef void  (_cdecl * purple_account_remove_buddy_wrapped_fnc)(PurpleAccount *account, PurpleBuddy *buddy, PurpleGroup *group);
extern purple_account_remove_buddy_wrapped_fnc purple_account_remove_buddy_wrapped;

typedef void  (_cdecl * purple_account_add_buddy_wrapped_fnc)(PurpleAccount *account, PurpleBuddy *buddy);
extern purple_account_add_buddy_wrapped_fnc purple_account_add_buddy_wrapped;

typedef const gchar * (_cdecl * purple_account_get_name_for_display_wrapped_fnc)(const PurpleAccount *account);
extern purple_account_get_name_for_display_wrapped_fnc purple_account_get_name_for_display_wrapped;

typedef void  (_cdecl * purple_accounts_set_ui_ops_wrapped_fnc)(PurpleAccountUiOps *ops);
extern purple_accounts_set_ui_ops_wrapped_fnc purple_accounts_set_ui_ops_wrapped;

typedef PurplePrefType  (_cdecl * purple_account_option_get_type_wrapped_fnc)(const PurpleAccountOption *option);
extern purple_account_option_get_type_wrapped_fnc purple_account_option_get_type_wrapped;

typedef const char * (_cdecl * purple_account_option_get_setting_wrapped_fnc)(const PurpleAccountOption *option);
extern purple_account_option_get_setting_wrapped_fnc purple_account_option_get_setting_wrapped;

typedef PurpleBlistNodeType  (_cdecl * purple_blist_node_get_type_wrapped_fnc)(PurpleBlistNode *node);
extern purple_blist_node_get_type_wrapped_fnc purple_blist_node_get_type_wrapped;

typedef const char * (_cdecl * purple_buddy_get_alias_wrapped_fnc)(PurpleBuddy *buddy);
extern purple_buddy_get_alias_wrapped_fnc purple_buddy_get_alias_wrapped;

typedef const char * (_cdecl * purple_buddy_get_server_alias_wrapped_fnc)(PurpleBuddy *buddy);
extern purple_buddy_get_server_alias_wrapped_fnc purple_buddy_get_server_alias_wrapped;

typedef PurpleBuddy * (_cdecl * purple_find_buddy_wrapped_fnc)(PurpleAccount *account, const char *name);
extern purple_find_buddy_wrapped_fnc purple_find_buddy_wrapped;

typedef PurpleGroup * (_cdecl * purple_buddy_get_group_wrapped_fnc)(PurpleBuddy *buddy);
extern purple_buddy_get_group_wrapped_fnc purple_buddy_get_group_wrapped;

typedef void  (_cdecl * purple_blist_remove_buddy_wrapped_fnc)(PurpleBuddy *buddy);
extern purple_blist_remove_buddy_wrapped_fnc purple_blist_remove_buddy_wrapped;

typedef void  (_cdecl * purple_blist_alias_buddy_wrapped_fnc)(PurpleBuddy *buddy, const char *alias);
extern purple_blist_alias_buddy_wrapped_fnc purple_blist_alias_buddy_wrapped;

typedef void  (_cdecl * purple_blist_server_alias_buddy_wrapped_fnc)(PurpleBuddy *buddy, const char *alias);
extern purple_blist_server_alias_buddy_wrapped_fnc purple_blist_server_alias_buddy_wrapped;

typedef PurpleGroup * (_cdecl * purple_find_group_wrapped_fnc)(const char *name);
extern purple_find_group_wrapped_fnc purple_find_group_wrapped;

typedef PurpleGroup * (_cdecl * purple_group_new_wrapped_fnc)(const char *name);
extern purple_group_new_wrapped_fnc purple_group_new_wrapped;

typedef void  (_cdecl * purple_blist_add_contact_wrapped_fnc)(PurpleContact *contact, PurpleGroup *group, PurpleBlistNode *node);
extern purple_blist_add_contact_wrapped_fnc purple_blist_add_contact_wrapped;

typedef PurpleContact * (_cdecl * purple_buddy_get_contact_wrapped_fnc)(PurpleBuddy *buddy);
extern purple_buddy_get_contact_wrapped_fnc purple_buddy_get_contact_wrapped;

typedef PurpleBuddy * (_cdecl * purple_buddy_new_wrapped_fnc)(PurpleAccount *account, const char *name, const char *alias);
extern purple_buddy_new_wrapped_fnc purple_buddy_new_wrapped;

typedef void  (_cdecl * purple_blist_add_buddy_wrapped_fnc)(PurpleBuddy *buddy, PurpleContact *contact, PurpleGroup *group, PurpleBlistNode *node);
extern purple_blist_add_buddy_wrapped_fnc purple_blist_add_buddy_wrapped;

typedef PurpleChat * (_cdecl * purple_blist_find_chat_wrapped_fnc)(PurpleAccount *account, const char *name);
extern purple_blist_find_chat_wrapped_fnc purple_blist_find_chat_wrapped;

typedef GHashTable * (_cdecl * purple_chat_get_components_wrapped_fnc)(PurpleChat *chat);
extern purple_chat_get_components_wrapped_fnc purple_chat_get_components_wrapped;

typedef PurplePresence * (_cdecl * purple_buddy_get_presence_wrapped_fnc)(const PurpleBuddy *buddy);
extern purple_buddy_get_presence_wrapped_fnc purple_buddy_get_presence_wrapped;

typedef PurpleAccount * (_cdecl * purple_buddy_get_account_wrapped_fnc)(const PurpleBuddy *buddy);
extern purple_buddy_get_account_wrapped_fnc purple_buddy_get_account_wrapped;

typedef const char * (_cdecl * purple_buddy_get_name_wrapped_fnc)(const PurpleBuddy *buddy);
extern purple_buddy_get_name_wrapped_fnc purple_buddy_get_name_wrapped;

typedef GSList * (_cdecl * purple_find_buddies_wrapped_fnc)(PurpleAccount *account, const char *name);
extern purple_find_buddies_wrapped_fnc purple_find_buddies_wrapped;

typedef const char * (_cdecl * purple_group_get_name_wrapped_fnc)(PurpleGroup *group);
extern purple_group_get_name_wrapped_fnc purple_group_get_name_wrapped;

typedef void  (_cdecl * purple_blist_set_ui_ops_wrapped_fnc)(PurpleBlistUiOps *ops);
extern purple_blist_set_ui_ops_wrapped_fnc purple_blist_set_ui_ops_wrapped;

typedef void  (_cdecl * purple_set_blist_wrapped_fnc)(PurpleBuddyList *blist);
extern purple_set_blist_wrapped_fnc purple_set_blist_wrapped;

typedef PurpleBuddyList * (_cdecl * purple_blist_new_wrapped_fnc)(void);
extern purple_blist_new_wrapped_fnc purple_blist_new_wrapped;

typedef void  (_cdecl * purple_blist_load_wrapped_fnc)(void);
extern purple_blist_load_wrapped_fnc purple_blist_load_wrapped;

typedef void * (_cdecl * purple_blist_get_handle_wrapped_fnc)(void);
extern purple_blist_get_handle_wrapped_fnc purple_blist_get_handle_wrapped;

typedef PurpleStoredImage * (_cdecl * purple_buddy_icons_set_account_icon_wrapped_fnc)(PurpleAccount *account, guchar *icon_data, size_t icon_len);
extern purple_buddy_icons_set_account_icon_wrapped_fnc purple_buddy_icons_set_account_icon_wrapped;

typedef PurpleBuddyIcon * (_cdecl * purple_buddy_icons_find_wrapped_fnc)(PurpleAccount *account, const char *username);
extern purple_buddy_icons_find_wrapped_fnc purple_buddy_icons_find_wrapped;

typedef char * (_cdecl * purple_buddy_icon_get_full_path_wrapped_fnc)(PurpleBuddyIcon *icon);
extern purple_buddy_icon_get_full_path_wrapped_fnc purple_buddy_icon_get_full_path_wrapped;

typedef PurpleBuddyIcon * (_cdecl * purple_buddy_icon_unref_wrapped_fnc)(PurpleBuddyIcon *icon);
extern purple_buddy_icon_unref_wrapped_fnc purple_buddy_icon_unref_wrapped;

typedef PurpleStoredImage * (_cdecl * purple_buddy_icons_find_account_icon_wrapped_fnc)(PurpleAccount *account);
extern purple_buddy_icons_find_account_icon_wrapped_fnc purple_buddy_icons_find_account_icon_wrapped;

typedef gconstpointer  (_cdecl * purple_buddy_icon_get_data_wrapped_fnc)(const PurpleBuddyIcon *icon, size_t *len);
extern purple_buddy_icon_get_data_wrapped_fnc purple_buddy_icon_get_data_wrapped;

typedef void  (_cdecl * purple_certificate_add_ca_search_path_wrapped_fnc)(const char *path);
extern purple_certificate_add_ca_search_path_wrapped_fnc purple_certificate_add_ca_search_path_wrapped;

typedef PurpleConnectionState  (_cdecl * purple_connection_get_state_wrapped_fnc)(const PurpleConnection *gc);
extern purple_connection_get_state_wrapped_fnc purple_connection_get_state_wrapped;

typedef PurpleAccount * (_cdecl * purple_connection_get_account_wrapped_fnc)(const PurpleConnection *gc);
extern purple_connection_get_account_wrapped_fnc purple_connection_get_account_wrapped;

typedef const char * (_cdecl * purple_connection_get_display_name_wrapped_fnc)(const PurpleConnection *gc);
extern purple_connection_get_display_name_wrapped_fnc purple_connection_get_display_name_wrapped;

typedef void  (_cdecl * purple_connections_set_ui_ops_wrapped_fnc)(PurpleConnectionUiOps *ops);
extern purple_connections_set_ui_ops_wrapped_fnc purple_connections_set_ui_ops_wrapped;

typedef void * (_cdecl * purple_connections_get_handle_wrapped_fnc)(void);
extern purple_connections_get_handle_wrapped_fnc purple_connections_get_handle_wrapped;

typedef PurpleConvIm * (_cdecl * purple_conversation_get_im_data_wrapped_fnc)(const PurpleConversation *conv);
extern purple_conversation_get_im_data_wrapped_fnc purple_conversation_get_im_data_wrapped;

typedef PurpleConvChat * (_cdecl * purple_conversation_get_chat_data_wrapped_fnc)(const PurpleConversation *conv);
extern purple_conversation_get_chat_data_wrapped_fnc purple_conversation_get_chat_data_wrapped;

typedef PurpleConversation * (_cdecl * purple_find_conversation_with_account_wrapped_fnc)( PurpleConversationType type, const char *name, const PurpleAccount *account);
extern purple_find_conversation_with_account_wrapped_fnc purple_find_conversation_with_account_wrapped;

typedef PurpleConversation * (_cdecl * purple_conversation_new_wrapped_fnc)(PurpleConversationType type, PurpleAccount *account, const char *name);
extern purple_conversation_new_wrapped_fnc purple_conversation_new_wrapped;

typedef PurpleConversationType  (_cdecl * purple_conversation_get_type_wrapped_fnc)(const PurpleConversation *conv);
extern purple_conversation_get_type_wrapped_fnc purple_conversation_get_type_wrapped;

typedef void (_cdecl * purple_conversation_set_data_wrapped_func)(const PurpleConversation *conv, const char *key, gpointer data); 
extern purple_conversation_set_data_wrapped_func purple_conversation_set_data_wrapped;

typedef void (_cdecl * purple_conversation_update_wrapped_func)(const PurpleConversation *conv, PurpleConversationUpdateType type); 
extern purple_conversation_update_wrapped_func purple_conversation_update_wrapped;

typedef void  (_cdecl * purple_conv_im_send_wrapped_fnc)(PurpleConvIm *im, const char *message);
extern purple_conv_im_send_wrapped_fnc purple_conv_im_send_wrapped;

typedef void  (_cdecl * purple_conv_im_send_with_flags_wrapped_fnc)(PurpleConvIm *im, const char *message, PurpleMessageFlags flags);
extern purple_conv_im_send_with_flags_wrapped_fnc purple_conv_im_send_with_flags_wrapped;

typedef void  (_cdecl * purple_conv_chat_send_wrapped_fnc)(PurpleConvChat *chat, const char *message);
extern purple_conv_chat_send_wrapped_fnc purple_conv_chat_send_wrapped;

typedef void  (_cdecl * purple_conv_chat_send_with_flags_wrapped_fnc)(PurpleConvChat *chat, const char *message, PurpleMessageFlags flags);
extern purple_conv_chat_send_with_flags_wrapped_fnc purple_conv_chat_send_with_flags_wrapped;

typedef void  (_cdecl * purple_conversation_destroy_wrapped_fnc)(PurpleConversation *conv);
extern purple_conversation_destroy_wrapped_fnc purple_conversation_destroy_wrapped;

typedef PurpleAccount * (_cdecl * purple_conversation_get_account_wrapped_fnc)(const PurpleConversation *conv);
extern purple_conversation_get_account_wrapped_fnc purple_conversation_get_account_wrapped;

typedef const char * (_cdecl * purple_conversation_get_name_wrapped_fnc)(const PurpleConversation *conv);
extern purple_conversation_get_name_wrapped_fnc purple_conversation_get_name_wrapped;

typedef void  (_cdecl * purple_conversations_set_ui_ops_wrapped_fnc)(PurpleConversationUiOps *ops);
extern purple_conversations_set_ui_ops_wrapped_fnc purple_conversations_set_ui_ops_wrapped;

typedef void * (_cdecl * purple_conversations_get_handle_wrapped_fnc)(void);
extern purple_conversations_get_handle_wrapped_fnc purple_conversations_get_handle_wrapped;

typedef void  (_cdecl * purple_core_set_ui_ops_wrapped_fnc)(PurpleCoreUiOps *ops);
extern purple_core_set_ui_ops_wrapped_fnc purple_core_set_ui_ops_wrapped;

typedef gboolean  (_cdecl * purple_core_init_wrapped_fnc)(const char *ui);
extern purple_core_init_wrapped_fnc purple_core_init_wrapped;

typedef void  (_cdecl * purple_debug_set_ui_ops_wrapped_fnc)(PurpleDebugUiOps *ops);
extern purple_debug_set_ui_ops_wrapped_fnc purple_debug_set_ui_ops_wrapped;

typedef void  (_cdecl * purple_debug_set_verbose_wrapped_fnc)(gboolean verbose);
extern purple_debug_set_verbose_wrapped_fnc purple_debug_set_verbose_wrapped;

typedef void  (_cdecl * purple_dnsquery_set_ui_ops_wrapped_fnc)(PurpleDnsQueryUiOps *ops);
extern purple_dnsquery_set_ui_ops_wrapped_fnc purple_dnsquery_set_ui_ops_wrapped;

typedef gboolean  (_cdecl * purple_timeout_remove_wrapped_fnc)(guint handle);
extern purple_timeout_remove_wrapped_fnc purple_timeout_remove_wrapped;

typedef guint  (_cdecl * purple_input_add_wrapped_fnc)(int fd, PurpleInputCondition cond, PurpleInputFunction func, gpointer user_data);
extern purple_input_add_wrapped_fnc purple_input_add_wrapped;

typedef guint  (_cdecl * purple_timeout_add_wrapped_fnc)(guint interval, GSourceFunc function, gpointer data);
extern purple_timeout_add_wrapped_fnc purple_timeout_add_wrapped;

typedef guint  (_cdecl * purple_timeout_add_seconds_wrapped_fnc)(guint interval, GSourceFunc function, gpointer data);
extern purple_timeout_add_seconds_wrapped_fnc purple_timeout_add_seconds_wrapped;

typedef void  (_cdecl * purple_eventloop_set_ui_ops_wrapped_fnc)(PurpleEventLoopUiOps *ops);
extern purple_eventloop_set_ui_ops_wrapped_fnc purple_eventloop_set_ui_ops_wrapped;

typedef gboolean  (_cdecl * purple_input_remove_wrapped_fnc)(guint handle);
extern purple_input_remove_wrapped_fnc purple_input_remove_wrapped;

typedef void  (_cdecl * purple_xfer_ui_ready_wrapped_fnc)(PurpleXfer *xfer);
extern purple_xfer_ui_ready_wrapped_fnc purple_xfer_ui_ready_wrapped;

typedef void  (_cdecl * purple_xfer_request_accepted_wrapped_fnc)(PurpleXfer *xfer, const char *filename);
extern purple_xfer_request_accepted_wrapped_fnc purple_xfer_request_accepted_wrapped;

typedef void  (_cdecl * purple_xfer_request_denied_wrapped_fnc)(PurpleXfer *xfer);
extern purple_xfer_request_denied_wrapped_fnc purple_xfer_request_denied_wrapped;

typedef PurpleAccount * (_cdecl * purple_xfer_get_account_wrapped_fnc)(const PurpleXfer *xfer);
extern purple_xfer_get_account_wrapped_fnc purple_xfer_get_account_wrapped;

typedef const char * (_cdecl * purple_xfer_get_filename_wrapped_fnc)(const PurpleXfer *xfer);
extern purple_xfer_get_filename_wrapped_fnc purple_xfer_get_filename_wrapped;

typedef size_t  (_cdecl * purple_xfer_get_size_wrapped_fnc)(const PurpleXfer *xfer);
extern purple_xfer_get_size_wrapped_fnc purple_xfer_get_size_wrapped;

typedef void  (_cdecl * purple_xfer_unref_wrapped_fnc)(PurpleXfer *xfer);
extern purple_xfer_unref_wrapped_fnc purple_xfer_unref_wrapped;

typedef void  (_cdecl * purple_xfer_ref_wrapped_fnc)(PurpleXfer *xfer);
extern purple_xfer_ref_wrapped_fnc purple_xfer_ref_wrapped;

typedef void  (_cdecl * purple_xfers_set_ui_ops_wrapped_fnc)(PurpleXferUiOps *ops);
extern purple_xfers_set_ui_ops_wrapped_fnc purple_xfers_set_ui_ops_wrapped;

typedef void * (_cdecl * purple_xfers_get_handle_wrapped_fnc)(void);
extern purple_xfers_get_handle_wrapped_fnc purple_xfers_get_handle_wrapped;

typedef void (_cdecl * purple_roomlist_set_ui_ops_wrapped_fnc)(PurpleRoomlistUiOps *ops);
extern purple_roomlist_set_ui_ops_wrapped_fnc purple_roomlist_set_ui_ops_wrapped;

typedef PurpleRoomlist * (_cdecl * purple_roomlist_get_list_wrapped_fnc)(PurpleConnection *con);
extern purple_roomlist_get_list_wrapped_fnc purple_roomlist_get_list_wrapped;

typedef gconstpointer  (_cdecl * purple_imgstore_get_data_wrapped_fnc)(PurpleStoredImage *img);
extern purple_imgstore_get_data_wrapped_fnc purple_imgstore_get_data_wrapped;

typedef size_t  (_cdecl * purple_imgstore_get_size_wrapped_fnc)(PurpleStoredImage *img);
extern purple_imgstore_get_size_wrapped_fnc purple_imgstore_get_size_wrapped;

typedef PurpleStoredImage * (_cdecl * purple_imgstore_unref_wrapped_fnc)(PurpleStoredImage *img);
extern purple_imgstore_unref_wrapped_fnc purple_imgstore_unref_wrapped;

typedef PurpleNotifyUserInfo * (_cdecl * purple_notify_user_info_new_wrapped_fnc)(void);
extern purple_notify_user_info_new_wrapped_fnc purple_notify_user_info_new_wrapped;

typedef void  (_cdecl * purple_notify_user_info_destroy_wrapped_fnc)(PurpleNotifyUserInfo *user_info);
extern purple_notify_user_info_destroy_wrapped_fnc purple_notify_user_info_destroy_wrapped;

typedef GList * (_cdecl * purple_notify_user_info_get_entries_wrapped_fnc)(PurpleNotifyUserInfo *user_info);
extern purple_notify_user_info_get_entries_wrapped_fnc purple_notify_user_info_get_entries_wrapped;

typedef const gchar * (_cdecl * purple_notify_user_info_entry_get_label_wrapped_fnc)(PurpleNotifyUserInfoEntry *user_info_entry);
extern purple_notify_user_info_entry_get_label_wrapped_fnc purple_notify_user_info_entry_get_label_wrapped;

typedef const gchar * (_cdecl * purple_notify_user_info_entry_get_value_wrapped_fnc)(PurpleNotifyUserInfoEntry *user_info_entry);
extern purple_notify_user_info_entry_get_value_wrapped_fnc purple_notify_user_info_entry_get_value_wrapped;

typedef void  (_cdecl * purple_notify_set_ui_ops_wrapped_fnc)(PurpleNotifyUiOps *ops);
extern purple_notify_set_ui_ops_wrapped_fnc purple_notify_set_ui_ops_wrapped;

typedef void  (_cdecl * purple_plugins_add_search_path_wrapped_fnc)(const char *path);
extern purple_plugins_add_search_path_wrapped_fnc purple_plugins_add_search_path_wrapped;

typedef void  (_cdecl * purple_plugin_action_free_wrapped_fnc)(PurplePluginAction *action);
extern purple_plugin_action_free_wrapped_fnc purple_plugin_action_free_wrapped;

typedef gboolean  (_cdecl * purple_prefs_load_wrapped_fnc)(void);
extern purple_prefs_load_wrapped_fnc purple_prefs_load_wrapped;

typedef void  (_cdecl * purple_prefs_set_bool_wrapped_fnc)(const char *name, gboolean value);
extern purple_prefs_set_bool_wrapped_fnc purple_prefs_set_bool_wrapped;

typedef void  (_cdecl * purple_prefs_set_string_wrapped_fnc)(const char *name, const char *value);
extern purple_prefs_set_string_wrapped_fnc purple_prefs_set_string_wrapped;

typedef void  (_cdecl * purple_privacy_deny_wrapped_fnc)(PurpleAccount *account, const char *who, gboolean local, gboolean restore);
extern purple_privacy_deny_wrapped_fnc purple_privacy_deny_wrapped;

typedef void  (_cdecl * purple_privacy_allow_wrapped_fnc)(PurpleAccount *account, const char *who, gboolean local, gboolean restore);
extern purple_privacy_allow_wrapped_fnc purple_privacy_allow_wrapped;

typedef gboolean  (_cdecl * purple_privacy_check_wrapped_fnc)(PurpleAccount *account, const char *who);
extern purple_privacy_check_wrapped_fnc purple_privacy_check_wrapped;

typedef PurpleProxyInfo * (_cdecl * purple_proxy_info_new_wrapped_fnc)(void);
extern purple_proxy_info_new_wrapped_fnc purple_proxy_info_new_wrapped;

typedef void  (_cdecl * purple_proxy_info_set_type_wrapped_fnc)(PurpleProxyInfo *info, PurpleProxyType type);
extern purple_proxy_info_set_type_wrapped_fnc purple_proxy_info_set_type_wrapped;

typedef void  (_cdecl * purple_proxy_info_set_host_wrapped_fnc)(PurpleProxyInfo *info, const char *host);
extern purple_proxy_info_set_host_wrapped_fnc purple_proxy_info_set_host_wrapped;

typedef void  (_cdecl * purple_proxy_info_set_port_wrapped_fnc)(PurpleProxyInfo *info, int port);
extern purple_proxy_info_set_port_wrapped_fnc purple_proxy_info_set_port_wrapped;

typedef void  (_cdecl * purple_proxy_info_set_username_wrapped_fnc)(PurpleProxyInfo *info, const char *username);
extern purple_proxy_info_set_username_wrapped_fnc purple_proxy_info_set_username_wrapped;

typedef void  (_cdecl * purple_proxy_info_set_password_wrapped_fnc)(PurpleProxyInfo *info, const char *password);
extern purple_proxy_info_set_password_wrapped_fnc purple_proxy_info_set_password_wrapped;

typedef PurplePlugin * (_cdecl * purple_find_prpl_wrapped_fnc)(const char *id);
extern purple_find_prpl_wrapped_fnc purple_find_prpl_wrapped;

typedef void  (_cdecl * purple_prpl_send_attention_wrapped_fnc)(PurpleConnection *gc, const char *who, guint type_code);
extern purple_prpl_send_attention_wrapped_fnc purple_prpl_send_attention_wrapped;

typedef void  (_cdecl * purple_request_set_ui_ops_wrapped_fnc)(PurpleRequestUiOps *ops);
extern purple_request_set_ui_ops_wrapped_fnc purple_request_set_ui_ops_wrapped;

typedef void  (_cdecl * serv_get_info_wrapped_fnc)(PurpleConnection *, const char *);
extern serv_get_info_wrapped_fnc serv_get_info_wrapped;

typedef void  (_cdecl * serv_alias_buddy_wrapped_fnc)(PurpleBuddy *);
extern serv_alias_buddy_wrapped_fnc serv_alias_buddy_wrapped;

typedef unsigned int  (_cdecl * serv_send_typing_wrapped_fnc)(PurpleConnection *gc, const char *name, PurpleTypingState state);
extern serv_send_typing_wrapped_fnc serv_send_typing_wrapped;

typedef void  (_cdecl * serv_join_chat_wrapped_fnc)(PurpleConnection *, GHashTable *data);
extern serv_join_chat_wrapped_fnc serv_join_chat_wrapped;

typedef gulong  (_cdecl * purple_signal_connect_wrapped_fnc)(void *instance, const char *signal, void *handle, PurpleCallback func, void *data);
extern purple_signal_connect_wrapped_fnc purple_signal_connect_wrapped;

typedef const char * (_cdecl * purple_status_type_get_id_wrapped_fnc)(const PurpleStatusType *status_type);
extern purple_status_type_get_id_wrapped_fnc purple_status_type_get_id_wrapped;

typedef PurpleStatus * (_cdecl * purple_presence_get_active_status_wrapped_fnc)(const PurplePresence *presence);
extern purple_presence_get_active_status_wrapped_fnc purple_presence_get_active_status_wrapped;

typedef PurpleStatusPrimitive  (_cdecl * purple_status_type_get_primitive_wrapped_fnc)( const PurpleStatusType *status_type);
extern purple_status_type_get_primitive_wrapped_fnc purple_status_type_get_primitive_wrapped;

typedef PurpleStatusType * (_cdecl * purple_status_get_type_wrapped_fnc)(const PurpleStatus *status);
extern purple_status_get_type_wrapped_fnc purple_status_get_type_wrapped;

typedef const char * (_cdecl * purple_status_get_attr_string_wrapped_fnc)(const PurpleStatus *status, const char *id);
extern purple_status_get_attr_string_wrapped_fnc purple_status_get_attr_string_wrapped;

typedef gchar * (_cdecl * purple_markup_escape_text_wrapped_fnc)(const gchar *text, gssize length);
extern purple_markup_escape_text_wrapped_fnc purple_markup_escape_text_wrapped;

typedef char * (_cdecl * purple_markup_strip_html_wrapped_fnc)(const char *str);
extern purple_markup_strip_html_wrapped_fnc purple_markup_strip_html_wrapped;

typedef gchar * (_cdecl * purple_strdup_withhtml_wrapped_fnc)(const gchar *src);
extern purple_strdup_withhtml_wrapped_fnc purple_strdup_withhtml_wrapped;

typedef void  (_cdecl * purple_markup_html_to_xhtml_wrapped_fnc)(const char *html, char **dest_xhtml, char **dest_plain);
extern purple_markup_html_to_xhtml_wrapped_fnc purple_markup_html_to_xhtml_wrapped;

typedef const char * (_cdecl * purple_normalize_wrapped_fnc)(const PurpleAccount *account, const char *str);
extern purple_normalize_wrapped_fnc purple_normalize_wrapped;

typedef gchar * (_cdecl * purple_utf8_try_convert_wrapped_fnc)(const char *str);
extern purple_utf8_try_convert_wrapped_fnc purple_utf8_try_convert_wrapped;

typedef void  (_cdecl * purple_util_set_user_dir_wrapped_fnc)(const char *dir);
extern purple_util_set_user_dir_wrapped_fnc purple_util_set_user_dir_wrapped;

typedef GIOChannel * (_cdecl * wpurple_g_io_channel_win32_new_socket_wrapped_fnc)(int socket);
extern wpurple_g_io_channel_win32_new_socket_wrapped_fnc wpurple_g_io_channel_win32_new_socket_wrapped;


#else


#define PURPLE_BLIST_NODE_IS_CHAT_WRAPPED PURPLE_BLIST_NODE_IS_CHAT
#define PURPLE_BLIST_NODE_IS_BUDDY_WRAPPED PURPLE_BLIST_NODE_IS_BUDDY
#define PURPLE_BLIST_NODE_IS_CONTACT_WRAPPED PURPLE_BLIST_NODE_IS_CONTACT
#define PURPLE_BLIST_NODE_IS_GROUP_WRAPPED PURPLE_BLIST_NODE_IS_GROUP

#define PURPLE_CONV_IM_WRAPPED PURPLE_CONV_IM
#define PURPLE_CONV_CHAT_WRAPPED PURPLE_CONV_CHAT

#define PURPLE_CONNECTION_IS_CONNECTED_WRAPPED PURPLE_CONNECTION_IS_CONNECTED	

#define purple_account_set_bool_wrapped purple_account_set_bool
#define purple_account_get_protocol_id_wrapped purple_account_get_protocol_id
#define purple_account_set_int_wrapped purple_account_set_int
#define purple_account_get_string_wrapped purple_account_get_string
#define purple_account_set_string_wrapped purple_account_set_string
#define purple_account_get_username_wrapped purple_account_get_username
#define purple_account_set_username_wrapped purple_account_set_username
#define purple_account_set_proxy_info_wrapped purple_account_set_proxy_info
#define purple_accounts_find_wrapped purple_accounts_find
#define purple_account_new_wrapped purple_account_new
#define purple_accounts_add_wrapped purple_accounts_add
#define purple_account_get_password_wrapped purple_account_get_password
#define purple_account_set_password_wrapped purple_account_set_password
#define purple_account_set_enabled_wrapped purple_account_set_enabled
#define purple_account_set_privacy_type_wrapped purple_account_set_privacy_type
#define purple_account_get_status_type_with_primitive_wrapped purple_account_get_status_type_with_primitive
#define purple_account_set_status_wrapped purple_account_set_status
#define purple_account_get_int_wrapped purple_account_get_int
#define purple_account_disconnect_wrapped purple_account_disconnect
#define purple_accounts_delete_wrapped purple_accounts_delete
#define purple_account_get_connection_wrapped purple_account_get_connection
#define purple_account_set_alias_wrapped purple_account_set_alias
#define purple_account_set_public_alias_wrapped purple_account_set_public_alias
#define purple_account_remove_buddy_wrapped purple_account_remove_buddy
#define purple_account_add_buddy_wrapped purple_account_add_buddy
#define purple_account_get_name_for_display_wrapped purple_account_get_name_for_display
#define purple_accounts_set_ui_ops_wrapped purple_accounts_set_ui_ops
#define purple_account_option_get_type_wrapped purple_account_option_get_type
#define purple_account_option_get_setting_wrapped purple_account_option_get_setting
#define purple_blist_node_get_type_wrapped purple_blist_node_get_type
#define purple_buddy_get_alias_wrapped purple_buddy_get_alias
#define purple_buddy_get_server_alias_wrapped purple_buddy_get_server_alias
#define purple_find_buddy_wrapped purple_find_buddy
#define purple_buddy_get_group_wrapped purple_buddy_get_group
#define purple_blist_remove_buddy_wrapped purple_blist_remove_buddy
#define purple_blist_alias_buddy_wrapped purple_blist_alias_buddy
#define purple_blist_server_alias_buddy_wrapped purple_blist_server_alias_buddy
#define purple_find_group_wrapped purple_find_group
#define purple_group_new_wrapped purple_group_new
#define purple_blist_add_contact_wrapped purple_blist_add_contact
#define purple_buddy_get_contact_wrapped purple_buddy_get_contact
#define purple_buddy_new_wrapped purple_buddy_new
#define purple_blist_add_buddy_wrapped purple_blist_add_buddy
#define purple_blist_find_chat_wrapped purple_blist_find_chat
#define purple_chat_get_components_wrapped purple_chat_get_components
#define purple_buddy_get_presence_wrapped purple_buddy_get_presence
#define purple_buddy_get_account_wrapped purple_buddy_get_account
#define purple_buddy_get_name_wrapped purple_buddy_get_name
#define purple_find_buddies_wrapped purple_find_buddies
#define purple_group_get_name_wrapped purple_group_get_name
#define purple_blist_set_ui_ops_wrapped purple_blist_set_ui_ops
#define purple_set_blist_wrapped purple_set_blist
#define purple_blist_new_wrapped purple_blist_new
#define purple_blist_load_wrapped purple_blist_load
#define purple_blist_get_handle_wrapped purple_blist_get_handle
#define purple_buddy_icons_set_account_icon_wrapped purple_buddy_icons_set_account_icon
#define purple_buddy_icons_find_wrapped purple_buddy_icons_find
#define purple_buddy_icon_get_full_path_wrapped purple_buddy_icon_get_full_path
#define purple_buddy_icon_unref_wrapped purple_buddy_icon_unref
#define purple_buddy_icons_find_account_icon_wrapped purple_buddy_icons_find_account_icon
#define purple_buddy_icon_get_data_wrapped purple_buddy_icon_get_data
#define purple_certificate_add_ca_search_path_wrapped purple_certificate_add_ca_search_path
#define purple_connection_get_state_wrapped purple_connection_get_state
#define purple_connection_get_account_wrapped purple_connection_get_account
#define purple_connection_get_display_name_wrapped purple_connection_get_display_name
#define purple_connections_set_ui_ops_wrapped purple_connections_set_ui_ops
#define purple_connections_get_handle_wrapped purple_connections_get_handle
#define purple_conversation_get_im_data_wrapped purple_conversation_get_im_data
#define purple_conversation_get_chat_data_wrapped purple_conversation_get_chat_data
#define purple_find_conversation_with_account_wrapped purple_find_conversation_with_account
#define purple_conversation_new_wrapped purple_conversation_new
#define purple_conversation_get_type_wrapped purple_conversation_get_type
#define purple_conversation_set_data_wrapped purple_conversation_set_data
#define purple_conversation_update_wrapped purple_conversation_update
#define purple_conv_im_send_wrapped purple_conv_im_send
#define purple_conv_im_send_with_flags_wrapped purple_conv_im_send_with_flags
#define purple_conv_chat_send_wrapped purple_conv_chat_send
#define purple_conv_chat_send_with_flags_wrapped purple_conv_chat_send_with_flags
#define purple_conversation_destroy_wrapped purple_conversation_destroy
#define purple_conversation_get_account_wrapped purple_conversation_get_account
#define purple_conversation_get_name_wrapped purple_conversation_get_name
#define purple_conversations_set_ui_ops_wrapped purple_conversations_set_ui_ops
#define purple_conversations_get_handle_wrapped purple_conversations_get_handle
#define purple_core_set_ui_ops_wrapped purple_core_set_ui_ops
#define purple_core_init_wrapped purple_core_init
#define purple_debug_set_ui_ops_wrapped purple_debug_set_ui_ops
#define purple_debug_set_verbose_wrapped purple_debug_set_verbose
#define purple_dnsquery_set_ui_ops_wrapped purple_dnsquery_set_ui_ops
#define purple_timeout_remove_wrapped purple_timeout_remove
#define purple_input_add_wrapped purple_input_add
#define purple_timeout_add_wrapped purple_timeout_add
#define purple_timeout_add_seconds_wrapped purple_timeout_add_seconds
#define purple_eventloop_set_ui_ops_wrapped purple_eventloop_set_ui_ops
#define purple_input_remove_wrapped purple_input_remove
#define purple_xfer_ui_ready_wrapped purple_xfer_ui_ready
#define purple_xfer_request_accepted_wrapped purple_xfer_request_accepted
#define purple_xfer_request_denied_wrapped purple_xfer_request_denied
#define purple_xfer_get_account_wrapped purple_xfer_get_account
#define purple_xfer_get_filename_wrapped purple_xfer_get_filename
#define purple_xfer_get_size_wrapped purple_xfer_get_size
#define purple_xfer_unref_wrapped purple_xfer_unref
#define purple_xfer_ref_wrapped purple_xfer_ref
#define purple_xfers_set_ui_ops_wrapped purple_xfers_set_ui_ops
#define purple_xfers_get_handle_wrapped purple_xfers_get_handle
#define purple_roomlist_set_ui_ops_wrapped purple_roomlist_set_ui_ops
#define purple_roomlist_get_list_wrapped purple_roomlist_get_list
#define purple_imgstore_get_data_wrapped purple_imgstore_get_data
#define purple_imgstore_get_size_wrapped purple_imgstore_get_size
#define purple_imgstore_unref_wrapped purple_imgstore_unref
#define purple_notify_user_info_new_wrapped purple_notify_user_info_new
#define purple_notify_user_info_destroy_wrapped purple_notify_user_info_destroy
#define purple_notify_user_info_get_entries_wrapped purple_notify_user_info_get_entries
#define purple_notify_user_info_entry_get_label_wrapped purple_notify_user_info_entry_get_label
#define purple_notify_user_info_entry_get_value_wrapped purple_notify_user_info_entry_get_value
#define purple_notify_set_ui_ops_wrapped purple_notify_set_ui_ops
#define purple_plugins_add_search_path_wrapped purple_plugins_add_search_path
#define purple_plugin_action_free_wrapped purple_plugin_action_free
#define purple_prefs_load_wrapped purple_prefs_load
#define purple_prefs_set_bool_wrapped purple_prefs_set_bool
#define purple_prefs_set_string_wrapped purple_prefs_set_string
#define purple_privacy_deny_wrapped purple_privacy_deny
#define purple_privacy_allow_wrapped purple_privacy_allow
#define purple_privacy_check_wrapped purple_privacy_check
#define purple_proxy_info_new_wrapped purple_proxy_info_new
#define purple_proxy_info_set_type_wrapped purple_proxy_info_set_type
#define purple_proxy_info_set_host_wrapped purple_proxy_info_set_host
#define purple_proxy_info_set_port_wrapped purple_proxy_info_set_port
#define purple_proxy_info_set_username_wrapped purple_proxy_info_set_username
#define purple_proxy_info_set_password_wrapped purple_proxy_info_set_password
#define purple_find_prpl_wrapped purple_find_prpl
#define purple_prpl_send_attention_wrapped purple_prpl_send_attention
#define purple_request_set_ui_ops_wrapped purple_request_set_ui_ops
#define serv_get_info_wrapped serv_get_info
#define serv_alias_buddy_wrapped serv_alias_buddy
#define serv_send_typing_wrapped serv_send_typing
#define serv_join_chat_wrapped serv_join_chat
#define purple_signal_connect_wrapped purple_signal_connect
#define purple_status_type_get_id_wrapped purple_status_type_get_id
#define purple_presence_get_active_status_wrapped purple_presence_get_active_status
#define purple_status_type_get_primitive_wrapped purple_status_type_get_primitive
#define purple_status_get_type_wrapped purple_status_get_type
#define purple_status_get_attr_string_wrapped purple_status_get_attr_string
#define purple_markup_escape_text_wrapped purple_markup_escape_text
#define purple_markup_strip_html_wrapped purple_markup_strip_html
#define purple_strdup_withhtml_wrapped purple_strdup_withhtml
#define purple_markup_html_to_xhtml_wrapped purple_markup_html_to_xhtml
#define purple_normalize_wrapped purple_normalize
#define purple_utf8_try_convert_wrapped purple_utf8_try_convert
#define purple_util_set_user_dir_wrapped purple_util_set_user_dir
#define wpurple_g_io_channel_win32_new_socket_wrapped wpurple_g_io_channel_win32_new_socket
#endif

bool resolvePurpleFunctions();

