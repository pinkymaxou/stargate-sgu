#include "Settings.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <assert.h>
#include "cJSON.h"
#include "esp_log.h"
#include <string.h>

#define TAG "SETTINGS"
#define PARTITION_NAME "nvs"

typedef enum
{
    ETYPE_Int32,
    ETYPE_String
} ETYPE;

typedef union
{
    struct
    {
       int32_t s32Min;
       int32_t s32Max;
       int32_t s32Default;
    } sInt32;
    struct
    {
       float fMin;
       float fMax;
       float fDefault;
    } sFloat;
    struct
    {
       char* szDefault;
    } sString;
} UConfig;

typedef struct
{
    const char* szKey;
    const char* szDesc;
    ETYPE eType;
    UConfig uConfig;
    SETTINGS_EFLAGS eFlags;
} SSettingEntry;

static SSettingEntry m_sConfigEntries[SETTINGS_EENTRY_Count] =
{
    [SETTINGS_EENTRY_ClampLockedPWM] =          { .szKey = "Clamp.LockedPWM",   .eType = ETYPE_Int32, .szDesc = "Servo motor locked PWM",             .uConfig = { .sInt32 = { .s32Min = 1000, .s32Max = 2000, .s32Default = 1250 } }  },
    [SETTINGS_EENTRY_ClampReleasedPWM] =        { .szKey = "Clamp.ReleasPWM",   .eType = ETYPE_Int32, .szDesc = "Servo motor released PWM",           .uConfig = { .sInt32 = { .s32Min = 1000, .s32Max = 2000, .s32Default = 1000 } }  },
    [SETTINGS_EENTRY_RingHomeOffset] =          { .szKey = "Ring.HomeOffset",   .eType = ETYPE_Int32, .szDesc = "Offset relative to home sensor",     .uConfig = { .sInt32 = { .s32Min = -500, .s32Max = 500, .s32Default = -55 } }  },
    [SETTINGS_EENTRY_RingSlowDelta] =           { .szKey = "Ring.SlowDelta",    .eType = ETYPE_Int32, .szDesc = "Encoder delta in slow move mode",    .uConfig = { .sInt32 = { .s32Min = 0, .s32Max = 800, .s32Default = 300 } }  },
    [SETTINGS_EENTRY_RingSymbolBrightness] =    { .szKey = "Ring.SymBright",    .eType = ETYPE_Int32, .szDesc = "Symbol brightness",                  .uConfig = { .sInt32 = { .s32Min = 3, .s32Max = 50, .s32Default = 15 } }  },
    [SETTINGS_EENTRY_StepPerRotation] =         { .szKey = "StepPerRot",        .eType = ETYPE_Int32, .szDesc = "How many step per rotation",         .uConfig = { .sInt32 = { .s32Min = 0, .s32Max = 64000, .s32Default = 7334 } }  },
    [SETTINGS_EENTRY_GateOpenedTimeout] =       { .szKey = "GateTimeoutS",      .eType = ETYPE_Int32, .szDesc = "Timeout (s) before the gate close",  .uConfig = { .sInt32 = { .s32Min = 10, .s32Max = 42*60, .s32Default = 300 } }  },
    [SETTINGS_EENTRY_HomeMaximumStepTicks] =    { .szKey = "Home.MaxSteps",     .eType = ETYPE_Int32, .szDesc = "Maximum step for homing process",    .uConfig = { .sInt32 = { .s32Min = 0, .s32Max = 64000, .s32Default = 8000 } }  },
    [SETTINGS_EENTRY_RampOnPercent] =           { .szKey = "Ramp.LightOn",      .eType = ETYPE_Int32, .szDesc = "Ramp illumination ON (percent)",     .uConfig = { .sInt32 = { .s32Min = 0, .s32Max = 100, .s32Default = 30 } }  },
    [SETTINGS_EENTRY_RampOffPercent] =          { .szKey = "Ramp.LightOff",     .eType = ETYPE_Int32, .szDesc = "Ramp illumination OFF (percent)",    .uConfig = { .sInt32 = { .s32Min = 0, .s32Max = 100, .s32Default = 0 } }  },

    // WiFi Station related
    [SETTINGS_EENTRY_WSTAIsActive] =            { .szKey = "WSTA.IsActive",     .eType = ETYPE_Int32, .szDesc = "Wifi is active",                     .uConfig = { .sInt32 = { .s32Min = 0, .s32Max = 1, .s32Default = 0 } },   .eFlags = SETTINGS_EFLAGS_NeedsReboot  },
    [SETTINGS_EENTRY_WSTASSID] =                { .szKey = "WSTA.SSID",         .eType = ETYPE_String,.szDesc = "WiFi (SSID)",                        .uConfig = { .sString = { .szDefault = "" } },                            .eFlags = SETTINGS_EFLAGS_NeedsReboot  },
    [SETTINGS_EENTRY_WSTAPass] =                { .szKey = "WSTA.Pass",         .eType = ETYPE_String,.szDesc = "WiFi password",                      .uConfig = { .sString = { .szDefault = "" } },                            .eFlags = SETTINGS_EFLAGS_Secret | SETTINGS_EFLAGS_NeedsReboot }
};



