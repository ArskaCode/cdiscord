/*
 * Created by arska on 1/12/21.
 */

#ifndef CDISCORD_GATEWAY_H
#define CDISCORD_GATEWAY_H

#include <stddef.h>
#include <libwebsockets.h>

/*
 * Connect
 */

void dc_gateway_connect(lws_sorted_usec_list_t *connect_sul);


/*
 * Callback
 */

int dc_gateway_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);


/*
 * Gateway events
 */

enum dc_gateway_event {
    // GUILD_MESSAGES
    dc_gateway_guild_message_create,
    dc_gateway_guild_message_update,
    dc_gateway_guild_message_delete,
    dc_gateway_guild_message_delete_bulk,

    // DIRECT_MESSAGES
    dc_gateway_direct_message_create,
    dc_gateway_direct_message_update,
    dc_gateway_direct_message_delete,
    dc_gateway_direct_channel_pins_update,
};

#endif //CDISCORD_GATEWAY_H
