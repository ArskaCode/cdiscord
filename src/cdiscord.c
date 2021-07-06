/*
 * Created by arska on 11/25/20.
 */

#include <cdiscord.h>
#include <stdlib.h>
#include <stdbool.h>
#include <libwebsockets.h>
#include <json.h>

#include "internal.h"
#include "gateway.h"
#include "dc_api.h"

struct cdiscord context;

static void dc_client_destroy_internal(discord_client *client);

static struct lws_protocols protocols[] = {
        {
                "discord-gateway",
                dc_gateway_callback,
                0,
                512
        }, {0}
};

static struct lws_extension extensions[] = {
        {0}
};

void dc_init(void)
{
    if (context.initialized)
        return;

    lws_set_log_level(LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE, NULL);
    struct lws_context_creation_info i = {0};
    i.port = CONTEXT_PORT_NO_LISTEN;
    i.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    i.protocols = protocols;
    i.gid = -1;
    i.uid = -1;
    i.extensions = extensions;
    context.lws_ctx = lws_create_context(&i);
    context.tokener = json_tokener_new();
    dc_api_init();
    context.initialized = true;
}

void dc_run(void)
{
    while (lws_service(context.lws_ctx, 0) >= 0);
}

void dc_terminate(void)
{
    discord_client *p = context.head;
    while (p) {
        discord_client *old = p;
        printf("Freeing %s\n", p->token);
        p = p->next;
        dc_client_destroy_internal(old);
    }

    lws_context_destroy(context.lws_ctx);
    json_tokener_free(context.tokener);
    dc_api_terminate();
    memset(&context, 0, sizeof context);
}

void dc_log(int level, char *format, ...)
{
    va_list args;
    va_start(args, format);
    _lws_logv(level, format, args);
    va_end(args);
}

discord_client* dc_client_create(const char *token, bool selfbot, void *user)
{
    discord_client* client = calloc(1, sizeof(discord_client));
    client->token = malloc(strlen(token)+1);
    client->selfbot = selfbot;
    client->user_data = user;
    client->heartbeat_acked = true;
    strcpy(client->token, token);
    client->next = context.head;
    context.head = client;
    return client;
}

void dc_client_connect(discord_client *client)
{
    lws_sul_schedule(context.lws_ctx, 0, &client->connect_sul, dc_gateway_connect, 1);
}

void dc_client_destroy(discord_client *client)
{
    discord_client *prev = NULL;
    discord_client *p = context.head;

    while (p && p != client) {
        prev = p;
        p = p->next;
    }

    if (p && prev) {
        prev->next = p->next;
    }

    dc_client_destroy_internal(client);
}

void dc_client_destroy_internal(discord_client *client)
{
    if (client->connected) {
        lws_sul_cancel(&client->heartbeat_sul);
        lws_sul_cancel(&client->connect_sul);
        lws_sul_cancel(&client->identify_sul);
        lws_close_free_wsi(client->wsi, LWS_CLOSE_STATUS_NORMAL, "dc_client_destroy_internal");
    }

    json_object_put(client->ready_object);

    free(client->message_buffer);
    free(client->token);
    free(client);
}

void dc_client_set_event_handler(discord_client *client, dc_event_handler_fun event_handler)
{
    client->event_handler = event_handler;
}

void *dc_client_get_user_data(discord_client *client) {
    return client->user_data;
}

const dc_user *dc_client_get_user(discord_client *client)
{
    return &client->user;
}

bool dc_client_redeem_gift(discord_client *client, const char *gift_code)
{
    return dc_api_redeem_gift_code(client, gift_code);
}