static const SSettingEntry* GetSettingEntry(SETTINGS_EENTRY eEntry);
static bool GetSettingEntryByKey(const char* szKey, SETTINGS_EENTRY* peEntry);

//static StaticSemaphore_t m_xSemaphoreCreateMutex;
//static SemaphoreHandle_t m_xSemaphoreHandle;
static nvs_handle_t m_sNVS;

void SETTINGS_Init()
{
    //m_xSemaphoreHandle = xSemaphoreCreateMutexStatic(&m_xSemaphoreCreateMutex);
    //configASSERT( m_xSemaphoreHandle );
    ESP_ERROR_CHECK(nvs_open(PARTITION_NAME, NVS_READWRITE, &m_sNVS));
}

void SETTINGS_Load()
{
    //xSemaphoreTake(m_xSemaphoreHandle, portMAX_DELAY);
    //xSemaphoreGive(m_xSemaphoreHandle);
}

void SETTINGS_Save()
{
    //xSemaphoreTake(m_xSemaphoreHandle, portMAX_DELAY);
    ESP_ERROR_CHECK(nvs_commit(m_sNVS));
    //xSemaphoreGive(m_xSemaphoreHandle);
}

int32_t SETTINGS_GetValueInt32(SETTINGS_EENTRY eEntry)
{
    const SSettingEntry* pEnt = GetSettingEntry(eEntry);
    assert(pEnt != NULL && pEnt->eType == ETYPE_Int32);
    int32_t s32 = 0;
    /* s32 >= pEnt->uConfig.sInt32.s32Min &&
       s32 <= pEnt->uConfig.sInt32.s32Max*/
    if (nvs_get_i32(m_sNVS, pEnt->szKey, &s32) == ESP_OK)
        return s32;
    return pEnt->uConfig.sInt32.s32Default;
}

SETTINGS_ESETRET SETTINGS_SetValueInt32(SETTINGS_EENTRY eEntry, bool bIsDryRun, int32_t s32NewValue)
{
    const SSettingEntry* pEnt = GetSettingEntry(eEntry);
    assert(pEnt != NULL && pEnt->eType == ETYPE_Int32);
    if (s32NewValue < pEnt->uConfig.sInt32.s32Min || s32NewValue > pEnt->uConfig.sInt32.s32Max)
        return SETTINGS_ESETRET_InvalidRange;
    if (!bIsDryRun)
    {
        const esp_err_t err = nvs_set_i32(m_sNVS, pEnt->szKey, s32NewValue);
        ESP_ERROR_CHECK_WITHOUT_ABORT(err);
        if (err != ESP_OK)
            return SETTINGS_ESETRET_CannotSet;
    }
    return SETTINGS_ESETRET_OK;
}

