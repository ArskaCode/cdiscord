/*
 * Created by arska on 1/12/21.
 */

#ifndef CDISCORD_INTERNAL_H
#define CDISCORD_INTERNAL_H

#include <cdiscord.h>

#include <stdbool.h>
#include <curl/curl.h>
#include <libwebsockets.h>
#include <json.h>

struct discord_client_s {
    struct lws *wsi;
    lws_sorted_usec_list_t heartbeat_sul;
    lws_sorted_usec_list_t connect_sul;
    lws_sorted_usec_list_t identify_sul;
    bool connected;
    int reconnect_tries;

    char *token;
    bool selfbot;
    dc_user user;
    dc_event_handler_fun event_handler;

    char *message_buffer;
    size_t message_len;
    size_t remaining;

    int last_seq_num;
    int heartbeat_interval;
    const char *session_id;
    int32_t disconnect_code;

    struct json_object *ready_object;

    void *user_data;

    discord_client *next;
};

struct cdiscord {
    bool initialized;
    struct lws_context *lws_ctx;
    json_tokener *tokener;
    discord_client *head;
    CURL *curl;
};

extern struct cdiscord context;

#endif //CDISCORD_INTERNAL_H
