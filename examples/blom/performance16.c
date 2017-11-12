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
  static uint16_t a[LAMBDA];
  static uint16_t id;
  static uint8_t i;
  static uint16_t ticks[4];

  PROCESS_BEGIN();
  
  random_init(0);
  printf("ticks/s = %lu\n", CLOCK_SECOND);
  
  /* init */
  for (i = 0; i < LAMBDA; i++) {
    do {
    a[i] = random_rand();
    } while (a[i] >= 0xFFF1);
  }
  id = 1;
  
  /* invoking Blom */
  ticks[0] = clock_time();
  for (i = 0; i < ROUNDS; i++) {
    blom_0x10001_optimized(a, id);
  }
  ticks[1] = clock_time();
  for (i = 0; i < ROUNDS; i++) {
    blom_0xFFF1(a, id);
  }
  ticks[2] = clock_time();
  for (i = 0; i < ROUNDS; i++) {
    blom_0xFFF1_optimized(a, id);
  }
  ticks[3] = clock_time();
  
  printf("blom_0x10001_optimized = %u ticks\n", ticks[1] - ticks[0]);
  printf("blom_0xFFF1 = %u ticks\n", ticks[2] - ticks[1]);
  printf("blom_0xFFF1_optimized = %u ticks\n", ticks[3] - ticks[2]);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
