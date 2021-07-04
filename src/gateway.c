/*
 * Created by arska on 1/12/21.
 */

#include "gateway.h"

#include <json.h>
#include <bsd/stdlib.h>

#include "internal.h"

/*
 * Opcode handlers
 */

static void handle_packet(discord_client *client);

static void handle_dispatch(discord_client *client, const char *type, json_object *data);

static void handle_hello(discord_client *client, json_object *data);

static void handle_invalid_session(discord_client *client, json_object *data);


/*
 * Messages
 */

// Message buffer. Max size 512 bytes for now.
char msg_buf[LWS_PRE + 512];

static void send_heartbeat(discord_client *client);

static void send_identify(discord_client *client);

static void send_resume(discord_client *client);


/*
 * Intents
 */

#define GUILDS                      (1 << 0)
#define GUILD_MEMBERS               (1 << 1)
#define GUILD_BANS                  (1 << 2)
#define GUILD_EMOJIS                (1 << 3)
#define GUILD_INTEGRATIONS          (1 << 4)
#define GUILD_WEBHOOKS              (1 << 5)
#define GUILD_INVITES               (1 << 6)
#define GUILD_VOICE_STATES          (1 << 7)
#define GUILD_PRESENCES             (1 << 8)
#define GUILD_MESSAGES              (1 << 9)
#define GUILD_MESSAGE_REACTIONS     (1 << 10)
#define GUILD_MESSAGE_TYPING        (1 << 11)

#define DIRECT_MESSAGES             (1 << 12)
#define DIRECT_MESSAGE_REACTIONS    (1 << 13)
#define DIRECT_MESSAGE_TYPING       (1 << 14)

static const int supported_intents = GUILD_MESSAGES | DIRECT_MESSAGES;

/*
 * Misc
 */

static void invalidate_session(discord_client *client);

static void identify_callback(lws_sorted_usec_list_t *identify_sul);

static const int ssl_flags = LCCSCF_USE_SSL;

void dc_gateway_connect(lws_sorted_usec_list_t *connect_sul)
{
    discord_client *client = lws_container_of(connect_sul, discord_client, connect_sul);

    struct lws_client_connect_info i = {0};

    i.context = context.lws_ctx;
    i.address = "gateway.discord.gg";
    i.path = "/?v=8&encoding=json";
    i.port = 443;
    i.host = i.address;
    i.origin = i.address;
    i.ssl_connection = ssl_flags;
    i.pwsi = &client->wsi;
    i.userdata = client;
    i.protocol = NULL;

    lws_client_connect_via_info(&i);
    client->connected = true;
}

