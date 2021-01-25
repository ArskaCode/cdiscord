//
// Created by arska on 1/21/21.
//

#include "dc_api.h"

#include <curl/curl.h>
#include <string.h>

#include "internal.h"


static int http_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);

static void request(discord_client *client, const char *endpoint, const char *data, const char *method);

#define POST_REQ(client, endpoint, data) request(client, endpoint, data, "POST");


struct lws_protocols dc_api_protocol = {
        "http",
        http_callback,
        0,
        512

};


void dc_api_init(void)
{

}

void dc_api_terminate(void)
{

}

void dc_api_redeem_gift(discord_client *client, const char *gift_code)
{

}

int http_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {
    return lws_callback_http_dummy(wsi, reason, user, in, len);
}