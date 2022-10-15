#include <stdio.h>
#include "nvsjson.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <assert.h>
#include "cJSON.h"
#include "esp_log.h"
#include <string.h>

#define TAG "SETTINGS"
#define PARTITION_NAME "nvs"

// JSON entries
#define JSON_ENTRIES_NAME "entries"

#define JSON_ENTRY_KEY_NAME "key"
#define JSON_ENTRY_VALUE_NAME "value"

#define JSON_ENTRY_INFO_NAME "info"

#define JSON_ENTRY_INFO_DESC_NAME "desc"
#define JSON_ENTRY_INFO_DEFAULT_NAME "default"
#define JSON_ENTRY_INFO_MIN_NAME "min"
#define JSON_ENTRY_INFO_MAX_NAME "max"
#define JSON_ENTRY_INFO_TYPE_NAME "type"
#define JSON_ENTRY_INFO_FLAG_REBOOT_NAME "flag_reboot"

static const NVSJSON_SSettingEntry* GetSettingEntry(NVSJSON_SHandle* pHandle, uint16_t eEntry);
static bool GetSettingEntryByKey(NVSJSON_SHandle* pHandle, const char* szKey, uint16_t* pU16Entry);

void NVSJSON_Init(NVSJSON_SHandle* pHandle, const NVSJSON_SSettingEntry* pSettingEntries, uint32_t u32SettingEntryCount)
{
	pHandle->pSettingEntries = pSettingEntries;
	pHandle->u32SettingEntryCount = u32SettingEntryCount;
	
    ESP_ERROR_CHECK(nvs_open(PARTITION_NAME, NVS_READWRITE, &pHandle->sNVS));
}

void NVSJSON_Load(NVSJSON_SHandle* pHandle)
{
	
}

void NVSJSON_Save(NVSJSON_SHandle* pHandle)
{
    ESP_ERROR_CHECK(nvs_commit(pHandle->sNVS));
}

int32_t NVSJSON_GetValueInt32(NVSJSON_SHandle* pHandle, uint16_t u16Entry)
{
    const NVSJSON_SSettingEntry* pEnt = GetSettingEntry(pHandle, u16Entry);
    assert(pEnt != NULL && pEnt->eType == NVSJSON_ETYPE_Int32);
    int32_t s32 = 0;
    if (nvs_get_i32(pHandle->sNVS, pEnt->szKey, &s32) == ESP_OK)
        return s32;
    return pEnt->uConfig.sInt32.s32Default;
}

NVSJSON_ESETRET NVSJSON_SetValueInt32(NVSJSON_SHandle* pHandle, uint16_t u16Entry, bool bIsDryRun, int32_t s32NewValue)
{
    const NVSJSON_SSettingEntry* pEnt = GetSettingEntry(pHandle, u16Entry);
    assert(pEnt != NULL && pEnt->eType == NVSJSON_ETYPE_Int32);
        
    if (pEnt->uConfig.sInt32.ptrValidator != NULL)
    {
        if (!pEnt->uConfig.sInt32.ptrValidator(pEnt, s32NewValue))
            return NVSJSON_ESETRET_ValidatorFailed;
    }
    else
    {
        if (s32NewValue < pEnt->uConfig.sInt32.s32Min || s32NewValue > pEnt->uConfig.sInt32.s32Max)
            return NVSJSON_ESETRET_InvalidRange;
    }

    if (!bIsDryRun)
    {
        const esp_err_t err = nvs_set_i32(pHandle->sNVS, pEnt->szKey, s32NewValue);
        ESP_ERROR_CHECK_WITHOUT_ABORT(err);
        if (err != ESP_OK)
            return NVSJSON_ESETRET_CannotSet;
    }
    return NVSJSON_ESETRET_OK;
}

double NVSJSON_GetValueDouble(NVSJSON_SHandle* pHandle, uint16_t u16Entry)
{
    const NVSJSON_SSettingEntry* pEnt = GetSettingEntry(pHandle, u16Entry);
    assert(pEnt != NULL && pEnt->eType == NVSJSON_ETYPE_Double);
    double dbl = 0;
    size_t len = sizeof(double);
    if (nvs_get_blob(pHandle->sNVS, pEnt->szKey, (void*)&dbl, &len) == ESP_OK)
        return dbl;
    return pEnt->uConfig.sDouble.dDefault;
}

