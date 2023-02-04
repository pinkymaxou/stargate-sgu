#include "SimpleOTA.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "esp_partition.h"
#include "esp_flash_partitions.h"
#include "esp_ota_ops.h"

#define SIMPLEOTA_TAG "SimpleOTA"

static SIMPLEOTA_Params m_params = SIMPLEOTA_PARAM_DEFAULT;

static TaskHandle_t m_taskHandle = NULL;
static bool m_bIsReceiving = false;

static void simpleota_task(void* arg);

esp_err_t SIMPLEOTA_StartAsync(const SIMPLEOTA_Params* params)
{
    if (m_taskHandle != NULL)
    {
        ESP_LOGE(SIMPLEOTA_TAG, "Already working");
        return ESP_FAIL;
    }

    ESP_LOGI(SIMPLEOTA_TAG, "SimpleOTA_StartAsync");
    if (xTaskCreate(simpleota_task, "simpleota_task", 8000, &m_params, tskIDLE_PRIORITY, &m_taskHandle) != pdTRUE)
    {
        ESP_LOGE(SIMPLEOTA_TAG, "Error create simpleota_task");
        return ESP_FAIL;
    }
    return ESP_OK;
}

bool SIMPLEOTA_GetIsReceiving()
{
    return m_bIsReceiving;
}

static void simpleota_task(void* arg)
{
    int sock = 0;

    ESP_LOGI(SIMPLEOTA_TAG, "Simple OTA is starting ...");

    int addr_family = 0;
    int ip_protocol = 0;

    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(m_params.port);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;


    struct timeval timeout;
    timeout.tv_sec = 20;
    timeout.tv_usec = 0;

    ESP_LOGI(SIMPLEOTA_TAG, "Simple OTA is about to start listening on port: %d ...", m_params.port);

    int listensock =  socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listensock < 0)
    {
        ESP_LOGE(SIMPLEOTA_TAG, "Unable to create socket: errno %d", errno);
        goto CLEAN_UP;
    }

    int opt = 1;
    setsockopt(listensock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    int err = bind(listensock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(SIMPLEOTA_TAG, "Socket unable to bind: errno %d", errno);
        ESP_LOGE(SIMPLEOTA_TAG, "IPPROTO: %d", addr_family);
        goto CLEAN_UP;
    }
    ESP_LOGI(SIMPLEOTA_TAG, "Socket bound, port %d", m_params.port);

    err = listen(listensock, 1);
    if (err != 0) {
        ESP_LOGE(SIMPLEOTA_TAG, "Error occurred during listen: errno %d", errno);
        goto CLEAN_UP;
    }

    struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
    socklen_t addr_len = sizeof(source_addr);
    sock = accept(listensock, (struct sockaddr *)&source_addr, &addr_len);
    if (sock < 0) {
        ESP_LOGE(SIMPLEOTA_TAG, "Unable to accept connection: errno %d", errno);
        goto CLEAN_UP;
    }

    if (setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
        ESP_LOGE(SIMPLEOTA_TAG, "setsockopt failed");

    if (setsockopt (sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
        ESP_LOGE(SIMPLEOTA_TAG, "setsockopt failed");

    // Convert ip address to string
    if (source_addr.ss_family == PF_INET)
    {
        char addr_str[128];
        inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
        ESP_LOGI(SIMPLEOTA_TAG, "Socket accepted ip address: %s", addr_str);
    }

    m_bIsReceiving = true;

    ESP_LOGI(SIMPLEOTA_TAG, "Request sent, waiting for response ....");

    // -----------------------------
    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);

    ESP_LOGI(SIMPLEOTA_TAG, "OTA Partition subtype %d at offset 0x%x",
             update_partition->subtype, update_partition->address);

    esp_app_desc_t running_app_info;
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK)
    {
        ESP_LOGI(SIMPLEOTA_TAG, "Running firmware version: %s", running_app_info.version);
    }

    const esp_partition_t* last_invalid_app = esp_ota_get_last_invalid_partition();
    esp_app_desc_t invalid_app_info;
    if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK)
    {
        ESP_LOGI(SIMPLEOTA_TAG, "Last invalid firmware version: %s", invalid_app_info.version);
    }

    esp_ota_handle_t update_handle = 0 ;
    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(SIMPLEOTA_TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
        goto CLEAN_UP;
    }

    ESP_LOGI(SIMPLEOTA_TAG, "OTA begins!");

    // Receive ....
    uint8_t buffer[4096];
    int received = 0;

    int r = recv(sock, buffer, sizeof(buffer), 0);
    received += r;
    while (r > 0)
    {
        esp_err_t err3 = esp_ota_write( update_handle, (const void *)buffer, r);
        if (err3 != ESP_OK)
        {
            ESP_LOGE(SIMPLEOTA_TAG, "esp_ota_write error, stopped !");
            goto CLEAN_UP;
        }

        ESP_LOGI(SIMPLEOTA_TAG, "Receiving transmission, data: %d bytes", received);

        r = recv(sock, buffer, sizeof(buffer), 0);
        received += r;
    }

    ESP_LOGI(SIMPLEOTA_TAG, "Ending OTA");

    err = esp_ota_end(update_handle);
    if (err != ESP_OK)
    {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED)
        {
            ESP_LOGE(SIMPLEOTA_TAG, "Image validation failed, image is corrupted");
        }

        ESP_LOGE(SIMPLEOTA_TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
        goto CLEAN_UP;
    }
    else
    {
        ESP_LOGI(SIMPLEOTA_TAG, "Setting up boot partition ...!");
        err = esp_ota_set_boot_partition(update_partition);
        if (err != ESP_OK)
        {
            ESP_LOGE(SIMPLEOTA_TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
            goto CLEAN_UP;
        }

        ESP_LOGI(SIMPLEOTA_TAG, "Install succeeded!");
        ESP_LOGI(SIMPLEOTA_TAG, "About to restart!");
    }

    CLEAN_UP:
    if (sock != -1)
    {
        ESP_LOGE(SIMPLEOTA_TAG, "Shutting down socket");
        shutdown(sock, 0);
        close(sock);
    }

    // Restart, no matter the results
    esp_restart();

    m_taskHandle = NULL;
	vTaskDelete(NULL);
}