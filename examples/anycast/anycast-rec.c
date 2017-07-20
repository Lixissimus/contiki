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

#include "dev/leds.h"

#include "net/ip/simple-udp.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-anycast.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip-debug.h"

#include <stdio.h>

#include "apps/servreg-hack/servreg-hack.h"

#define UDP_PORT 1234

static struct ctimer off_timer;

/*---------------------------------------------------------------------------*/
PROCESS(anycast_rec_process, "Anycast receiver process");
AUTOSTART_PROCESSES(&anycast_rec_process);

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
PROCESS_THREAD(anycast_rec_process, ev, data)
{
  static struct simple_udp_connection anycast_connection;
  uip_ipaddr_t ipaddr;

  PROCESS_BEGIN();

  /* init the anycast module */
  uip_anycast_init();

  /* set ip address of receiver manually */
  uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0x212, 0x4b00, 0x430, 0x5403);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_MANUAL);

  /* register the connection */
  simple_udp_register(&anycast_connection, UDP_PORT,
                      NULL, UDP_PORT,
                      receiver);

  while(1)
  {
    PROCESS_WAIT_EVENT();
  }
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