int dc_gateway_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
    discord_client *client = (discord_client*) user;

    switch (reason) {
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            lwsl_err("CLIENT_CONNECTION_ERROR: %s\n",
                    in ? (char *)in : "(null)");
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE: {
            bool first_frag = lws_is_first_fragment(client->wsi);
            bool last_frag = lws_is_final_fragment(client->wsi);

            if (first_frag) {
                free(client->message_buffer);
                client->message_len = 0;
                client->message_buffer = malloc(lws_remaining_packet_payload(client->wsi) + len);
            }

            memcpy(&client->message_buffer[client->message_len], in, len);
            client->message_len += len;

            if (!last_frag)
                break;

             /*
            if (client->message_len < 4 || ntohl(((uint32_t*)client->message_buffer)[0]) != 0x0000FFFF)
                break;

            z_stream stream;
            stream.zalloc = Z_NULL;
            stream.zfree = Z_NULL;
            stream.opaque = Z_NULL;

            stream.avail_in = 0;
            stream.next_in = Z_NULL;

            int result = inflateInit(&stream);
            if (result != Z_OK) {
                lwsl_err("Could not initialize zlib decompress.\n");
                break;
            }
             */

            json_object* obj = json_tokener_parse_ex(context.tokener, client->message_buffer, client->message_len);
            int opcode = json_object_get_int(json_object_object_get(obj, "op"));
            json_object *data = json_object_object_get(obj, "d");

            switch (opcode) {
                case 0: { // dispatch
                    client->last_seq_num = json_object_get_int(json_object_object_get(obj, "s"));
                    const char* type = json_object_get_string(json_object_object_get(obj, "t"));
                    handle_dispatch(client, type, data);
                    break;
                }

                case 7: // reconnect
                    lwsl_user("%s#%s: Received reconnect opcode.\n", client->user.username, client->user.discriminator);
                    lws_close_free_wsi(client->wsi, LWS_CLOSE_STATUS_NORMAL, "dc_gateway_callback");
                    break;

                case 9: // invalid session
                    handle_invalid_session(client, data);
                    break;

                case 10: // hello
                    handle_hello(client, data);
                    break;

                case 11: // heartbeat ack
                    break;

                default:
                    lwsl_warn("Unknown opcode %d received\n", opcode);
            }
            json_object_put(obj);
            break;
        }

        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            break;

        case LWS_CALLBACK_CLIENT_CLOSED:
            lwsl_warn("%s#%s: Connection closed with code %d!\n", client->user.username, client->user.discriminator, client->disconnect_code);
            lws_sul_cancel(&client->heartbeat_sul);
            client->connected = false;

            if (client->reconnect_tries < 5) {
                lws_sul_schedule(context.lws_ctx, 0, &client->connect_sul, dc_gateway_connect, 5000 * LWS_US_PER_MS);
                client->reconnect_tries++;
            }
            break;

        case LWS_CALLBACK_WS_PEER_INITIATED_CLOSE:
            if (len < 2) // No close event code
                break;
            client->disconnect_code = ntohs(*(int16_t *) in);
            break;

        default:
            break;
    }

    return lws_callback_http_dummy(wsi, reason, user, in, len);
}

static void heartbeat_callback(lws_sorted_usec_list_t *heartbeat_sul)
{
    discord_client *client = lws_container_of(heartbeat_sul, discord_client, heartbeat_sul);
    send_heartbeat(client);
    lws_sul_schedule(context.lws_ctx, 0, &client->heartbeat_sul, heartbeat_callback, client->heartbeat_interval * LWS_US_PER_MS);
}

static void handle_dispatch(discord_client *client, const char *type, json_object *data)
{
    if (!strcmp(type, "MESSAGE_CREATE")) {
        json_object_put(client->message_create_object);
        client->message_create_object = NULL;
        json_object_deep_copy(data, (json_object**)&client->message_create_object, NULL);
        client->latest_message.contents = json_object_get_string(json_object_object_get(client->message_create_object, "content"));
        if (client->event_handler)
            client->event_handler(client, dc_event_message);
    }
    else if (!strcmp(type, "MESSAGE_UPDATE")) {
        json_object_put(client->message_update_object);
        client->message_update_object = NULL;
        json_object_deep_copy(data, (json_object**)&client->message_update_object, NULL);
        client->latest_message.contents = json_object_get_string(json_object_object_get(client->message_update_object, "content"));
        if (client->event_handler)
            client->event_handler(client, dc_event_message);
    }
    else if (!strcmp(type, "READY")) {
        json_object_put(client->ready_object);
        client->ready_object = NULL;
        json_object_deep_copy(data, (json_object**)&client->ready_object, NULL);

        client->session_id = json_object_get_string(json_object_object_get(client->ready_object, "session_id"));

        json_object *user = json_object_object_get(client->ready_object, "user");
        client->user.id = json_object_get_uint64(json_object_object_get(user, "id"));
        client->user.username = json_object_get_string(json_object_object_get(user, "username"));
        client->user.discriminator = json_object_get_string(json_object_object_get(user, "discriminator"));

        if (client->event_handler)
            client->event_handler(client, dc_event_logged_in);

        client->reconnect_tries = 0;
    }
    else if (!strcmp(type, "RESUMED")) {
        lwsl_user("%s#%s: Connection resumed successfully!\n", client->user.username, client->user.discriminator);
        client->reconnect_tries = 0;
    }
}

