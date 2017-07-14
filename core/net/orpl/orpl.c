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

#include "orpl.h"

#include "net/ipv6/uip-ds6.h"

#define DEBUG 0
#if DEBUG && MAIN_DEBUG_CONF
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else /* DEBUG */
#define PRINTF(...)
#endif /* DEBUG */

/* Decide whether we generally want to receive a
 * packet from that source. This is called from
 * secrdc.c interrupt routine. 
 */
enum orpl_routing_decision
/* orpl_should_receive(const linkaddr_t *src_addr, uint8_t type) */
orpl_should_receive()
{
    return ORPL_ROUTE_KEEP;
}

/* Decide whether to route the packet upwards, 
 * downwards, or keep it (because it's for us) 
 */
enum orpl_routing_decision
orpl_make_routing_decision(uip_ipaddr_t *dest_addr)
{
    if(uip_ds6_is_my_addr(dest_addr)) {
        return ORPL_ROUTE_KEEP;
    }
    /* just routing upwards for now */
    return ORPL_ROUTE_UP;
}