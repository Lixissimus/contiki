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

// #if ORPL_ENABLED

#include "net/ipv6/uip-ds6.h"
#include "net/packetbuf.h"
#include "net/linkaddr.h"
#include "net/ip/uip.h"
#include "net/ip/simple-udp.h"
#include "net/rpl/rpl.h"
#include "net/rpl/rpl-private.h"

#include "lib/random.h"
#include "lib/lladdr-id-mapping.h"

#include "orpl-routing-set.h"
#include "orpl.h"
#include <stdio.h>

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#include <inttypes.h>
#define ANNOTATE_L(src, dst) printf("#L %" PRIu16 " %" PRIu16 "\n", src, dst)
#else /* DEBUG */
#define PRINTF(...)
#define ANNOTATE_L(src, dst)
#endif /* DEBUG */

#define PRINT6ADDR(addr) PRINTF(                                                \
    " %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x ",\
    ((uint8_t *)addr)[0], ((uint8_t *)addr)[1],                                 \
    ((uint8_t *)addr)[2], ((uint8_t *)addr)[3],                                 \
    ((uint8_t *)addr)[4], ((uint8_t *)addr)[5],                                 \
    ((uint8_t *)addr)[6], ((uint8_t *)addr)[7],                                 \
    ((uint8_t *)addr)[8], ((uint8_t *)addr)[9],                                 \
    ((uint8_t *)addr)[10], ((uint8_t *)addr)[11],                               \
    ((uint8_t *)addr)[12], ((uint8_t *)addr)[13],                               \
    ((uint8_t *)addr)[14], ((uint8_t *)addr)[15])

#if ORPL_CONF_DOWNWARDS_ROUTES
/* UDP port used for routing set broadcasting */
#define ROUTING_SET_PORT 4444
/* UDP connection used for routing set broadcasting */
static struct simple_udp_connection routing_set_connection;
/* Multicast IP address used for routing set broadcasting */
static uip_ipaddr_t routing_set_addr;
/* Timer for periodic broadcast of routing sets */
static struct ctimer routing_set_broadcast_timer;
/* Flag used to tell lower layers that the current UDP transmission
 * is a routing set, so that the desired callback function is called
 * after each transmission attempt */
int sending_routing_set = 0;
/* Data structure used for routing set broadcasting. */
struct routing_set_broadcast_s {
  uint16_t edc;
  union {
    struct routing_set_s rs;
    uint8_t padding[64];
  };
};

static void broadcast_routing_set(void *ptr);
static void request_routing_set_broadcast();
#endif /* ORPL_CONF_DOWNWARDS_ROUTES */

/* Rank changes of more than RANK_MAX_CHANGE trigger a trickle timer reset */
#define RANK_MAX_CHANGE (2*EDC_DIVISOR)

/* A flag that tells whether we are root or not */
static int is_root_flag = 0;

/* Total number of broadcast sent */
uint32_t orpl_broadcast_count = 0;

/* The last boradcasted EDC */
static uint16_t last_broadcasted_edc = 0xffff;

/* The current RPL instance */
static rpl_instance_t *curr_instance;

/* Decide whether we generally want to receive a
 * packet from that source. This is called from
 * secrdc.c interrupt routine.
 */
