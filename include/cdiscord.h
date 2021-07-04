/*
 * Created by arska on 11/25/20.
 */

#ifndef CDISCORD_CDISCORD_H
#define CDISCORD_CDISCORD_H

#include <stdlib.h>
#include <stdbool.h>

typedef struct discord_client_s discord_client;


/*
 * General library functions
 */

void dc_init(void);

void dc_run(void);

void dc_terminate(void);

void dc_log(int level, char *format, ...);

#define DC_LOG_ERR (1 << 0)
#define DC_LOG_WARN (1 << 1)
#define	DC_LOG_USER (1 << 10)


/*
 * Events
 */

enum dc_event {
    dc_event_connected,
    dc_event_logged_in,
    dc_event_message,
};

typedef void (*dc_event_handler_fun)(discord_client *client, enum dc_event event);


/*
 * Discord data structures
 */

typedef struct dc_user_s {
    u_int64_t id;
    const char *username;
    const char *discriminator;
} dc_user;

typedef struct dc_message_s {
    const char *contents;
} dc_message;


/*
 * Discord client functions
 */

discord_client* dc_client_create(const char *token, bool selfbot);

void dc_client_connect(discord_client *client);

void dc_client_destroy(discord_client *client);

void dc_client_set_event_handler(discord_client *client, dc_event_handler_fun event_handler);

const dc_user* dc_client_get_user(discord_client *client);

const dc_message* dc_client_get_latest_message(discord_client *client);

void dc_client_redeem_gift(discord_client *client, const char *gift_code);


#endif //CDISCORD_CDISCORD_H
