//
// Created by arska on 1/21/21.
//

#include "dc_api.h"

#include <curl/curl.h>
#include <string.h>

#include "internal.h"

void dc_api_init(void)
{
    curl_global_init(CURL_GLOBAL_ALL);
    context.curl = curl_easy_init();
}

void dc_api_terminate(void)
{
    curl_easy_cleanup(context.curl);
    curl_global_cleanup();
}

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
    return size * nmemb;
}

void dc_api_redeem_gift_code(discord_client *client, const char *gift_code)
{
    char redeem_url[256];
    strcpy(redeem_url, "https://discord.com/api/v8/entitlements/gift-codes/");
    strcat(redeem_url, gift_code);
    strcat(redeem_url, "/redeem");

    char authorization_str[256];
    strcpy(authorization_str, "Authorization: ");
    strcat(authorization_str, client->token);

    struct curl_slist *chunk = NULL;
    chunk = curl_slist_append(chunk, authorization_str);
    chunk = curl_slist_append(chunk, "content-type: application/json");

    curl_easy_setopt(context.curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(context.curl, CURLOPT_POST, 1);
    curl_easy_setopt(context.curl, CURLOPT_POSTFIELDS, "{\"channel_id\":null}");
    curl_easy_setopt(context.curl, CURLOPT_HTTPHEADER, chunk);
    curl_easy_setopt(context.curl, CURLOPT_URL, redeem_url);

    CURLcode code = curl_easy_perform(context.curl);
    if (code != CURLE_OK)
        dc_log(DC_LOG_ERR, "curl_easy_perform failed with error %s\n", curl_easy_strerror(code));

    long result = 0;
    curl_easy_getinfo(context.curl, CURLINFO_RESPONSE_CODE, &result);

    if (result == 200) {
        dc_log(DC_LOG_USER, "%s#%s: Gift code %s claimed!\n", client->user.username, client->user.discriminator, gift_code);
    }
    else {
        dc_log(DC_LOG_WARN, "%s#%s: Couldn't claim gift code %s: %d!\n", client->user.username, client->user.discriminator, gift_code, result);
    }
}