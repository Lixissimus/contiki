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

#include "net/ip/simple-udp.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip-debug.h"
#include "net/linkaddr.h"
#include "net/orpl/orpl.h"

#include "lib/random.h"

#include <stdio.h>
#include <string.h>

#define SERVICE_ID 112
#define UDP_PORT 1234
#define NETWORK_SIZE 2
#define ROOT_ID 1

static struct ctimer off_timer;

static uint16_t ip_prefix[] = { UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0x212, 0x4b00, 0x430, 0 };

#define ip_from_id(dest, id) uip_ip6addr(dest,  \
    ip_prefix[0],                               \
    ip_prefix[1],                               \
    ip_prefix[2],                               \
    ip_prefix[3],                               \
    ip_prefix[4],                               \
    ip_prefix[5],                               \
    ip_prefix[6],                               \
    id)

/*---------------------------------------------------------------------------*/
PROCESS(anycast_process, "Anycast process");
AUTOSTART_PROCESSES(&anycast_process);

static void
leds_turn_off(void *d)
{
  leds_off(LEDS_ALL);
}

static void
receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  printf("Data received on port %d from port %d with length %d: %s\n",
         receiver_port, sender_port, datalen, data);
  leds_on(LEDS_ALL);
  if(!ctimer_expired(&off_timer)) {
    ctimer_restart(&off_timer);
  } else {
    ctimer_set(&off_timer, CLOCK_SECOND / 2, leds_turn_off, NULL);
  }
}


/*---------------------------------------------------------------------------*/
PROCESS_THREAD(anycast_process, ev, data)
{
  static struct simple_udp_connection anycast_connection;
  
  PROCESS_BEGIN();

  if(LINKADDR_SIZE != 8) {
    printf("This example only works with LINKADDR_SIZE 8\n");
    PROCESS_EXIT();
  }

  static uint16_t own_id;
  uip_ipaddr_t ipaddr;
  own_id = (linkaddr_node_addr.u8[LINKADDR_SIZE-2] << 8) + linkaddr_node_addr.u8[LINKADDR_SIZE-1];
  ip_from_id(&ipaddr, own_id);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_MANUAL);
    

  SENSORS_ACTIVATE(button_sensor);

  if(own_id == ROOT_ID) {
    rpl_dag_t *dag = rpl_set_root(RPL_DEFAULT_INSTANCE, &ipaddr);
    rpl_set_prefix(dag, &ipaddr, 64);
    // NETSTACK_RDC.off(1);
  }
  
  /* init the ORPL module */
  orpl_init(own_id == ROOT_ID);

  /* register the connection */
  simple_udp_register(&anycast_connection, UDP_PORT,
                      NULL, UDP_PORT,
                      receiver);

  printf("Hello from %d\n", own_id);

  while(1) {
    PROCESS_WAIT_EVENT();
    if(ev == sensors_event && data == &button_sensor) {
      /* Send a message */
      if(button_sensor.value(BUTTON_SENSOR_VALUE_TYPE_LEVEL) == BUTTON_SENSOR_PRESSED_LEVEL) {
        static uint8_t msg_idx;
        char buf[127];
        uip_ipaddr_t addr;
        uint16_t dest_id;

        if(orpl_current_edc() == 0xffff) {
          printf("Node is not in DODAG\n");
          // continue;
        } else {
          printf("current edc: %d\n", orpl_current_edc());
        }

        // do {
        //   dest_id = random_rand() % NETWORK_SIZE + 1;
        // } while(dest_id == own_id);

        /* Only traffic to root */
        dest_id = ROOT_ID;

        ip_from_id(&addr, dest_id);
        printf("Sending anycast from %d to %d: %d\n", own_id, dest_id, msg_idx);
        sprintf(buf, "Message %d", msg_idx);
        simple_udp_sendto(&anycast_connection, buf, strlen(buf) + 1, &addr);
        msg_idx += 1;
      }
    }
  }
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
