#ifndef BLOM_H_
#define BLOM_H_

#include "contiki.h"

#ifndef LAMBDA
#define LAMBDA 128
#endif

extern uint32_t blom_0x100000001_optimized(uint32_t *a, uint32_t id);
extern uint32_t blom_0x100000001(uint32_t *a, uint32_t id);
extern uint16_t blom_0x10001_optimized(uint16_t *a, uint16_t id);
extern uint16_t blom_0x10001(uint16_t *a, uint16_t id);
extern uint16_t blom_0xFFF1_optimized(uint16_t *a, uint16_t id);
extern uint16_t blom_0xFFF1(uint16_t *a, uint16_t id);

#endif /* BLOM_H_ */
