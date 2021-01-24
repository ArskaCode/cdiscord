//
// Created by arska on 1/21/21.
//

#ifndef CDISCORD_DC_API_H
#define CDISCORD_DC_API_H

#include <cdiscord.h>

void dc_api_init(void);

void dc_api_terminate(void);

/*
void dc_api_client_init(discord_client *client);

void dc_api_client_terminate(discord_client *client);
*/

void dc_api_redeem_gift_code(discord_client *client, const char *gift_code);

#endif //CDISCORD_DC_API_H
