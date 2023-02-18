#ifndef _NVSJSON_H_
#define _NVSJSON_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "nvs.h"

typedef enum
{
    NVSJSON_ESETRET_OK = 0,
    NVSJSON_ESETRET_CannotSet = 1,
    NVSJSON_ESETRET_InvalidRange = 2,
    NVSJSON_ESETRET_ValidatorFailed = 3,
} NVSJSON_ESETRET;

typedef enum
{
    NVSJSON_EFLAGS_None = 0,
    NVSJSON_EFLAGS_Secret = 1,      // Indicate it cannot be retrieved, only set
    NVSJSON_EFLAGS_NeedsReboot = 2
} NVSJSON_EFLAGS;

typedef enum
{
    NVSJSON_ETYPE_Int32,
    NVSJSON_ETYPE_String,
    NVSJSON_ETYPE_Double
} NVSJSON_ETYPE;

typedef struct _NVSJSON_SSettingEntry NVSJSON_SSettingEntry;

typedef bool (*PtrValidateInt32)(const NVSJSON_SSettingEntry* pSettingEntry, int32_t s32Value);
typedef bool (*PtrValidateDouble)(const NVSJSON_SSettingEntry* pSettingEntry, int32_t dlbValue);
typedef bool (*PtrValidateString)(const NVSJSON_SSettingEntry* pSettingEntry, const char* szValue);

typedef union
{
    struct
    {
       int32_t s32Min;
       int32_t s32Max;
       int32_t s32Default;
       PtrValidateInt32 ptrValidator;
    } sInt32;
    struct
    {
       double dMin;
       double dMax;
       double dDefault;
       PtrValidateDouble ptrValidator;
    } sDouble;
    struct
    {
       char* szDefault;
       PtrValidateString ptrValidator;
    } sString;
} NVSJSON_UConfig;

typedef struct _NVSJSON_SSettingEntry
{
    const char* szKey;
    const char* szDesc;
    NVSJSON_ETYPE eType;
    NVSJSON_UConfig uConfig;
    NVSJSON_EFLAGS eFlags;
} NVSJSON_SSettingEntry;

typedef struct
{
	nvs_handle_t sNVS;
	// Entries
	const NVSJSON_SSettingEntry* pSettingEntries;
	uint32_t u32SettingEntryCount;
} NVSJSON_SHandle;

#define NVSJSON_GETVALUESTRING_MAXLEN (100)
		
#define NVSJSON_INITSTRING_RNG(_szKey, _szDesc, _szDefault, _eFlags) { .szKey = _szKey, .eType = NVSJSON_ETYPE_String,.szDesc = _szDesc, .uConfig = { .sString = { .szDefault = _szDefault, .ptrValidator = NULL } }, .eFlags = _eFlags }
#define NVSJSON_INITSTRING_VALIDATOR(_szKey, _szDesc, _szDefault, _ptrValidateString, _eFlags) { .szKey = _szKey, .eType = NVSJSON_ETYPE_String,.szDesc = _szDesc, .uConfig = { .sString = { .szDefault = _szDefault, .ptrValidator = _ptrValidateString } }, .eFlags = _eFlags }

#define NVSJSON_INITDOUBLE_RNG(_szKey, _szDesc, _dDefault, _dMin, _dMax, _eFlags) { .szKey = _szKey, .eType = NVSJSON_ETYPE_Double,.szDesc = _szDesc, .uConfig = { .sDouble = { .dDefault = _dDefault, .dMin = _dMin, .dMax = _dMax, .ptrValidator = NULL } }, .eFlags = _eFlags }
#define NVSJSON_INITDOUBLE_VALIDATOR(_szKey, _szDesc, _dDefault, _ptrValidateDouble, _eFlags) { .szKey = _szKey, .eType = NVSJSON_ETYPE_Double,.szDesc = _szDesc, .uConfig = { .sDouble = { .dDefault = _dDefault, .ptrValidator = _ptrValidateDouble } }, .eFlags = _eFlags }

#define NVSJSON_INITINT32_RNG(_szKey, _szDesc, _s32Default, _s32Min, _s32Max, _eFlags) { .szKey = _szKey, .eType = NVSJSON_ETYPE_Int32,.szDesc = _szDesc, .uConfig = { .sInt32 = { .s32Default = _s32Default, .s32Min = _s32Min, .s32Max = _s32Max, .ptrValidator = NULL } }, .eFlags = _eFlags }
#define NVSJSON_INITINT32_VALIDATOR(_szKey, _szDesc, _s32Default, _ptrValidateInt32, _eFlags) { .szKey = _szKey, .eType = NVSJSON_ETYPE_Int32,.szDesc = _szDesc, .uConfig = { .sInt32 = { .s32Default = _s32Default, .ptrValidator = _ptrValidateInt32 } }, .eFlags = _eFlags }

void NVSJSON_Init(NVSJSON_SHandle* pHandle, const NVSJSON_SSettingEntry* pSettingEntries, uint32_t u32SettingEntryCount);
void NVSJSON_Load(NVSJSON_SHandle* pHandle);
void NVSJSON_Save(NVSJSON_SHandle* pHandle);

int32_t NVSJSON_GetValueInt32(NVSJSON_SHandle* pHandle, uint16_t u16Entry);
NVSJSON_ESETRET NVSJSON_SetValueInt32(NVSJSON_SHandle* pHandle, uint16_t u16Entry, bool bIsDryRun, int32_t s32NewValue);

double NVSJSON_GetValueDouble(NVSJSON_SHandle* pHandle, uint16_t u16Entry);
NVSJSON_ESETRET NVSJSON_SetValueDouble(NVSJSON_SHandle* pHandle, uint16_t u16Entry, bool bIsDryRun, double dNewValue);

void NVSJSON_GetValueString(NVSJSON_SHandle* pHandle, uint16_t u16Entry, char* out_value, size_t* length);
NVSJSON_ESETRET NVSJSON_SetValueString(NVSJSON_SHandle* pHandle, uint16_t u16Entry, bool bIsDryRun, const char* szValue);

char* NVSJSON_ExportJSON(NVSJSON_SHandle* pHandle);
bool NVSJSON_ImportJSON(NVSJSON_SHandle* pHandle, const char* szJSON);


#endif