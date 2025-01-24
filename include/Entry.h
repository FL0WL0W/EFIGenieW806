#include "stdint.h"

#ifndef ENTRY_H
#define ENTRY_H

#ifdef __cplusplus
extern "C" {
#endif

    void Setup(uint8_t startEngine);
    void Loop();

#ifdef __cplusplus
}
#endif

#endif