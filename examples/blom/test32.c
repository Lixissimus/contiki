/**
 * \file
 *         Performance testing of modular arithmetic.
 * \author
 *         Konrad Krentz
 */

#include "contiki.h"
#include <stdio.h> /* For printf() */
#include "clock.h"
#include "blom.h"
#include "random.h"

#define ROUNDS 10

/*---------------------------------------------------------------------------*/
PROCESS(test32, "test32");
AUTOSTART_PROCESSES(&test32);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test32, ev, data)
{
  static uint32_t a[LAMBDA];
  static uint32_t id;
  static uint8_t i;

  PROCESS_BEGIN();
  
  while (1) {
    for (i = 0; i < LAMBDA; i++) {
      a[i] = random_rand();
    }
    id = random_rand();

    if (blom_0x100000001_optimized(a, id) != blom_0x100000001(a, id)) {
        printf("ERROR\n");
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
