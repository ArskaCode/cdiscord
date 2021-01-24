/*
 * Created by arska on 1/12/21.
 */

#ifndef CDISCORD_GATEWAY_H
#define CDISCORD_GATEWAY_H

#include <stddef.h>
#include <libwebsockets.h>

/*
 * Connection
 */

void dc_gateway_connect(lws_sorted_usec_list_t *connect_sul);


/*
 * Callback
 */

int dc_gateway_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);


#endif //CDISCORD_GATEWAY_H
