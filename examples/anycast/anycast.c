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

#include "net/ip/simple-udp.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-anycast.h"

#include <stdio.h>
#include <string.h>

#define UDP_PORT 1234



/*---------------------------------------------------------------------------*/
PROCESS(anycast_process, "Anycast process");
AUTOSTART_PROCESSES(&anycast_process);

static void
receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  printf("Data received on port %d from port %d with length %d\n",
         receiver_port, sender_port, datalen);
}




/*---------------------------------------------------------------------------*/
PROCESS_THREAD(anycast_process, ev, data)
{
  static struct simple_udp_connection anycast_connection;
  static uip_ipaddr_t anycast_addr;
  // static struct etimer wait_timer;

  PROCESS_BEGIN();

  SENSORS_ACTIVATE(button_sensor);
  /* init the anycast module */
  init_uip_anycast();
  create_anycast_addr(&anycast_addr);

  /* register the connection */
  simple_udp_register(&anycast_connection, UDP_PORT,
                      NULL, UDP_PORT,
                      receiver);

  
  while(1)
  {
    // etimer_set(&wait_timer, 10*CLOCK_SECOND);

    // PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&wait_timer));
    PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);
    if(button_sensor.value(BUTTON_SENSOR_VALUE_TYPE_LEVEL) == BUTTON_SENSOR_PRESSED_LEVEL) {
      static uint8_t msg_idx;
      char buf[20];
      printf("Sending anycast %d\n", msg_idx);
      sprintf(buf, "Message %d", msg_idx);
      simple_udp_sendto(&anycast_connection, buf,
                        strlen(buf) + 1, &anycast_addr);
      msg_idx += 1;
    }
  }
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
