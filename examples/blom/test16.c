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
PROCESS(test, "test");
AUTOSTART_PROCESSES(&test);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test, ev, data)
{
  static uint16_t a[LAMBDA];
  static uint16_t id;
  static uint8_t i;

  PROCESS_BEGIN();
  
  while (1) {
    for (i = 0; i < LAMBDA; i++) {
      a[i] = random_rand();
    }
    id = random_rand();

    if (blom_0x10001(a, id) != blom_0x10001_optimized(a, id)) {
        printf("ERROR");
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
