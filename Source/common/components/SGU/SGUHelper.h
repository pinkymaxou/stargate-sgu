#ifndef _SGUHELPER_H_
#define _SGUHELPER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

double SGUHELPER_LEDIndexToDeg(int32_t ledIndex);

int32_t SGUHELPER_ChevronIndexToLedIndex(int32_t chevronIndexOneBased);

int32_t SGUHELPER_SymbolIndexToLedIndex(int32_t symbolIndexOneBased);

bool SGUHELPER_IsLEDIndexChevron(int32_t ledIndex);

#ifdef __cplusplus
}
#endif

#endif