void SETTINGS_GetValueString(SETTINGS_EENTRY eEntry, char* out_value, size_t* length)
{
    const SSettingEntry* pEnt = GetSettingEntry(eEntry);
    assert(pEnt != NULL && pEnt->eType == ETYPE_String);

    if (nvs_get_str(m_sNVS, pEnt->szKey, out_value, length) != ESP_OK)
    {
        // Default value if it cannot retrieve it ...
        if (pEnt->uConfig.sString.szDefault != NULL)
        {
            *length = strlen(pEnt->uConfig.sString.szDefault);
            strncpy(out_value, pEnt->uConfig.sString.szDefault, *length);
        }
    }
}

SETTINGS_ESETRET SETTINGS_SetValueString(SETTINGS_EENTRY eEntry, bool bIsDryRun, const char* szValue)
{
    const SSettingEntry* pEnt = GetSettingEntry(eEntry);
    assert(pEnt != NULL && pEnt->eType == ETYPE_String);
    if (!bIsDryRun)
    {
        const esp_err_t err = nvs_set_str(m_sNVS, pEnt->szKey, szValue);
        ESP_ERROR_CHECK_WITHOUT_ABORT(err);
        if (err != ESP_OK)
            return SETTINGS_ESETRET_CannotSet;
    }
    return SETTINGS_ESETRET_OK;
}

const char* SETTINGS_ExportJSON()
{
    cJSON* pRoot = cJSON_CreateObject();
    if (pRoot == NULL)
        goto ERROR;

    cJSON* pEntries = cJSON_AddArrayToObject(pRoot, "Entries");

    for(int i = 0; i < SETTINGS_EENTRY_Count; i++)
    {
        SETTINGS_EENTRY eEntry = (SETTINGS_EENTRY)i;
        const SSettingEntry* pEntry = GetSettingEntry( eEntry );

        cJSON* pEntryJSON = cJSON_CreateObject();
        cJSON_AddItemToObject(pEntryJSON, "key", cJSON_CreateString(pEntry->szKey));

        cJSON* pEntryInfoJSON = cJSON_CreateObject();

        // Description and flags apply everywhere
        cJSON_AddItemToObject(pEntryInfoJSON, "desc", cJSON_CreateString(pEntry->szDesc));
        cJSON_AddItemToObject(pEntryInfoJSON, "flag_reboot", cJSON_CreateNumber((pEntry->eFlags & SETTINGS_EFLAGS_NeedsReboot)? 1 : 0));

        if (pEntry->eType == ETYPE_Int32)
        {
            if ((pEntry->eFlags & SETTINGS_EFLAGS_Secret) != SETTINGS_EFLAGS_Secret)
                cJSON_AddItemToObject(pEntryJSON, "value", cJSON_CreateNumber(SETTINGS_GetValueInt32(eEntry)));

            cJSON_AddItemToObject(pEntryInfoJSON, "default", cJSON_CreateNumber(pEntry->uConfig.sInt32.s32Default));
            cJSON_AddItemToObject(pEntryInfoJSON, "min", cJSON_CreateNumber(pEntry->uConfig.sInt32.s32Min));
            cJSON_AddItemToObject(pEntryInfoJSON, "max", cJSON_CreateNumber(pEntry->uConfig.sInt32.s32Max));
        }
        else if (pEntry->eType == ETYPE_String)
        {
            char value[SETTINGS_GETVALUESTRING_MAXLEN+1] = {0,};
            size_t length = SETTINGS_GETVALUESTRING_MAXLEN;
            if ((pEntry->eFlags & SETTINGS_EFLAGS_Secret) != SETTINGS_EFLAGS_Secret)
            {
                SETTINGS_GetValueString(eEntry, value, &length);
                cJSON_AddItemToObject(pEntryJSON, "value", cJSON_CreateString(value));
            }
            cJSON_AddItemToObject(pEntryInfoJSON, "default", cJSON_CreateString(pEntry->uConfig.sString.szDefault));
        }

        cJSON_AddItemToObject(pEntryJSON, "info", pEntryInfoJSON);

        cJSON_AddItemToArray(pEntries, pEntryJSON);
    }
    const char* pStr = cJSON_Print(pRoot);
    cJSON_Delete(pRoot);
    return pStr;
    ERROR:
    cJSON_Delete(pRoot);
    return NULL;
}

