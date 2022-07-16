#include "webserver.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include <stdio.h>
#include <sys/param.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include "GateControl.h"
#include "EmbeddedFiles.h"
#include "esp_ota_ops.h"
#include "cJSON.h"
#include "Settings.h"
#include "base-fw.h"
#include "GPIO.h"

#define TAG "webserver"

/* Max length a file path can have on storage */
#define HTTPSERVER_BUFFERSIZE (1024*6)

#define DEFAULT_RELATIVE_URI "/index.html"

#define API_GETSETTINGSJSON_URI "/api/getsettingsjson"
#define API_POSTSETTINGSJSON_URI "/api/setsettingsjson"

static esp_err_t api_get_handler(httpd_req_t *req);
static esp_err_t api_post_handler(httpd_req_t *req);

static esp_err_t file_get_handler(httpd_req_t *req);
static esp_err_t file_post_handler(httpd_req_t *req);
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filename);

static esp_err_t file_otauploadpost_handler(httpd_req_t *req);

static const EF_SFile* GetFile(const char* strFilename);

static uint8_t m_u8Buffers[HTTPSERVER_BUFFERSIZE];

static const httpd_uri_t m_sHttpUI = {
    .uri       = "/*",
    .method    = HTTP_GET,
    .handler   = file_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = ""
};

static const httpd_uri_t m_sHttpGetAPI = {
    .uri       = "/api/*",
    .method    = HTTP_GET,
    .handler   = api_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = ""
};

static const httpd_uri_t m_sHttpPostAPI = {
    .uri       = "/api/*",
    .method    = HTTP_POST,
    .handler   = api_post_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = ""
};
static const httpd_uri_t m_sHttpActionPost = {
    .uri       = "/action/*",
    .method    = HTTP_POST,
    .handler   = file_post_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = ""
};

static const httpd_uri_t m_sHttpOTAUploadPost = {
    .uri       = "/ota/upload",
    .method    = HTTP_POST,
    .handler   = file_otauploadpost_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = ""
};
void WEBSERVER_Init()
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.max_open_sockets = 13;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &m_sHttpActionPost);
        httpd_register_uri_handler(server, &m_sHttpGetAPI);
        httpd_register_uri_handler(server, &m_sHttpPostAPI);
        httpd_register_uri_handler(server, &m_sHttpUI);
        httpd_register_uri_handler(server, &m_sHttpOTAUploadPost);
        // return server;
    }
}

