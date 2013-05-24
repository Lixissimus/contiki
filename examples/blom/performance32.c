/**
 * \file
 *         Performance testing of modular arithmetic.
 * \author
 *         Konrad Krentz <>
 */

#include "contiki.h"
#include <stdio.h> /* For printf() */
#include "clock.h"
#include "blom.h"
#include "random.h"

#define ROUNDS 10

/*---------------------------------------------------------------------------*/
PROCESS(performance, "performance");
AUTOSTART_PROCESSES(&performance);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(performance, ev, data)
{
  static unsigned short t;
  static uint32_t a[LAMBDA];
  static uint32_t id;
  static uint8_t i;
  static uint16_t ticks[4];

  PROCESS_BEGIN();
  
  random_init(0);
  printf("ticks/s = %lu\n", CLOCK_SECOND);
  
  /* init */
  for (i = 0; i < LAMBDA; i++) {
    a[i] = random_rand();
  }
  id = 1;
  
  /* invoking Blom */
  ticks[0] = clock_time();
  for (i = 0; i < ROUNDS; i++) {
      blom_0x100000001(a, id);
  }
  ticks[1] = clock_time();
  for (i = 0; i < ROUNDS; i++) {
      blom_0x100000001_optimized(a, id);
  }
  ticks[2] = clock_time();
  
  printf("blom_0x100000001 = %u ticks\n", ticks[1] - ticks[0]);
  printf("blom_0x100000001_optimized = %u ticks\n", ticks[2] - ticks[1]);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