bool SETTINGS_ImportJSON(const char* szJSON)
{
    bool bRet = true;
    cJSON* pRoot = cJSON_Parse(szJSON);

    cJSON* pEntriesArray = cJSON_GetObjectItem(pRoot, "Entries");
    if (!cJSON_IsArray(pEntriesArray))
    {
        ESP_LOGE(TAG, "Entries array is not valid");
        goto ERROR;
    }

    for(int pass = 0; pass < 2; pass++)
    {
        const bool bIsDryRun = pass == 0;

        for(int i = 0; i < cJSON_GetArraySize(pEntriesArray); i++)
        {
            cJSON* pEntryJSON = cJSON_GetArrayItem(pEntriesArray, i);

            cJSON* pKeyJSON = cJSON_GetObjectItem(pEntryJSON, "key");
            if (pKeyJSON == NULL || !cJSON_IsString(pKeyJSON))
            {
                ESP_LOGE(TAG, "Cannot find JSON key element");
                goto ERROR;
            }

            cJSON* pValueJSON = cJSON_GetObjectItem(pEntryJSON, "value");
            if (pValueJSON == NULL)
            {
                // We just ignore changing the setting if the value property is not there.
                // it allows us to handle secret cases.
                ESP_LOGD(TAG, "JSON value is not there, skipping it");
                continue;
            }

            SETTINGS_EENTRY eEntry;
            if (!GetSettingEntryByKey(pKeyJSON->valuestring, &eEntry))
            {
                ESP_LOGE(TAG, "Key: '%s' is not valid", pKeyJSON->valuestring);
                goto ERROR;
            }

            const SSettingEntry* pSettingEntry = GetSettingEntry(eEntry);

            if (pSettingEntry->eType == ETYPE_Int32)
            {
                if (!cJSON_IsNumber(pValueJSON))
                {
                    ESP_LOGE(TAG, "JSON value type is invalid, not a number");
                    goto ERROR;
                }
                int32_t s32 = pValueJSON->valueint;
                SETTINGS_ESETRET eSetRet;
                if ((eSetRet = SETTINGS_SetValueInt32(eEntry, bIsDryRun, s32)) != SETTINGS_ESETRET_OK)
                {
                    ESP_LOGE(TAG, "Unable to set value for key: %s, bIsDryRun: %d, ret: %d", pSettingEntry->szKey, bIsDryRun, eSetRet);
                    goto ERROR;
                }
            }
            else if (pSettingEntry->eType == ETYPE_String)
            {
                if (!cJSON_IsString(pValueJSON))
                {
                    ESP_LOGE(TAG, "JSON value type is invalid, not a string");
                    goto ERROR;
                }
                
                const char* str = pValueJSON->valuestring;
                SETTINGS_ESETRET eSetRet;
                if ((eSetRet = SETTINGS_SetValueString(eEntry, bIsDryRun, str)) != SETTINGS_ESETRET_OK)
                {
                    ESP_LOGE(TAG, "Unable to set value for key: %s, bIsDryRun: %d, ret: %d", pSettingEntry->szKey, bIsDryRun, eSetRet);
                    goto ERROR;
                }
            }
        }
    }

    bRet = true;
    ESP_LOGI(TAG, "Import JSON completed");
    goto END;
    ERROR:
    bRet = false;
    END:
    cJSON_free(pRoot);
    return bRet;
}

static const SSettingEntry* GetSettingEntry(SETTINGS_EENTRY eEntry)
{
    if ( (int)eEntry < 0 || (int)eEntry >= SETTINGS_EENTRY_Count )
        return NULL;
    return &m_sConfigEntries[(int)eEntry];
}

static bool GetSettingEntryByKey(const char* szKey, SETTINGS_EENTRY* peEntry)
{
    for(int i = 0; i < SETTINGS_EENTRY_Count; i++)
    {
        if (strcmp(m_sConfigEntries[i].szKey, szKey) == 0)
        {
            *peEntry = (SETTINGS_EENTRY)i;
            return true;
        }
    }
    return false;
}