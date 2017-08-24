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
 *         id <-> mac mapping for orpl example deployment
 * \author
 *         Felix Wolff <lixissimus@gmail.com>
 */

#include "net/linkaddr.h"
#include "net/ipv6/uip-ds6.h"

#include "lladdr-id-mapping.h"

/* some hardcoded network setup */
static const uint8_t nbrs[8][8] = {
  { 1, 1, 0, 0, 1, 0, 0, 0 },
  { 1, 1, 1, 0, 0, 1, 0, 0 },
  { 0, 1, 1, 1, 0, 0, 1, 0 },
  { 0, 0, 1, 1, 0, 0, 0, 1 },
  { 1, 0, 0, 0, 1, 1, 0, 0 },
  { 0, 1, 0, 0, 1, 1, 1, 0 },
  { 0, 0, 1, 0, 0, 1, 1, 1 },
  { 0, 0, 0, 1, 0, 0, 1, 1 }
};

struct id_linkaddr {
  uint16_t id;
  linkaddr_t addr;
};

static const struct id_linkaddr id_linkaddr_list[] = {
  /* wall network */
#if 1
  { 1, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0x2b}} },
  { 2, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0x29}} },
  { 3, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x52,0xe8}} },
  { 4, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x52,0xf1}} },
  { 5, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0xb8}} },
  { 6, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0x2f}} },
  { 7, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x54,0x03}} },
  { 8, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0x73}} }
#elif 0
  /* desk test motes uni*/
  /* with tape */
  { 1, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0x32}} },
  /* without tape */
  { 2, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0xc1}} }
#else
  /* home */
  { 1, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0xd9}} },
  { 2, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0x66}} }
#endif
};

static const uint16_t ip_prefix[] = { 0xfe80, 0, 0, 0, 0x212, 0x4b00, 0x430, 0 };
/*---------------------------------------------------------------------------*/
uint16_t
lladdr_id_mapping_own_id(void)
{
  static uint16_t own_id = 0;
  if(own_id) {
    return own_id;
  }
  own_id = lladdr_id_mapping_id_from_ll(&linkaddr_node_addr);
  return own_id;
}
/*---------------------------------------------------------------------------*/
uint16_t
lladdr_id_mapping_id_from_ll(const linkaddr_t *addr)
{
  uint16_t i;
  for(i = 0; i < (sizeof(id_linkaddr_list) / sizeof(struct id_linkaddr)); ++i) {
    if(memcmp(addr, &id_linkaddr_list[i].addr, LINKADDR_SIZE) == 0) {
      return id_linkaddr_list[i].id;
    }
  }
  return 0xffff;
}
/*---------------------------------------------------------------------------*/
int
lladdr_id_mapping_ll_from_id(const uint16_t id, linkaddr_t *addr)
{
  uint16_t i;
  for(i = 0; i < (sizeof(id_linkaddr_list) / sizeof(struct id_linkaddr)); ++i) {
    if(id_linkaddr_list[i].id == id) {
      memcpy(addr, &id_linkaddr_list[i].addr, LINKADDR_SIZE);
      return 1;
    }
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
uint16_t
lladdr_id_mapping_id_from_ipv6(const uip_ipaddr_t *ip_addr)
{
  uint16_t i;
  for(i = 0; i < (sizeof(id_linkaddr_list) / sizeof(struct id_linkaddr)); ++i) {
    if( id_linkaddr_list[i].addr.u8[LINKADDR_SIZE-2] == ip_addr->u8[14] && 
        id_linkaddr_list[i].addr.u8[LINKADDR_SIZE-1] == ip_addr->u8[15])
    {
      return id_linkaddr_list[i].id;
    }
  }
  return 0xffff;
}
/*---------------------------------------------------------------------------*/
int
lladdr_id_mapping_ipv6_from_id(const uint16_t id, uip_ipaddr_t *ipaddr)
{
  uint16_t i;
  for(i = 0; i < (sizeof(id_linkaddr_list) / sizeof(struct id_linkaddr)); ++i) {
    if(id_linkaddr_list[i].id == id) {
      uip_ip6addr(ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);
      uip_ds6_set_addr_iid(ipaddr, &uip_lladdr);
      uip_ds6_set_addr_iid(ipaddr, (uip_lladdr_t*) &id_linkaddr_list[i].addr);
      return 1;
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
int
lladdr_id_mapping_ll_from_ipv6(const uip_ipaddr_t *ipaddr, linkaddr_t *lladdr)
{
  uint16_t i;
  for(i = 0; i < (sizeof(id_linkaddr_list) / sizeof(struct id_linkaddr)); ++i) {
    if( id_linkaddr_list[i].addr.u8[LINKADDR_SIZE-2] == ipaddr->u8[14] && 
        id_linkaddr_list[i].addr.u8[LINKADDR_SIZE-1] == ipaddr->u8[15])
    {
      memcpy(lladdr, &id_linkaddr_list[i].addr, LINKADDR_SIZE);
      return 1;
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
int
lladdr_id_mapping_are_nbrs(const uint16_t id1, const uint16_t id2)
{
  return nbrs[id1-1][id2-1];
}