NVSJSON_ESETRET NVSJSON_SetValueDouble(NVSJSON_SHandle* pHandle, uint16_t u16Entry, bool bIsDryRun, double dNewValue)
{
    const NVSJSON_SSettingEntry* pEnt = GetSettingEntry(pHandle, u16Entry);
    assert(pEnt != NULL && pEnt->eType == NVSJSON_ETYPE_Double);

    // Default value if it cannot retrieve it ...
    if (pEnt->uConfig.sDouble.ptrValidator != NULL)
    {
        if (!pEnt->uConfig.sDouble.ptrValidator(pEnt, dNewValue))
            return NVSJSON_ESETRET_ValidatorFailed;
    }
    else
    {
        if (dNewValue < pEnt->uConfig.sDouble.dMin || dNewValue > pEnt->uConfig.sDouble.dMax)
            return NVSJSON_ESETRET_InvalidRange;
    }

    if (!bIsDryRun)
    {
        const esp_err_t err = nvs_set_blob(pHandle->sNVS, pEnt->szKey, &dNewValue, sizeof(double));
        ESP_ERROR_CHECK_WITHOUT_ABORT(err);
        if (err != ESP_OK)
            return NVSJSON_ESETRET_CannotSet;
    }
    return NVSJSON_ESETRET_OK;
}

void NVSJSON_GetValueString(NVSJSON_SHandle* pHandle, uint16_t u16Entry, char* out_value, size_t* length)
{
    const NVSJSON_SSettingEntry* pEnt = GetSettingEntry(pHandle, u16Entry);
    assert(pEnt != NULL && pEnt->eType == NVSJSON_ETYPE_String);

    if (nvs_get_str(pHandle->sNVS, pEnt->szKey, out_value, length) != ESP_OK)
    {
        if (pEnt->uConfig.sString.szDefault != NULL)
        {
            *length = strlen(pEnt->uConfig.sString.szDefault);
            strncpy(out_value, pEnt->uConfig.sString.szDefault, *length);
        }
    }
}

NVSJSON_ESETRET NVSJSON_SetValueString(NVSJSON_SHandle* pHandle, uint16_t u16Entry, bool bIsDryRun, const char* szValue)
{
    const NVSJSON_SSettingEntry* pEnt = GetSettingEntry(pHandle, u16Entry);
    assert(pEnt != NULL && pEnt->eType == NVSJSON_ETYPE_String);

    // Default value if it cannot retrieve it ...
    if (pEnt->uConfig.sString.ptrValidator != NULL)
    {
        if (!pEnt->uConfig.sString.ptrValidator(pEnt, szValue))
            return NVSJSON_ESETRET_ValidatorFailed;
    }

    if (!bIsDryRun)
    {
        const esp_err_t err = nvs_set_str(pHandle->sNVS, pEnt->szKey, szValue);
        ESP_ERROR_CHECK_WITHOUT_ABORT(err);
        if (err != ESP_OK)
            return NVSJSON_ESETRET_CannotSet;
    }
    return NVSJSON_ESETRET_OK;
}