static void handle_hello(discord_client *client, json_object *data)
{
    if (client->event_handler)
        client->event_handler(client, dc_event_connected);

    int interval = json_object_get_int(json_object_object_get(data, "heartbeat_interval"));
    client->heartbeat_interval = interval;
    lws_sul_schedule(context.lws_ctx, 0, &client->heartbeat_sul, heartbeat_callback, interval * LWS_US_PER_MS);
    send_heartbeat(client);

    if (client->session_id) {
        lwsl_user("Resuming connection...\n");
        send_resume(client);
    }
    else {
        send_identify(client);
    }
}

void handle_invalid_session(discord_client *client, json_object *data)
{
    invalidate_session(client);
    lwsl_warn("Invalid session. Creating new.\n");
    lws_sul_schedule(context.lws_ctx, 0, &client->identify_sul, identify_callback, (1000 + arc4random_uniform(4001)) * LWS_US_PER_MS);
}

static void send_heartbeat(discord_client *client)
{
    json_object *root = json_object_new_object();
    json_object_object_add(root, "op", json_object_new_int(1));
    json_object_object_add(root, "d", client->last_seq_num == 0 ? json_object_new_null() : json_object_new_int(client->last_seq_num));

    const char* json_str = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PLAIN);
    sprintf(&msg_buf[LWS_PRE], "%s", json_str);
    lws_write(client->wsi, (unsigned char*) &msg_buf[LWS_PRE], strlen(json_str), LWS_WRITE_TEXT);
    json_object_put(root);
}

static void send_identify(discord_client *client)
{
    json_object *root = json_object_new_object();
    json_object *properties = json_object_new_object();

    json_object_object_add(properties, "os", json_object_new_string("Linux"));
    json_object_object_add(properties, "browser", json_object_new_string("Firefox"));
    json_object_object_add(properties, "device", json_object_new_string(""));

    json_object *d = json_object_new_object();

    json_object_object_add(d, "token", json_object_new_string(client->token));
    json_object_object_add(d, "intents", json_object_new_int(supported_intents));
    json_object_object_add(d, "properties", properties);
    json_object_object_add(root, "op", json_object_new_int(2));
    json_object_object_add(root, "d", d);

    const char* json_str = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PLAIN);
    sprintf(&msg_buf[LWS_PRE], "%s", json_str);
    lws_write(client->wsi, (unsigned char*) &msg_buf[LWS_PRE], strlen(json_str), LWS_WRITE_TEXT);
    json_object_put(root);
}

void send_resume(discord_client *client)
{
    json_object *root = json_object_new_object();
    json_object *d = json_object_new_object();

    json_object_object_add(d, "token", json_object_new_string(client->token));
    json_object_object_add(d, "session_id", json_object_new_string(client->session_id));
    json_object_object_add(d, "seq", json_object_new_int(client->last_seq_num));
    json_object_object_add(root, "op", json_object_new_int(2));
    json_object_object_add(root, "d", d);

    const char* json_str = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PLAIN);
    sprintf(&msg_buf[LWS_PRE], "%s", json_str);
    lws_write(client->wsi, (unsigned char*) &msg_buf[LWS_PRE], strlen(json_str), LWS_WRITE_TEXT);
    json_object_put(root);
}

void invalidate_session(discord_client *client)
{
    json_object_put(client->ready_object);
    client->ready_object = NULL;
    client->session_id = NULL;
    memset(&client->user, 0, sizeof client->user);
}

void identify_callback(lws_sorted_usec_list_t *identify_sul)
{
    discord_client *client = lws_container_of(identify_sul, discord_client, identify_sul);
    send_identify(client);
}
