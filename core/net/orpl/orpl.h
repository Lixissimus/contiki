/*
 * Copyright (c) 2017, Hasso-Plattner-Institut.
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
 *         Opportunistic RPL with POTR/ILOCS link-layer security.
 * \author
 *         Felix Wolff <lixissimus@gmail.com>
 */

#ifndef ORPL_H_
#define ORPL_H_

// #if ORPL_ENABLED

#include "net/linkaddr.h"
#include "net/ip/uip.h"
#include "net/rpl/rpl.h"

#ifdef ORPL_CONF_EDC_W
#define ORPL_EDC_W ORPL_CONF_EDC_W
#else /* ORPL_CONF_EDC_W */
#define ORPL_EDC_W 64
#endif /* ORPL_CONF_EDC_W */

/* Fixed point divisor */
#define EDC_DIVISOR 128

enum orpl_routing_decision {
  ORPL_ROUTE_UP,
  ORPL_ROUTE_DOWN,
  ORPL_ROUTE_KEEP,
  ORPL_ROUTE_REJECT
};

/* Total number of broadcast sent */
extern uint32_t orpl_broadcast_count;

enum orpl_routing_decision orpl_should_receive();
enum orpl_routing_decision orpl_make_routing_decision(uip_ipaddr_t *dest_addr);

/* Initialize the ORPL module */
void orpl_init(uint8_t is_root);

/* Update the rank of a node  */
void orpl_update_edc(rpl_rank_t edc);

/* Get the current edc or 0xffff, if we are not part of any DODAG */
rpl_rank_t orpl_current_edc();

/* Check if we are the root */
int orpl_is_root();

/* Called from RPL trickle callback */
void orpl_trickle_callback(rpl_instance_t *instance);

/* Flag used to tell lower layers that the current UDP transmission
 * is a routing set, so that the desired callback function is called
 * after each transmission attempt */
extern int sending_routing_set;

// #endif /* ORPL_ENABLED */

#endif /* ORPL_H_ */