const char* NVSJSON_ExportJSON(NVSJSON_SHandle* pHandle)
{
    cJSON* pRoot = cJSON_CreateObject();
    if (pRoot == NULL)
        goto ERROR;

    cJSON* pEntries = cJSON_AddArrayToObject(pRoot, JSON_ENTRIES_NAME);

    for(int i = 0; i < pHandle->u32SettingEntryCount; i++)
    {
        uint16_t u16Entry = (uint16_t)i;
        const NVSJSON_SSettingEntry* pEntry = GetSettingEntry( pHandle, u16Entry );

        cJSON* pEntryJSON = cJSON_CreateObject();
        cJSON_AddItemToObject(pEntryJSON, JSON_ENTRY_KEY_NAME, cJSON_CreateString(pEntry->szKey));

        cJSON* pEntryInfoJSON = cJSON_CreateObject();
        
        // Description and flags apply everywhere
        cJSON_AddItemToObject(pEntryInfoJSON, JSON_ENTRY_INFO_DESC_NAME, cJSON_CreateString(pEntry->szDesc));
        cJSON_AddItemToObject(pEntryInfoJSON, JSON_ENTRY_INFO_FLAG_REBOOT_NAME, cJSON_CreateNumber((pEntry->eFlags & NVSJSON_EFLAGS_NeedsReboot)? 1 : 0));

        if (pEntry->eType == NVSJSON_ETYPE_Int32)
        {
            if ((pEntry->eFlags & NVSJSON_EFLAGS_Secret) != NVSJSON_EFLAGS_Secret)
                cJSON_AddItemToObject(pEntryJSON, JSON_ENTRY_VALUE_NAME, cJSON_CreateNumber(NVSJSON_GetValueInt32(pHandle, u16Entry)));

            cJSON_AddItemToObject(pEntryInfoJSON, JSON_ENTRY_INFO_DEFAULT_NAME, cJSON_CreateNumber(pEntry->uConfig.sInt32.s32Default));

            if (pEntry->uConfig.sInt32.ptrValidator == NULL)
            {
                cJSON_AddItemToObject(pEntryInfoJSON, JSON_ENTRY_INFO_MIN_NAME, cJSON_CreateNumber(pEntry->uConfig.sInt32.s32Min));
                cJSON_AddItemToObject(pEntryInfoJSON, JSON_ENTRY_INFO_MAX_NAME, cJSON_CreateNumber(pEntry->uConfig.sInt32.s32Max));
            }
            cJSON_AddItemToObject(pEntryInfoJSON, JSON_ENTRY_INFO_TYPE_NAME, cJSON_CreateString("int32"));
        }
        else if (pEntry->eType == NVSJSON_ETYPE_Double)
        {
            if ((pEntry->eFlags & NVSJSON_EFLAGS_Secret) != NVSJSON_EFLAGS_Secret)
                cJSON_AddItemToObject(pEntryJSON, JSON_ENTRY_VALUE_NAME, cJSON_CreateNumber(NVSJSON_GetValueDouble(pHandle, u16Entry)));

            cJSON_AddItemToObject(pEntryInfoJSON, JSON_ENTRY_INFO_DEFAULT_NAME, cJSON_CreateNumber(pEntry->uConfig.sDouble.dDefault));
            if (pEntry->uConfig.sDouble.ptrValidator == NULL)
            {
                cJSON_AddItemToObject(pEntryInfoJSON, JSON_ENTRY_INFO_MIN_NAME, cJSON_CreateNumber(pEntry->uConfig.sDouble.dMin));
                cJSON_AddItemToObject(pEntryInfoJSON, JSON_ENTRY_INFO_MAX_NAME, cJSON_CreateNumber(pEntry->uConfig.sDouble.dMax));
            }
            cJSON_AddItemToObject(pEntryInfoJSON, JSON_ENTRY_INFO_TYPE_NAME, cJSON_CreateString("double"));
        }
        else if (pEntry->eType == NVSJSON_ETYPE_String)
        {
            char value[NVSJSON_GETVALUESTRING_MAXLEN+1] = {0,};
            size_t length = NVSJSON_GETVALUESTRING_MAXLEN;
            if ((pEntry->eFlags & NVSJSON_EFLAGS_Secret) != NVSJSON_EFLAGS_Secret)
            {
                NVSJSON_GetValueString(pHandle, u16Entry, value, &length);
                cJSON_AddItemToObject(pEntryJSON, JSON_ENTRY_VALUE_NAME, cJSON_CreateString(value));
            }
            cJSON_AddItemToObject(pEntryInfoJSON, JSON_ENTRY_INFO_DEFAULT_NAME, cJSON_CreateString(pEntry->uConfig.sString.szDefault));
            cJSON_AddItemToObject(pEntryInfoJSON, JSON_ENTRY_INFO_TYPE_NAME, cJSON_CreateString("string"));
        }

        cJSON_AddItemToObject(pEntryJSON, JSON_ENTRY_INFO_NAME, pEntryInfoJSON);

        cJSON_AddItemToArray(pEntries, pEntryJSON);
    }
    const char* pStr =  cJSON_PrintUnformatted(pRoot);
    cJSON_Delete(pRoot);
    return pStr;
    ERROR:
    cJSON_Delete(pRoot);
    return NULL;
}

