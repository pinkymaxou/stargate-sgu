#ifndef _SGUHELPER_H_
#define _SGUHELPER_H_

#ifdef __cplusplus
extern "C" {
#endif

double SGUHELPER_LEDIndexToDeg(int ledIndex);

int SGUHELPER_ChevronIndexToLedIndex(int chevronIndexOneBased);

int SGUHELPER_SymbolIndexToLedIndex(int symbolIndexOneBased);

#ifdef __cplusplus
}
#endif

#endif