/* An HTTP GET handler */
static esp_err_t file_get_handler(httpd_req_t *req)
{
    // Redirect root to index.html
    if (strcmp(req->uri, "/") == 0)
    {
        // Remember, browser keep 301 in cache so be careful
        ESP_LOGW(TAG, "Redirect URI: '%s', to '%s'", req->uri, DEFAULT_RELATIVE_URI);
        // Redirect to default page
        httpd_resp_set_type(req, "text/html");
        httpd_resp_set_status(req, "301 Moved Permanently");
        httpd_resp_set_hdr(req, "Location", DEFAULT_RELATIVE_URI);
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Opening file uri: %s", req->uri);

    const EF_SFile* pFile = GetFile(req->uri+1);
    if (pFile == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file for reading");
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File does not exist");
        return ESP_FAIL;
    }

    set_content_type_from_file(req, pFile->strFilename);

    uint32_t u32Index = 0;

    while(u32Index < pFile->u32Length)
    {
        const uint32_t n = MIN(pFile->u32Length - u32Index, HTTPSERVER_BUFFERSIZE);

        if (n > 0) {
            /* Send the buffer contents as HTTP response m_u8Buffers */
            if (httpd_resp_send_chunk(req, (char*)(pFile->pu8StartAddr + u32Index), n) != ESP_OK) {
                ESP_LOGE(TAG, "File sending failed!");
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
               return ESP_FAIL;
           }
        }
        u32Index += n;
    }

    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t file_post_handler(httpd_req_t *req)
{
    cJSON* root = NULL;

    ESP_LOGI(TAG, "file_post_handler, url: %s", req->uri);

    if (strcmp(req->uri, "/action/gohome") == 0)
        GATECONTROL_DoAction(GATECONTROL_EMODE_GoHome, NULL);
    else if (strcmp(req->uri, "/action/releaseclamp") == 0)
        GATECONTROL_DoAction(GATECONTROL_EMODE_ManualReleaseClamp, NULL);
    else if (strcmp(req->uri, "/action/lockclamp") == 0)
        GATECONTROL_DoAction(GATECONTROL_EMODE_ManualLockClamp, NULL);
    else if (strcmp(req->uri, "/action/stop") == 0)
        GATECONTROL_DoAction(GATECONTROL_EMODE_Stop, NULL);
    else if (strcmp(req->uri, "/action/dial") == 0)
    {
        char content[100+1];
        size_t recv_size = MIN(req->content_len, sizeof(content)-1);
        int n = httpd_req_recv(req, (char*)content, recv_size);
        if (n <= 0) {  /* 0 return value indicates connection closed */
            goto ERROR;
        }
        content[n] = 0; // End
        ESP_LOGI(TAG, "Receiving content: %s", content);
        // Decode JSON
        root = cJSON_Parse(content);
        if (root == NULL || !cJSON_IsArray(root))
            goto ERROR;

        GATECONTROL_UModeArg uModeArg;

        const cJSON *itemArray;
        uModeArg.sDialArg.u8SymbolCount = 0;
        cJSON_ArrayForEach(itemArray, root)
        {
            if (!cJSON_IsNumber(itemArray))
                goto ERROR;
            uModeArg.sDialArg.u8Symbols[uModeArg.sDialArg.u8SymbolCount] = itemArray->valueint;
            uModeArg.sDialArg.u8SymbolCount++;
        }

        if (!GATECONTROL_DoAction(GATECONTROL_EMODE_Dial, &uModeArg))
            goto ERROR;
    }
    else if (strcmp(req->uri, "/action/manualramplight") == 0)
    {
        char content[100+1];
        size_t recv_size = MIN(req->content_len, sizeof(content)-1);
        int n = httpd_req_recv(req, (char*)content, recv_size);
        if (n <= 0) {  /* 0 return value indicates connection closed */
            goto ERROR;
        }
        content[n] = 0; // End
        ESP_LOGI(TAG, "Receiving content: %s", content);
        // Decode JSON
        root = cJSON_Parse(content);
        if (root == NULL)
            goto ERROR;
        cJSON* pKeyJSON = cJSON_GetObjectItem(root, "light_perc");
        if (pKeyJSON == NULL || !cJSON_IsNumber(pKeyJSON))
        {
            ESP_LOGE(TAG, "Cannot find JSON key element");
            goto ERROR;
        }

        GPIO_SetRampLightPerc((float)pKeyJSON->valueint / 100.0f);
    }
    else if (strcmp(req->uri, "/action/ramplighton") == 0)
        GPIO_SetRampLightOnOff(true);
    else if (strcmp(req->uri, "/action/ramplightoff") == 0)
        GPIO_SetRampLightOnOff(false);
    else if (strcmp(req->uri, "/action/ringturnoff") == 0)
    {
        ESP_LOGI(TAG, "GateControl ring turn off");
        SGUBRCOMM_TurnOff(&g_sSGUBRCOMMHandle);
    }
    else if (strcmp(req->uri, "/action/activewormhole") == 0)
        GATECONTROL_DoAction(GATECONTROL_EMODE_ManualWormhole, NULL);
    else if (strcmp(req->uri, "/action/ringchevronerror") == 0)
    {
        ESP_LOGI(TAG, "GateControl ring chevron error");
        SGUBRCOMM_ChevronLightning(&g_sSGUBRCOMMHandle, SGUBRPROTOCOL_ECHEVRONANIM_ErrorToWhite);
    }
    else if (strcmp(req->uri, "/action/ringgotofactory") == 0)
    {            
        ESP_LOGI(TAG, "GateControl ring goto factory");
        SGUBRCOMM_GotoFactory(&g_sSGUBRCOMMHandle);
    }
    else if (strcmp(req->uri, "/action/stop") == 0)
        GATECONTROL_DoAction(GATECONTROL_EMODE_Stop, NULL);
    else if (strcmp(req->uri, "/action/reboot") == 0)
        BASEFW_RequestReboot();
    else
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Unknown request");

    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send_chunk(req, NULL, 0);
    if (root != NULL)
        cJSON_Delete(root);
    return ESP_OK;
    ERROR:
    ESP_LOGE(TAG, "Invalid request");
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad request");
    if (root != NULL)
        cJSON_Delete(root);
    return ESP_FAIL;
}

static esp_err_t api_get_handler(httpd_req_t *req)
{
    char* pExportJSON = NULL;

    if (strcmp(req->uri, API_GETSETTINGSJSON_URI) == 0)
    {
        pExportJSON = SETTINGS_ExportJSON();

        if (httpd_resp_send_chunk(req, pExportJSON, strlen(pExportJSON)) != ESP_OK)
        {
            ESP_LOGE(TAG, "Unable to send JSON data");
            goto END;
        }
    }
    else
    {
        ESP_LOGE(TAG, "api_get_handler, url: %s", req->uri);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Unknown request");
    }
    END:
    if (pExportJSON != NULL)
        free(pExportJSON);
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t api_post_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "api_post_handler, url: %s", req->uri);
    if (strcmp(req->uri, API_POSTSETTINGSJSON_URI) == 0)
    {
        int n = httpd_req_recv(req, (char*)m_u8Buffers, HTTPSERVER_BUFFERSIZE);
        m_u8Buffers[n] = '\0';

        if (!SETTINGS_ImportJSON((const char*)m_u8Buffers))
        {
            ESP_LOGE(TAG, "Unable to import JSON");
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Unknown request");
        }
    }
    else
    {
        ESP_LOGE(TAG, "api_post_handler, url: %s", req->uri);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Unknown request");
    }
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t file_otauploadpost_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "file_otauploadpost_handler / uri: %s", req->uri);

    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    if (configured != running)
    {
        ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
                 configured->address, running->address);
        ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }

    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
             running->type, running->subtype, running->address);

    const esp_partition_t* update_partition = esp_ota_get_next_update_partition(NULL);
    assert(update_partition != NULL);
    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
             update_partition->subtype, update_partition->address);

    esp_ota_handle_t update_handle = 0;

    esp_err_t err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
    err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
        esp_ota_abort(update_handle);
        goto ERROR;
    }

    int n = httpd_req_recv(req, (char*)m_u8Buffers, HTTPSERVER_BUFFERSIZE);
    int binary_file_length = 0;

    while(n > 0)
    {
        ESP_LOGI(TAG, "file_otauploadpost_handler / receiving: %d bytes", n);

        err = esp_ota_write( update_handle, (const void *)m_u8Buffers, n);
        if (err != ESP_OK)
        {
            esp_ota_abort(update_handle);
            goto ERROR;
        }
        binary_file_length += n;
        ESP_LOGD(TAG, "Written image length %d", binary_file_length);

        n = httpd_req_recv(req, (char*)m_u8Buffers, HTTPSERVER_BUFFERSIZE);
    }

    err = esp_ota_end(update_handle);
    if (err != ESP_OK)
    {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
        } else {
            ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
        }

        // TODO: esp_ota_abort(update_handle); needed ?
        goto ERROR;
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        goto ERROR;
    }

    ESP_LOGI(TAG, "OTA Completed !");
    ESP_LOGI(TAG, "Prepare to restart system!");

    BASEFW_RequestReboot();

    httpd_resp_set_hdr(req, "Connection", "close");
    return ESP_OK;