bool NVSJSON_ImportJSON(NVSJSON_SHandle* pHandle, const char* szJSON)
{
    bool bRet = true;
    cJSON* pRoot = cJSON_Parse(szJSON);

    cJSON* pEntriesArray = cJSON_GetObjectItem(pRoot, JSON_ENTRIES_NAME);
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

            cJSON* pKeyJSON = cJSON_GetObjectItem(pEntryJSON, JSON_ENTRY_KEY_NAME);
            if (pKeyJSON == NULL || !cJSON_IsString(pKeyJSON))
            {
                ESP_LOGE(TAG, "Cannot find JSON key element");
                goto ERROR;
            }

            cJSON* pValueJSON = cJSON_GetObjectItem(pEntryJSON, JSON_ENTRY_VALUE_NAME);
            if (pValueJSON == NULL)
            {
                // We just ignore changing the setting if the value property is not there.
                // it allows us to handle secret cases.
                ESP_LOGD(TAG, "JSON value is not there, skipping it");
                continue;
            }

            uint16_t u16Entry;
            if (!GetSettingEntryByKey(pHandle, pKeyJSON->valuestring, &u16Entry))
            {
                ESP_LOGE(TAG, "Key: '%s' is not valid", pKeyJSON->valuestring);
                goto ERROR;
            }

            const NVSJSON_SSettingEntry* pSettingEntry = GetSettingEntry(pHandle, u16Entry);

            if (pSettingEntry->eType == NVSJSON_ETYPE_Int32)
            {
                if (!cJSON_IsNumber(pValueJSON))
                {
                    ESP_LOGE(TAG, "JSON value type is invalid, not a number");
                    goto ERROR;
                }
                int32_t s32 = pValueJSON->valueint;
                NVSJSON_ESETRET eSetRet;
                if ((eSetRet = NVSJSON_SetValueInt32(pHandle, u16Entry, bIsDryRun, s32)) != NVSJSON_ESETRET_OK)
                {
                    ESP_LOGE(TAG, "Unable to set value for key: %s, bIsDryRun: %d, ret: %d", pSettingEntry->szKey, bIsDryRun, eSetRet);
                    goto ERROR;
                }
            }
            else if (pSettingEntry->eType == NVSJSON_ETYPE_Double)
            {
                if (!cJSON_IsNumber(pValueJSON))
                {
                    ESP_LOGE(TAG, "JSON value type is invalid, not a number");
                    goto ERROR;
                }
                double dbl = (float)pValueJSON->valuedouble;
                NVSJSON_ESETRET eSetRet;
                if ((eSetRet = NVSJSON_SetValueDouble(pHandle, u16Entry, bIsDryRun, dbl)) != NVSJSON_ESETRET_OK)
                {
                    ESP_LOGE(TAG, "Unable to set value for key: %s, bIsDryRun: %d, ret: %d", pSettingEntry->szKey, bIsDryRun, eSetRet);
                    goto ERROR;
                }
            }
            else if (pSettingEntry->eType == NVSJSON_ETYPE_String)
            {
                if (!cJSON_IsString(pValueJSON))
                {
                    ESP_LOGE(TAG, "JSON value type is invalid, not a string");
                    goto ERROR;
                }
                
                const char* str = pValueJSON->valuestring;
                NVSJSON_ESETRET eSetRet;
                if ((eSetRet = NVSJSON_SetValueString(pHandle, u16Entry, bIsDryRun, str)) != NVSJSON_ESETRET_OK)
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

static const NVSJSON_SSettingEntry* GetSettingEntry(NVSJSON_SHandle* pHandle, uint16_t u16Entry)
{
    if ( (int)u16Entry >= pHandle->u32SettingEntryCount)
        return NULL;
    return &pHandle->pSettingEntries[(int)u16Entry];
}

static bool GetSettingEntryByKey(NVSJSON_SHandle* pHandle, const char* szKey, uint16_t* pu16Entry)
{
    for(int i = 0; i < pHandle->u32SettingEntryCount; i++)
    {
        if (strcmp(pHandle->pSettingEntries[i].szKey, szKey) == 0)
        {
            *pu16Entry = (uint16_t)i;
            return true;
        }
    }
    return false;
}