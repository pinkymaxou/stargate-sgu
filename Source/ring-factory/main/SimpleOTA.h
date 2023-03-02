#ifndef SIMPLEOTA_H
#define SIMPLEOTA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_log.h"
#include "esp_err.h"

typedef struct
{
    int port;
} SIMPLEOTA_Params;

#define SIMPLEOTA_PARAM_DEFAULT \
{                     \
    .port = 8888,     \
}

esp_err_t SIMPLEOTA_StartAsync(const SIMPLEOTA_Params* params);

bool SIMPLEOTA_GetIsReceiving();

#ifdef __cplusplus
}
#endif

#endif