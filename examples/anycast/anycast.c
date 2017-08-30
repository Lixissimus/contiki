/*
 * Copyright (c) 2017, Hasso Plattner Institute, Potsdam, Germany.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         A simple Contiki application for testing anycast support
 * \author
 *         Felix Wolff <lixissimus@gmail.com>
 */

#include "contiki.h"

#include "dev/button-sensor.h"
#include "dev/leds.h"

#include "net/linkaddr.h"
#include "net/ip/simple-udp.h"
#include "net/ip/uip.h"
#include "net/ip/uip-debug.h"
#include "net/ipv6/uip-ds6.h"
#include "net/orpl/orpl.h"
#include "net/llsec/adaptivesec/akes-nbr.h"

#include "lib/random.h"
#include "lib/led-debug.h"

#include "lib/lladdr-id-mapping.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#define SERVICE_ID 112
#define UDP_PORT 1234
#define NETWORK_SIZE 8
#define ROOT_ID 1

#define DEBUG 1

#if DEBUG
#define ANNOTATE_H(id) printf("#H %" PRIu16 "\n", id)
#define ANNOTATE_I(id, info) printf("#I %" PRIu16 " %s\n", id, info)
#define ANNOTATE_R(id, rank) printf("#R %" PRIu16 " %" PRIu16 "\n", id, rank)
#define ANNOTATE_N(id, nbr_id) printf("#N %" PRIu16 " %" PRIu16 "\n", id, nbr_id)
#else
#define ANNOTATE_H(id)
#define ANNOTATE_I(id, info)
#define ANNOTATE_R(id, rank)
#define ANNOTATE_N(id, nbr_id)
#endif

#define PERIODIC_SEND 1

#define SEND_INTERVAL (60*CLOCK_SECOND)
#define RANDOM_FRACTION (60*CLOCK_SECOND)
#define RANDOM_INTERVAL (SEND_INTERVAL - (RANDOM_FRACTION/2) \
                        + (random_rand() % RANDOM_FRACTION))

#if PERIODIC_SEND
static struct etimer periodic_timer;
static struct etimer delay_timer;
#endif

#if DEBUG
static struct ctimer nbr_timer;
#endif

/* experiment setup */
#define STARTUP_DELAY (5*CLOCK_SECOND)
#define EXP_RUNTIME (40*60*CLOCK_SECOND)
#define SHUTDOWN_DELAY (1*CLOCK_SECOND)

#define MEASURE_DELIVERY_RATIO 1
#define MEASURE_ENERGY_CONSUMPTION 1

#if MEASURE_DELIVERY_RATIO
static uint32_t packets_received[NETWORK_SIZE];
static uint32_t last_received[NETWORK_SIZE];
static uint32_t duplicates_received[NETWORK_SIZE];
static uint32_t packets_expected = EXP_RUNTIME / SEND_INTERVAL;
#endif

#if MEASURE_ENERGY_CONSUMPTION
#include "dev/radio-async.h"
#endif

/*---------------------------------------------------------------------------*/
PROCESS(anycast_process, "Anycast process");
AUTOSTART_PROCESSES(&anycast_process);
/*---------------------------------------------------------------------------*/

static uint16_t own_id;

