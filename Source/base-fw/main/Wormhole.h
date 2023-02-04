#ifndef _WORMHOLE_H_
#define _WORMHOLE_H_

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    WORMHOLE_ETYPE_NormalSGU = 0,
    WORMHOLE_ETYPE_NormalSG1,
    WORMHOLE_ETYPE_Hell,
    /* WORMHOLE_ETYPE_Blackhole = 3 */
    /* WORMHOLE_ETYPE_Glitch = 4 */
    WORMHOLE_ETYPE_Count
} WORMHOLE_ETYPE;

typedef struct
{
    WORMHOLE_ETYPE eType;
    bool bNoTimeLimit;
} WORMHOLE_SArg;


void WORMHOLE_FullStop();

void WORMHOLE_Open(const WORMHOLE_SArg* pArg, volatile bool* pIsCancelled);

void WORMHOLE_Run(volatile bool* pIsCancelled);

void WORMHOLE_Close(volatile bool* pIsCancelled);

bool WORMHOLE_ValidateWormholeType(WORMHOLE_ETYPE eWormholeType);

#endif