#ifndef _WEBAPI_H_
#define _WEBAPI_H_

#include <stdint.h>
#include <esp_http_server.h>

esp_err_t WEBAPI_GetHandler(httpd_req_t *req);

esp_err_t WEBAPI_PostHandler(httpd_req_t *req);

esp_err_t WEBAPI_ActionHandler(httpd_req_t *req);

#endif