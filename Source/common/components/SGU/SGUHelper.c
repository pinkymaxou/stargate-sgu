#include "SGUHelper.h"

double SGUHELPER_LEDIndexToDeg(int ledIndex)
{
    if (ledIndex < 0 || ledIndex >= 45)
        return 0.0d;

    const double _1of54 = 360.0d / 54.0d;
    const double _1of9 = 360.0d / 9.0d;

    return (ledIndex / 5) * _1of9 + (((ledIndex % 5) != 0) ? 10 + ((ledIndex % 5)-1) * _1of54 : 0);
}

int SGUHELPER_ChevronIndexToLedIndex(int chevronIndexOneBased)
{
    if (chevronIndexOneBased < 1 || chevronIndexOneBased > 9)
        return 0;

    int chevronIndex0Based = chevronIndexOneBased - 1;
    return chevronIndex0Based * 5;
}

int SGUHELPER_SymbolIndexToLedIndex(int symbolIndexOneBased)
{
    if (symbolIndexOneBased < 1 || symbolIndexOneBased > 36)
        return 0;

    int symbolIndex0Based = symbolIndexOneBased - 1;
    return symbolIndexOneBased + (symbolIndex0Based / 4);
}
