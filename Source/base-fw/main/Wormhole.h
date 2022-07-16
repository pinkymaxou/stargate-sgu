#ifndef _WORMHOLE_H_
#define _WORMHOLE_H_

#include <stdbool.h>
#include <stdint.h>

void WORMHOLE_FullStop();

void WORMHOLE_Open(volatile bool* pIsCancelled);

void WORMHOLE_Run(volatile bool* pIsCancelled, bool bNoTimeLimit);

void WORMHOLE_Close(volatile bool* pIsCancelled);

#endif