/*---------------------------------------------------------------------------*/
static void
print_own_addresses(void)
{
  int i;
  uint8_t state;

  printf("IPv6 addresses:\n");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      printf(" ");
      uip_debug_ipaddr_print(&uip_ds6_if.addr_list[i].ipaddr);
      printf("\n");
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  uint16_t sender_id;
  sender_id = lladdr_id_mapping_id_from_ipv6(sender_addr);
  printf("Received from %" PRIu16 ": %s\n", sender_id, data);
  uint32_t msg_number = atoi((const char*)data);
  if(msg_number <= last_received[sender_id-1]) {
    /* duplicate */
    duplicates_received[sender_id-1]++;
  } else {
    led_debug_set_all();
    ANNOTATE_H(own_id);
    last_received[sender_id-1] = msg_number;
    packets_received[sender_id-1]++;
  }
  
}
/*---------------------------------------------------------------------------*/
#if DEBUG
static void
check_status(void *d)
{
  linkaddr_t *addr;
  struct akes_nbr_entry *next;

  /* red indicates no akes neighbors */
  if(akes_nbr_count(AKES_NBR_PERMANENT) > 0) {
    leds_off(LEDS_RED);
#if MEASURE_ENERGY_CONSUMPTION
    /* init phase is done, start tracing */
    radio_async_set_tracing(1);
#endif
  } else {
    leds_on(LEDS_RED);
  }

  /* annotate neighbors */
  next = akes_nbr_head();
  while(next) {
    if(next->refs[AKES_NBR_PERMANENT]) {
      addr = akes_nbr_get_addr(next);
      ANNOTATE_N(own_id, lladdr_id_mapping_id_from_ll(addr));
    }
    next = akes_nbr_next(next);
  }

  /* yellow indicates not part of DODAG */
  uint16_t rank = orpl_current_edc();
  if(rank < 0xffff) {
    leds_off(LEDS_YELLOW);
  } else {
    leds_on(LEDS_YELLOW);
  }
  ANNOTATE_R(own_id, rank);

  ctimer_restart(&nbr_timer);
}
#endif
/*---------------------------------------------------------------------------*/
static void
print_metrics(void)
{
  uint8_t i;
  uint16_t own_id = lladdr_id_mapping_own_id();
  printf("Metrics of Node %" PRIu16 ":\n", own_id);
#if MEASURE_DELIVERY_RATIO
  if(own_id == ROOT_ID) {
    printf("Delivery Ratios:\n");
    for(i = 0; i < NETWORK_SIZE; ++i) {
      printf("Node %" PRIu8 ": %" PRIu32 "/%" PRIu32 "\n",
          i+1, packets_received[i], packets_expected
      );
    }
    printf("\n");
    printf("Duplicates:\n");
    for(i = 0; i < NETWORK_SIZE; ++i) {
      printf("Node %" PRIu8 ": %" PRIu32 "\n", i+1, duplicates_received[i]);
    }
  }
#endif
#if MEASURE_ENERGY_CONSUMPTION
  if(radio_async_is_tracing()) {
    struct duty_cycle_stats stats;
    radio_async_get_stats(&stats);
    printf("Radio on: ");
    printf("%" PRIu32 "/%" PRIu32 " (%" PRIu32 ".%" PRIu32 "%%)\n",
        stats.time_on, stats.time_total, stats.time_on * 100 / stats.time_total,
        (stats.time_on * 100 % stats.time_total) * 10 / stats.time_total
    );
  }
#endif
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(anycast_process, ev, data)
{
  static struct simple_udp_connection anycast_connection;
  uint8_t i;
  
  PROCESS_BEGIN();

#if LINKADDR_SIZE != 8
#error "This example only works with LINKADDR_SIZE 8\n"
#endif

  /* setup addresses */
  own_id = lladdr_id_mapping_own_id();
  printf("Own id: %" PRIu16 "\n", own_id);
  
  uip_ipaddr_t ipaddr;
  lladdr_id_mapping_ipv6_from_id(own_id, &ipaddr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);
  print_own_addresses();

#if !PERIODIC_SEND
  SENSORS_ACTIVATE(button_sensor);
#endif

  /* setup routing */
#if ORPL_ENABLED
  printf("Using ORPL\n");
  if(own_id == ROOT_ID) {
    rpl_dag_t *dag = rpl_set_root(RPL_DEFAULT_INSTANCE, &ipaddr);
    rpl_set_prefix(dag, &ipaddr, 64);
  }

  /* init the ORPL module */
  orpl_init(own_id == ROOT_ID);
#else
  printf("Using RPL\n");
  if(own_id == ROOT_ID) {
    rpl_dag_t *dag;
    uip_ipaddr_t prefix;
    
    rpl_set_root(RPL_DEFAULT_INSTANCE, &ipaddr);
    dag = rpl_get_any_dag();
    uip_ip6addr(&prefix, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);
    rpl_set_prefix(dag, &prefix, 64);
  }
#endif /* ORPL_ENABLED */

  /* register the connection */
  simple_udp_register(&anycast_connection, UDP_PORT,
                      NULL, UDP_PORT,
                      receiver);

  /* init measurements */
  for(i = 0; i < NETWORK_SIZE; ++i) {
    packets_received[i] = 0;
    duplicates_received[i] = 0;
    last_received[i] = 0;
  }

#if DEBUG
  ctimer_set(&nbr_timer, CLOCK_SECOND, check_status, NULL);
#endif

#if PERIODIC_SEND
  etimer_set(&delay_timer, STARTUP_DELAY);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&delay_timer));
  etimer_set(&periodic_timer, RANDOM_INTERVAL);