ERROR:
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid image");
    return ESP_FAIL;
}

#define IS_FILE_EXT(filename, ext) \
    (strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)

/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filename)
{
    if (IS_FILE_EXT(filename, ".pdf")) {
        return httpd_resp_set_type(req, "application/pdf");
    } else if (IS_FILE_EXT(filename, ".html")) {
        return httpd_resp_set_type(req, "text/html");
    } else if (IS_FILE_EXT(filename, ".jpeg")) {
        return httpd_resp_set_type(req, "image/jpeg");
    } else if (IS_FILE_EXT(filename, ".ico")) {
        return httpd_resp_set_type(req, "image/x-icon");
    } else if (IS_FILE_EXT(filename, ".css")) {
        return httpd_resp_set_type(req, "text/css");
    } else if (IS_FILE_EXT(filename, ".js")) {
        return httpd_resp_set_type(req, "text/javascript");
    }
    /* This is a limited set only */
    /* For any other type always set as plain text */
    return httpd_resp_set_type(req, "text/plain");
}

static const EF_SFile* GetFile(const char* strFilename)
{
    for(int i = 0; i < EF_EFILE_COUNT; i++)
    {
        const EF_SFile* pFile = &EF_g_sFiles[i];
        if (strcmp(pFile->strFilename, strFilename) == 0)
            return pFile;
    }

    return NULL;
}