enum orpl_routing_decision
orpl_should_receive()
{
  uint8_t *p = packetbuf_hdrptr();
  // uint8_t type = p[0];
  linkaddr_t addr;
  p += 1;
  memcpy(addr.u8, p, LINKADDR_SIZE);

  rpl_rank_t src_rank = rpl_get_parent_rank((uip_lladdr_t *) &addr);

  /* Dont accept packets from nodes with infinite rank */
  if(src_rank == 0xffff) {
    return ORPL_ROUTE_REJECT;
  }
  /* Todo: check if routing upwards by checking anycast type */
  if(1) {
    if(src_rank > orpl_current_edc() + ORPL_CONF_MIN_PROGRESS) {
      // PRINTF("ORPL: keep packet, routing upwards\n");
      ANNOTATE_L(lladdr_id_mapping_id_from_ll(&addr), lladdr_id_mapping_own_id());
      return ORPL_ROUTE_KEEP;
    } else {
      // PRINTF("ORPL: reject packet routing upwards\n");
      return ORPL_ROUTE_REJECT;
    }
  }
  #if ORPL_CONF_DOWNWARDS_ROUTES
  else {
    PRINTF("ORPL: downwards routing not implemented yet\n");
    return ORPL_ROUTE_REJECT;
    /* for downwards routing enforce src_rank < orpl_current_edc() */
  }
  #endif /* ORPL_CONF_DOWNWARDS_ROUTES */

  return ORPL_ROUTE_REJECT;
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

#if ORPL_CONF_DOWNWARDS_ROUTES
/* UDP callback function for received routing sets */
static void
udp_received_routing_set(struct simple_udp_connection *c,
     const uip_ipaddr_t *sender_addr,
     uint16_t sender_port,
     const uip_ipaddr_t *receiver_addr,
     uint16_t receiver_port,
     const uint8_t *payload,
     uint16_t datalen)
{
  struct routing_set_broadcast_s *data = (struct routing_set_broadcast_s *)payload;

  /* EDC: store edc as neighbor attribute, update metric */
  uint16_t neighbor_edc = data->edc;
  rpl_set_parent_rank((uip_lladdr_t *)packetbuf_addr(PACKETBUF_ADDR_SENDER), neighbor_edc);
  rpl_recalculate_ranks();

  /* Todo: check if the sender is a reachable neighbor, see original orpl code */


  /* Todo: include w */
  if(orpl_current_edc() < neighbor_edc) {
    int bit_count_before = orpl_routing_set_count_bits();
    int bit_count_after;
    /* Insert the neighbor in our routing set */
    orpl_routing_set_insert(sender_addr);
    PRINTF("ORPL: inserting neighbor into routing set:");
    PRINT6ADDR(sender_addr);
    PRINTF("\n");

    /* The neighbor is a child, merge its routing set into ours */ \
    orpl_routing_set_merge((const struct routing_set_s *)
      &((struct routing_set_broadcast_s*)data)->rs);
    PRINTF("ORPL: merging routing set from:");
    PRINT6ADDR(sender_addr);
    PRINTF("\n");
    
    /* Broadcast our routing set again if it has changed */
    bit_count_after = orpl_routing_set_count_bits();
    if(curr_instance && bit_count_after != bit_count_before) {
      request_routing_set_broadcast();
    }

    orpl_routing_set_print();
  }
}

/* Schedule a routing set broadcast in a few seconds */
static void
request_routing_set_broadcast()
{
  PRINTF("ORPL: requesting routing set broadcast\n");
  ctimer_set(&routing_set_broadcast_timer, random_rand() % (32 * CLOCK_SECOND), broadcast_routing_set, NULL);
}

/* Broadcast our routing set to all neighbors */
static void
broadcast_routing_set(void *ptr)
{
  /* Todo: check if routing sets are active */

  struct routing_set_broadcast_s routing_set_broadcast;
  rpl_rank_t curr_edc = orpl_current_edc();

  PRINTF("ORPL: broadcast routing set (edc=%u)\n", curr_edc);

  /* Build data structure to be broadcasted */
  last_broadcasted_edc = curr_edc;
  routing_set_broadcast.edc = curr_edc;
  memcpy(&routing_set_broadcast.rs, orpl_routing_set_get_active(), sizeof(struct routing_set_s));

  /* Proceed to UDP transmission */
  sending_routing_set = 1;
  simple_udp_sendto(&routing_set_connection, &routing_set_broadcast, sizeof(struct routing_set_broadcast_s), &routing_set_addr);
  sending_routing_set = 0;
}
#endif /* ORPL_CONF_DOWNWARDS_ROUTES */

rpl_rank_t
orpl_current_edc()
{
  rpl_dag_t *dag = rpl_get_any_dag();
  return dag == NULL ? 0xffff : dag->rank;
}

void
orpl_update_edc(rpl_rank_t edc)
{
  rpl_rank_t curr_edc = orpl_current_edc();
  rpl_dag_t *dag = rpl_get_any_dag();

  if(dag) {
    dag->rank = edc;
  }

  PRINTF("ORPL: update edc to %d\n", edc);

  /* Reset DIO timer if the edc changed significantly */
  if(curr_instance && last_broadcasted_edc != 0xffff &&
    ((last_broadcasted_edc > curr_edc && last_broadcasted_edc - curr_edc > RANK_MAX_CHANGE) ||
    (curr_edc > last_broadcasted_edc && curr_edc - last_broadcasted_edc > RANK_MAX_CHANGE))) {
    PRINTF("ORPL: reset DIO timer (edc changed from %u to %u)\n", last_broadcasted_edc, curr_edc);
    last_broadcasted_edc = curr_edc;
    rpl_reset_dio_timer(curr_instance);
  }

  /* Update EDC annotation */
  if(edc != curr_edc) {
    PRINTF("#A edc=%u.%u\n", edc/EDC_DIVISOR, (10 * (edc % EDC_DIVISOR)) / EDC_DIVISOR);
  }
}

void
orpl_trickle_callback(rpl_instance_t *instance)
{
  curr_instance = instance;

  /* Todo: implement aging by swapping routing sets, see original orpl code */
#if ORPL_CONF_DOWNWARDS_ROUTES
  /* broadcast our routing set */
  request_routing_set_broadcast();
#endif /* ORPL_CONF_DOWNWARDS_ROUTES */

  rpl_recalculate_ranks();
}

int
orpl_is_root()
{
  return is_root_flag;
}

void
orpl_init(uint8_t is_root)
{
  /* Init RPL module */
  rpl_init();

  if(is_root) {
    /* Root has rank of 0 */
    orpl_update_edc(0);
  }

#if ORPL_CONF_DOWNWARDS_ROUTES
  /* Init the routing set */
  orpl_routing_set_init();

  /* Set up multicast UDP connection for dissemination of routing sets */
  uip_create_linklocal_allnodes_mcast(&routing_set_addr);
  simple_udp_register(&routing_set_connection, ROUTING_SET_PORT,
      NULL, ROUTING_SET_PORT,
      udp_received_routing_set);
#endif /* ORPL_CONF_DOWNWARDS_ROUTES */
}

// #endif /* ORPL_ENABLED */