#endif

  static uint32_t msg_idx = 0;

  while(1) {
#if PERIODIC_SEND
#if MEASURE_DELIVERY_RATIO
    if(msg_idx >= packets_expected) {
      break;
    }
#endif
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_set(&periodic_timer, RANDOM_INTERVAL);
    print_metrics();
    if(own_id == ROOT_ID) {
      continue;
    }
#else
    PROCESS_WAIT_EVENT();
    if( ev == sensors_event &&
        data == &button_sensor &&
        button_sensor.value(BUTTON_SENSOR_VALUE_TYPE_LEVEL) ==
            BUTTON_SENSOR_PRESSED_LEVEL)
#endif /* PERIODIC_SEND */
    {
      char buf[127];
      uip_ipaddr_t addr;
      uint16_t dest_id;

      // printf("#id:%" PRIu16 "\n", own_id);

      if(akes_nbr_count(AKES_NBR_PERMANENT) <= 0) {
        printf("Node has no akes neighbors\n");
        continue;
      } else if(orpl_current_edc() == 0xffff) {
        printf("Node is not in DODAG\n");
        continue;
      } else {
        printf("current edc: %d\n", orpl_current_edc());
      }
      ANNOTATE_H(own_id);

      msg_idx++;

      // do {
      //   dest_id = (random_rand() % NETWORK_SIZE) + 1;
      // } while(dest_id == own_id);

      /* Only traffic towards root */
      dest_id = ROOT_ID;

      // ip_from_id(&addr, dest_id);
      lladdr_id_mapping_ipv6_from_id(dest_id, &addr);
      printf("Sending message from %d to %d: %" PRIu32 "/%" PRIu32 "\n", 
          own_id, dest_id, msg_idx, packets_expected
      );
      
      uip_debug_ipaddr_print(&addr);
      printf("\n");
      sprintf(buf, "%" PRIu32, msg_idx);
      led_debug_set_all();
      simple_udp_sendto(&anycast_connection, buf, strlen(buf) + 1, &addr);
    }
  }
  
  /* indicate that we're done */
  leds_on(LEDS_GREEN);

#if PERIODIC_SEND
  etimer_set(&delay_timer, SHUTDOWN_DELAY);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&delay_timer));
#endif

#if MEASURE_DELIVERY_RATIO
  printf("delivery ratios:\n");
  for(i = 0; i < NETWORK_SIZE; ++i) {
    printf("Node %" PRIu8 ": %" PRIu32 "/%" PRIu32 "\n",
        i+1, packets_received[i], packets_expected
    );
  }
  printf("\n");
  printf("duplicates:\n");
  for(i = 0; i < NETWORK_SIZE; ++i) {
    printf("Node %" PRIu8 ": %" PRIu32 "\n", i+1, duplicates_received[i]);
  }
#endif

  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
