/*
 * Copyright (c) 2016, Hasso-Plattner-Institut.
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
 *         Practical On-the-fly Rejection (POTR).
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "net/llsec/adaptivesec/potr.h"
#include "net/llsec/adaptivesec/adaptivesec.h"
#include "net/llsec/adaptivesec/akes-nbr.h"
#include "net/llsec/adaptivesec/akes.h"
#include "net/llsec/llsec802154.h"
#include "net/llsec/anti-replay.h"
#include "lib/aes-128.h"
#include "net/packetbuf.h"
#include "net/netstack.h"
#include "net/mac/contikimac/contikimac-framer.h"
#include "net/llsec/ccm-star-packetbuf.h"
#include "net/mac/contikimac/secrdc.h"
#include <string.h>
#include <inttypes.h>

#ifdef POTR_CONF_KEY
#define POTR_KEY POTR_CONF_KEY
#else /* POTR_CONF_KEY */
#define POTR_KEY { 0x00 , 0x01 , 0x02 , 0x03 , \
                   0x04 , 0x05 , 0x06 , 0x07 , \
                   0x08 , 0x09 , 0x0A , 0x0B , \
                   0x0C , 0x0D , 0x0E , 0x0F }
#endif /* POTR_CONF_KEY */

#define MAX_CACHED_OTPS 8

#ifdef POTR_CONF_WITH_CONTIKIMAC_FRAMER
#define WITH_CONTIKIMAC_FRAMER POTR_CONF_WITH_CONTIKIMAC_FRAMER
#else /* POTR_CONF_WITH_CONTIKIMAC_FRAMER */
#define WITH_CONTIKIMAC_FRAMER 0
#endif /* POTR_CONF_WITH_CONTIKIMAC_FRAMER */

#define PAN_ID_LEN (2)

#if ILOS_ENABLED
#ifdef POTR_CONF_WITH_PAN_IDS
#define WITH_PAN_IDS POTR_CONF_WITH_PAN_IDS
#else /* POTR_CONF_WITH_PAN_IDS */
#define WITH_PAN_IDS 0
#endif /* POTR_CONF_WITH_PAN_IDS */
#else /* ILOS_ENABLED */
#define WITH_PAN_IDS 0
#endif /* ILOS_ENABLED */

#define HELLO_LEN (POTR_HEADER_LEN \
    + (WITH_PAN_IDS ? PAN_ID_LEN - POTR_OTP_LEN : 0) \
    + (WITH_CONTIKIMAC_FRAMER ? CONTIKIMAC_FRAMER_HEADER_LEN : 0) \
    + 1 \
    + AKES_NBR_CHALLENGE_LEN \
    + (AKES_NBR_WITH_PAIRWISE_KEYS ? 0 : ADAPTIVESEC_BROADCAST_MIC_LEN))

#if WITH_CONTIKIMAC_FRAMER && (HELLO_LEN < CONTIKIMAC_FRAMER_SHORTEST_PACKET_SIZE)
#undef HELLO_LEN
#define HELLO_LEN CONTIKIMAC_FRAMER_SHORTEST_PACKET_SIZE
#endif /* WITH_CONTIKIMAC_FRAMER && (HELLO_LEN < CONTIKIMAC_FRAMER_SHORTEST_PACKET_SIZE) */

#include <stdio.h>

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else /* DEBUG */
#define PRINTF(...)
#endif /* DEBUG */

#if NETWORK_HARDCODED
#include "lib/lladdr-id-mapping.h"
#endif /* NETWORK_HARDCODED */

#if POTR_ENABLED
static void read_otp(void);

static uint8_t potr_key[16] = POTR_KEY;
static potr_otp_t cached_otps[MAX_CACHED_OTPS];
static uint8_t cached_otps_index;
#if ILOS_ENABLED
static linkaddr_t sender_of_last_accepted_broadcast;
static ilos_wake_up_counter_t wake_up_counter_at_last_accepted_broadcast;
#endif /* ILOS_ENABLED */
#if POTR_CONF_WITH_ANYCAST
static uint8_t anycast_ll_addr[LINKADDR_SIZE] = POTR_LL_ANYCAST_ADDR;
static uint8_t last_anycast_type;
static uint8_t strobe_index_received;
#endif /* POTR_CONF_WITH_ANYCAST */

/*---------------------------------------------------------------------------*/
void
potr_set_seqno(struct akes_nbr *receiver)
{
  packetbuf_set_attr(PACKETBUF_ATTR_MAC_SEQNO, ++receiver->my_unicast_seqno);
}
/*---------------------------------------------------------------------------*/
#if POTR_CONF_WITH_ANYCAST
void
potr_create_ll_anycast_addr(linkaddr_t *addr)
{
  memcpy(addr, anycast_ll_addr, LINKADDR_SIZE);
}
/*---------------------------------------------------------------------------*/
int
potr_is_ll_anycast_addr(const linkaddr_t *addr)
{
  return (memcmp(addr, anycast_ll_addr, LINKADDR_SIZE) == 0);
}
/*---------------------------------------------------------------------------*/
uint8_t
potr_get_last_anycast_type(void)
{
  return last_anycast_type;
}
/*---------------------------------------------------------------------------*/
uint8_t
potr_get_strobe_index_received(void)
{
  return strobe_index_received;
}
/*---------------------------------------------------------------------------*/
rtimer_clock_t
potr_calculate_strobe_time(void)
{
  /* data_len + shr_len + framelen_len (in bits) / 250kb/s 
   * converted to micro seconds, converted to rtimer ticks,
   * 4 instead of 1000/250
   */
  return US_TO_RTIMERTICKS((packetbuf_datalen() + 5 + 1) * 8 * 4);
}
#endif /* POTR_CONF_WITH_ANYCAST */
/*---------------------------------------------------------------------------*/
int
potr_received_duplicate(void)
{
  struct akes_nbr_entry *entry;
  uint8_t type;
  uint8_t seqno;

  entry = akes_nbr_get_sender_entry();
  if(!entry || !entry->permanent) {
    return 0;
  }

  type = ((uint8_t *)packetbuf_hdrptr())[0];
  seqno = packetbuf_attr(PACKETBUF_ATTR_MAC_SEQNO);
  switch(type) {
  case POTR_FRAME_TYPE_UNICAST_DATA:
  case POTR_FRAME_TYPE_UNICAST_COMMAND:
    if(entry->permanent->his_unicast_seqno == seqno) {
      return 1;
    }
    entry->permanent->his_unicast_seqno = seqno;
    break;
  default:
    break;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
#if !ILOS_ENABLED
static void
write_frame_counter(uint8_t *p)
{
  frame802154_frame_counter_t frame_counter;

  frame_counter.u16[0] = packetbuf_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_0_1);
  frame_counter.u16[1] = packetbuf_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_2_3);
  memcpy(p, frame_counter.u8, 4);
}
#endif /* !ILOS_ENABLED */
/*---------------------------------------------------------------------------*/
#if SECRDC_WITH_SECURE_PHASE_LOCK
int
potr_has_strobe_index(enum potr_frame_type type)
{
  switch(type) {
    case POTR_FRAME_TYPE_UNICAST_DATA:
    case POTR_FRAME_TYPE_UNICAST_COMMAND:
    case POTR_FRAME_TYPE_HELLOACK:
    case POTR_FRAME_TYPE_HELLOACK_P:
    case POTR_FRAME_TYPE_ACK:
    case POTR_FRAME_TYPE_ANYCAST_EVEN_0:
    case POTR_FRAME_TYPE_ANYCAST_EVEN_1:
    case POTR_FRAME_TYPE_ANYCAST_ODD_0:
    case POTR_FRAME_TYPE_ANYCAST_ODD_1:
      return 1;
    default:
      return 0;
  }
}
#endif /* SECRDC_WITH_SECURE_PHASE_LOCK */
/*---------------------------------------------------------------------------*/
static int
has_seqno(enum potr_frame_type type)
{
  switch(type) {
  case POTR_FRAME_TYPE_UNICAST_DATA:
  case POTR_FRAME_TYPE_UNICAST_COMMAND:
    return 1;
  default:
    return 0;
  }
}
/*---------------------------------------------------------------------------*/
int
potr_is_helloack(void)
{
  if(packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE) != FRAME802154_CMDFRAME) {
    return 0;
  }

  switch(adaptivesec_get_cmd_id()) {
  case POTR_FRAME_TYPE_HELLOACK:
  case POTR_FRAME_TYPE_HELLOACK_P:
    return 1;
  default:
    return 0;
  }
}
/*---------------------------------------------------------------------------*/
int
potr_is_ack(void)
{
  if(packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE) != FRAME802154_CMDFRAME) {
    return 0;
  }
  return adaptivesec_get_cmd_id() == POTR_FRAME_TYPE_ACK;
}
/*---------------------------------------------------------------------------*/
#if POTR_CONF_WITH_ANYCAST
int
potr_is_anycast(void)
{
  if(packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE) != FRAME802154_DATAFRAME) {
    return 0;
  }

  switch(adaptivesec_get_cmd_id()) {
  case POTR_FRAME_TYPE_ANYCAST_EVEN_0:
  case POTR_FRAME_TYPE_ANYCAST_EVEN_1:
  case POTR_FRAME_TYPE_ANYCAST_ODD_0:
  case POTR_FRAME_TYPE_ANYCAST_ODD_1:
    return 1;
  default:
    return 0;
  }
}
#endif /* POTR_CONF_WITH_ANYCAST */
/*---------------------------------------------------------------------------*/

int
potr_length_of(enum potr_frame_type type)
{
  return POTR_HEADER_LEN
#if WITH_PAN_IDS
      - ((type == POTR_FRAME_TYPE_HELLO) ? POTR_OTP_LEN - PAN_ID_LEN : 0)
#endif /* WITH_PAN_IDS */
#if SECRDC_WITH_SECURE_PHASE_LOCK
      + (potr_has_strobe_index(type) ? 1 : 0)
#endif /* SECRDC_WITH_SECURE_PHASE_LOCK */
      + (has_seqno(type) ? 1 : 0);
}
/*---------------------------------------------------------------------------*/
static int
length(void)
{
  if(packetbuf_holds_broadcast()) {
    return potr_length_of(POTR_FRAME_TYPE_BROADCAST_DATA);
  }
#if POTR_CONF_WITH_ANYCAST
  if(packetbuf_holds_anycast()) {
    return potr_length_of(POTR_FRAME_TYPE_ANYCAST_EVEN_0);
  }
#endif /* POTR_CONF_WITH_ANYCAST */  
  return potr_length_of(POTR_FRAME_TYPE_UNICAST_DATA);
}
/*---------------------------------------------------------------------------*/
void
potr_create_special_otp(potr_otp_t *result, const linkaddr_t *src, uint8_t *challenge)
{
  uint8_t block[AES_128_BLOCK_SIZE];

  memcpy(block, src->u8, LINKADDR_SIZE);
  memcpy(block + LINKADDR_SIZE, challenge, AKES_NBR_CHALLENGE_LEN);
  memset(block + LINKADDR_SIZE + AKES_NBR_CHALLENGE_LEN,
      0,
      AES_128_BLOCK_SIZE - LINKADDR_SIZE - AKES_NBR_CHALLENGE_LEN);

  AES_128_GET_LOCK();
  AES_128.set_key(potr_key);
  AES_128.encrypt(block);
  AES_128_RELEASE_LOCK();
  memcpy(result->u8, block, POTR_OTP_LEN);
}
/*---------------------------------------------------------------------------*/
static void
do_create_normal_otp(uint8_t *p, uint8_t *group_key, uint8_t *block)
{
  uint8_t key[AES_128_KEY_LENGTH];
  uint8_t i;

  for(i = 0; i < AES_128_KEY_LENGTH; i++) {
    key[i] = group_key[i] ^ potr_key[i];
  }
  AES_128_GET_LOCK();
  AES_128.set_key(key);
  AES_128.encrypt(block);
  AES_128_RELEASE_LOCK();
  memcpy(p, block, POTR_OTP_LEN);
}
/*---------------------------------------------------------------------------*/
#if ILOS_ENABLED
static void
create_normal_otp(uint8_t *p, int forward, void *entry)
{
  uint8_t block[AES_128_BLOCK_SIZE];
  uint8_t *group_key;

  memset(block,
      0,
      AES_128_BLOCK_SIZE);
  ccm_star_packetbuf_set_nonce(block,
      forward,
      (entry && ((struct akes_nbr_entry *)entry)->permanent)
          ? &((struct akes_nbr_entry *)entry)->permanent->phase
          : NULL);
  /* because we dont use strobe index in OTP */
  block[8] = 0x00;

  if(forward) {
    if(packetbuf_holds_broadcast()) {
      group_key = adaptivesec_group_key;
    } else {
      group_key = ((struct akes_nbr_entry *)entry)->permanent->group_key;
    }
  } else {
    if(packetbuf_holds_broadcast()) {
      group_key = ((struct akes_nbr_entry *)entry)->permanent->group_key;
    } else {
      group_key = adaptivesec_group_key;
    }
  }

  do_create_normal_otp(p, group_key, block);
}
#else /* ILOS_ENABLED */
static void
create_normal_otp(uint8_t *p, uint8_t *group_key)
{
  uint8_t block[AES_128_BLOCK_SIZE];

  if(packetbuf_holds_broadcast()) {
    memset(block, 0xFF, LINKADDR_SIZE);
  } else {
    memcpy(block, packetbuf_addr(PACKETBUF_ADDR_RECEIVER)->u8, LINKADDR_SIZE);
  }
  write_frame_counter(block + LINKADDR_SIZE);
  memset(block + LINKADDR_SIZE + 4, 0, AES_128_BLOCK_SIZE - LINKADDR_SIZE - 4);

  do_create_normal_otp(p, group_key, block);
}
#endif /* ILOS_ENABLED */
/*---------------------------------------------------------------------------*/
#if POTR_CONF_WITH_ANYCAST
void
potr_recreate_otp(uint8_t *p, uint8_t *group_key, uint8_t *nonce, uint8_t type) {
  uint8_t block[AES_128_BLOCK_SIZE];

  memset(block, 0, AES_128_BLOCK_SIZE);
  memcpy(block, nonce, 13);
  block[13] = type;

  do_create_normal_otp(p, group_key, block);
}
/*---------------------------------------------------------------------------*/
static void
create_anycast_otp(uint8_t *p, int forward, void *entry, enum potr_frame_type type)
{
  uint8_t block[AES_128_BLOCK_SIZE];
  uint8_t *group_key;

  memset(block, 0, AES_128_BLOCK_SIZE);
  ccm_star_packetbuf_set_nonce(
      block,
      forward,
      (entry && ((struct akes_nbr_entry *)entry)->permanent) ?
          &((struct akes_nbr_entry *)entry)->permanent->phase :
          NULL);

  /* keep strobe index as part of nonce */

  /* add frame type to otp */
  block[13] = type;

  if (forward) {
    /* for sending anycast, we use our own group key */
    group_key = adaptivesec_group_key;
  } else {
    /* when receiving anycast, we use the senders group key */
    group_key = ((struct akes_nbr_entry *)entry)->permanent->group_key;
  }

  do_create_normal_otp(p, group_key, block);
}
#endif /* POTR_CONF_WITH_ANYCAST */
/*---------------------------------------------------------------------------*/
static int
create(void)
{
  enum potr_frame_type type;
  uint8_t cmd_id;
  uint8_t *p;
  struct akes_nbr_entry *entry;

  /* Frame Type */
  switch(packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE)) {
  case FRAME802154_DATAFRAME:
    if(packetbuf_holds_broadcast()) {
      type = POTR_FRAME_TYPE_BROADCAST_DATA;
#if POTR_CONF_WITH_ANYCAST
    } else if(packetbuf_holds_anycast()) {
      type = secrdc_specialize_anycast_frame_type();
      last_anycast_type = type;
#endif /* POTR_CONF_WITH_ANYCAST */
    } else {
      type = POTR_FRAME_TYPE_UNICAST_DATA;
    }
    break;
  case FRAME802154_CMDFRAME:
    cmd_id = adaptivesec_get_cmd_id();
    switch(cmd_id) {
    case POTR_FRAME_TYPE_HELLO:
    case POTR_FRAME_TYPE_HELLOACK:
    case POTR_FRAME_TYPE_HELLOACK_P:
    case POTR_FRAME_TYPE_ACK:
      type = cmd_id;
      break;
    default:
      type = packetbuf_holds_broadcast() ? POTR_FRAME_TYPE_BROADCAST_COMMAND : POTR_FRAME_TYPE_UNICAST_COMMAND;
    }
    break;
  default:
    PRINTF("potr: unknown frame type\n");
    return FRAMER_FAILED;
  }
  if(!packetbuf_hdralloc(potr_length_of(type))) {
    PRINTF("potr: packetbuf_hdralloc failed\n");
    return FRAMER_FAILED;
  }
  p = packetbuf_hdrptr();
  /* there is no neighbor for anycasts */
  /* Todo: write Macro to test for all anycast types */
  if (type < POTR_FRAME_TYPE_ANYCAST_EVEN_0) {
    entry = akes_nbr_get_receiver_entry();
  } else {
    /* Make sure entry is initialized */
    entry = NULL;
  }
  p[0] = type;
  p += 1;

  /* Source Address */
  memcpy(p, linkaddr_node_addr.u8, LINKADDR_SIZE);
  p += LINKADDR_SIZE;

#if !ILOS_ENABLED
  /* Frame Counter */
#if LLSEC802154_USES_AUX_HEADER
  write_frame_counter(p);
  p += 4;
#else /* LLSEC802154_USES_AUX_HEADER */
  p[0] = packetbuf_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_0_1) & 0xFF;
  p += 1;
#endif /* LLSEC802154_USES_AUX_HEADER */
#endif /* !ILOS_ENABLED */

  /* OTP */
  switch(type) {
  case POTR_FRAME_TYPE_HELLOACK:
  case POTR_FRAME_TYPE_HELLOACK_P:
    if(!entry || !entry->tentative) {
      PRINTF("potr: Could not create HELLOACK OTP\n");
      return FRAMER_FAILED;
    }
    memcpy(p, entry->tentative->meta->otp.u8, POTR_OTP_LEN);
    /* create ACK OTP */
    potr_create_special_otp(&entry->tentative->meta->otp,
        packetbuf_addr(PACKETBUF_ADDR_RECEIVER),
        ((uint8_t *) packetbuf_dataptr()) + ILOS_WAKE_UP_COUNTER_LEN + 1);
    break;
  case POTR_FRAME_TYPE_ACK:
    if(!entry || !entry->tentative) {
      PRINTF("potr: Could not create ACK OTP\n");
      return FRAMER_FAILED;
    }
    memcpy(p, entry->tentative->meta->otp.u8, POTR_OTP_LEN);
    break;
#if POTR_CONF_WITH_ANYCAST
  case POTR_FRAME_TYPE_ANYCAST_EVEN_0:
  case POTR_FRAME_TYPE_ANYCAST_EVEN_1:
  case POTR_FRAME_TYPE_ANYCAST_ODD_0:
  case POTR_FRAME_TYPE_ANYCAST_ODD_1:
    create_anycast_otp(p, 1, NULL, type);
#if DEBUG
    rtimer_clock_t planned_start = secrdc_get_next_strobe_start();
    PRINTF("wakeup counter at %u: %u (%u)\n", planned_start, secrdc_get_wake_up_counter(planned_start).u32, type);
#endif /* DEBUG */
    break;
#endif /* POTR_CONF_WITH_ANYCAST */
#if WITH_PAN_IDS
  case POTR_FRAME_TYPE_HELLO:
    p[0] = IEEE802154_PANID & 0xff;
    p[1] = (IEEE802154_PANID >> 8) & 0xff;
    p += PAN_ID_LEN - POTR_OTP_LEN;
    break;
#endif /* WITH_PAN_IDS */
  default:
#if ILOS_ENABLED
    create_normal_otp(p, 1, entry);
#else /* ILOS_ENABLED */
    create_normal_otp(p, adaptivesec_group_key);
#endif /* ILOS_ENABLED */
    break;
  }
  p += POTR_OTP_LEN;

#if SECRDC_WITH_SECURE_PHASE_LOCK
  if(potr_has_strobe_index(type)) {
    p[0] = 0;
    p += 1;
  }
#endif /* SECRDC_WITH_SECURE_PHASE_LOCK */

  if(has_seqno(type)) {
    p[0] = (uint8_t) packetbuf_attr(PACKETBUF_ATTR_MAC_SEQNO);
  }

  return potr_length_of(type);
}
/*---------------------------------------------------------------------------*/
void
potr_clear_cached_otps(void)
{
  cached_otps_index = 0;
}
/*---------------------------------------------------------------------------*/
int
potr_parse_and_validate(void)
{
  uint8_t *p;
  enum potr_frame_type type;
  linkaddr_t addr;
  struct akes_nbr_entry *entry;
  potr_otp_t otp;
  uint8_t i;

  p = packetbuf_hdrptr();
  type = p[0];

  /* Frame Length */
#if WITH_CONTIKIMAC_FRAMER
  if(packetbuf_datalen() < CONTIKIMAC_FRAMER_SHORTEST_PACKET_SIZE) {
#else /* WITH_CONTIKIMAC_FRAMER */
  if(packetbuf_datalen() <= potr_length_of(type)) {
#endif /* WITH_CONTIKIMAC_FRAMER */
    PRINTF("potr: invalid length\n");
    return FRAMER_FAILED;
  }

  /* Frame Type */
  switch(type) {
  case POTR_FRAME_TYPE_UNICAST_DATA:
  case POTR_FRAME_TYPE_UNICAST_COMMAND:
  case POTR_FRAME_TYPE_HELLOACK:
  case POTR_FRAME_TYPE_HELLOACK_P:
  case POTR_FRAME_TYPE_ACK:
    packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &linkaddr_node_addr);
    break;
  case POTR_FRAME_TYPE_BROADCAST_DATA:
  case POTR_FRAME_TYPE_BROADCAST_COMMAND:
  case POTR_FRAME_TYPE_HELLO:
    packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &linkaddr_null);
    break;
#if POTR_CONF_WITH_ANYCAST
  case POTR_FRAME_TYPE_ANYCAST_EVEN_0:
  case POTR_FRAME_TYPE_ANYCAST_EVEN_1:
  case POTR_FRAME_TYPE_ANYCAST_ODD_0:
  case POTR_FRAME_TYPE_ANYCAST_ODD_1:
    packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &linkaddr_anycast);
    last_anycast_type = type;
    break;
#endif /* POTR_CONF_WITH_ANYCAST */
  default:
    PRINTF("potr: unknown frame type %02x\n", type);
    return FRAMER_FAILED;
  }

  switch(type) {
  case POTR_FRAME_TYPE_BROADCAST_DATA:
  case POTR_FRAME_TYPE_UNICAST_DATA:
  case POTR_FRAME_TYPE_ANYCAST_EVEN_0:
  case POTR_FRAME_TYPE_ANYCAST_EVEN_1:
  case POTR_FRAME_TYPE_ANYCAST_ODD_0:
  case POTR_FRAME_TYPE_ANYCAST_ODD_1:
    packetbuf_set_attr(PACKETBUF_ATTR_FRAME_TYPE, FRAME802154_DATAFRAME);
    break;
  default:
    packetbuf_set_attr(PACKETBUF_ATTR_FRAME_TYPE, FRAME802154_CMDFRAME);
    break;
  }
  p += 1;

  /* Source Address */
  memcpy(addr.u8, p, LINKADDR_SIZE);
#if NETWORK_HARDCODED
  uint16_t own_id = lladdr_id_mapping_own_id();
  uint16_t src_id = lladdr_id_mapping_id_from_ll(&addr);
  if(!lladdr_id_mapping_are_nbrs(own_id, src_id)) {
    PRINTF("reject because not a neighbor (%" PRIu16 " - %" PRIu16 ")\n",
        own_id, src_id);
    return FRAMER_FAILED;
  }
#endif /* NETWORK_HARDCODED */
  packetbuf_set_addr(PACKETBUF_ADDR_SENDER, &addr);
  entry = akes_nbr_get_sender_entry();
#if ILOS_ENABLED
  switch(type) {
    case POTR_FRAME_TYPE_BROADCAST_DATA:
    case POTR_FRAME_TYPE_BROADCAST_COMMAND:
    case POTR_FRAME_TYPE_HELLO:
      if((secrdc_get_wake_up_counter(secrdc_get_last_wake_up_time()).u32
              == (wake_up_counter_at_last_accepted_broadcast.u32 + 1))
          && linkaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_SENDER),
              &sender_of_last_accepted_broadcast)) {
        PRINTF("potr: Just accepted a broadcast frame from this sender already\n");
        return FRAMER_FAILED;
      }
      break;
    default:
      break;
  }
#endif /* ILOS_ENABLED */
  p += LINKADDR_SIZE;

#if !ILOS_ENABLED
  /* Frame Counter */
#if LLSEC802154_USES_AUX_HEADER
  anti_replay_parse_counter(p);
  p += 4;
#else /* LLSEC802154_USES_AUX_HEADER */
  packetbuf_set_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_0_1, p[0]);
  p += 1;
#endif /* LLSEC802154_USES_AUX_HEADER */
#if ANTI_REPLAY_WITH_SUPPRESSION
  if(entry && entry->permanent) {
    anti_replay_restore_counter(&entry->permanent->anti_replay_info);
  }
#endif /* ANTI_REPLAY_WITH_SUPPRESSION */
#endif /* !ILOS_ENABLED */

  /* OTP */
  switch(type) {
  case POTR_FRAME_TYPE_HELLOACK:
  case POTR_FRAME_TYPE_HELLOACK_P:
    if(cached_otps_index >= MAX_CACHED_OTPS) {
      PRINTF("potr: Too many HELLOACK OTPs cached\n");
      return FRAMER_FAILED;
    }

    if(!akes_is_acceptable_helloack()) {
      PRINTF("potr: Unacceptable HELLOACK\n");
      return FRAMER_FAILED;
    }

    /* create HELLOACK OTP */
    potr_create_special_otp(&cached_otps[cached_otps_index],
        packetbuf_addr(PACKETBUF_ADDR_SENDER),
        akes_hello_challenge);
    read_otp();
    if(memcmp(cached_otps[cached_otps_index].u8, p, POTR_OTP_LEN)) {
      PRINTF("potr: Invalid HELLOACK OTP\n");
      return FRAMER_FAILED;
    }

    for(i = 0; i < cached_otps_index; i++) {
      if(!memcmp(cached_otps[i].u8, cached_otps[cached_otps_index].u8, POTR_OTP_LEN)) {
        PRINTF("potr: Replayed HELLOACK OTP\n");
        return FRAMER_FAILED;
      }
    }

    cached_otps_index++;
    break;
  case POTR_FRAME_TYPE_ACK:
    read_otp();
    if(!akes_is_acceptable_ack(entry)) {
      PRINTF("potr: Unacceptable ACK\n");
      return FRAMER_FAILED;
    } else if(memcmp(entry->tentative->meta->otp.u8, p, POTR_OTP_LEN)) {
      PRINTF("potr: Invalid ACK OTP\n");
      return FRAMER_FAILED;
    } else {
      /*
       * Replay is prevented by either turning the sender
       * into a permanent neighbor or deleting the sender.
       */
    }
    break;
#if POTR_CONF_WITH_ANYCAST
  case POTR_FRAME_TYPE_ANYCAST_EVEN_0:
  case POTR_FRAME_TYPE_ANYCAST_EVEN_1:
  case POTR_FRAME_TYPE_ANYCAST_ODD_0:
  case POTR_FRAME_TYPE_ANYCAST_ODD_1:
    if (!entry || !entry->permanent) {
      PRINTF("potr: Anycast sender is not permanent\n");
      return FRAMER_FAILED;
    }
    
    read_otp();
    /* read strobe index */
    NETSTACK_RADIO_ASYNC.read_payload(1);
    strobe_index_received = *(p + POTR_OTP_LEN);
    create_anycast_otp(otp.u8, 0, entry, type);
    if(memcmp(otp.u8, p, POTR_OTP_LEN)) {
      PRINTF("potr: Invalid anycast OTP %u (%u)\n", restore_anycast_wakeup_counter(NULL).u32, type);
      return FRAMER_FAILED;
    }
    break;
#endif /* POTR_CONF_WITH_ANYCAST */
  case POTR_FRAME_TYPE_HELLO:
    if(packetbuf_totlen() != HELLO_LEN) {
      PRINTF("potr: Rejected HELLO\n");
      return FRAMER_FAILED;
    }
    if(!akes_is_acceptable_hello(entry)) {
      PRINTF("potr: Inacceptable HELLO\n");
      return FRAMER_FAILED;
    }
#if WITH_PAN_IDS
    /* check PAN-ID instead of OTP */
    NETSTACK_RADIO_ASYNC.read_payload(PAN_ID_LEN);
    if((p[0] + (p[1] << 8)) != IEEE802154_PANID) {
      PRINTF("potr: HELLO for other PAN\n");
      return FRAMER_FAILED;
    }
    /* prevent replay */
    linkaddr_copy(&sender_of_last_accepted_broadcast,
        packetbuf_addr(PACKETBUF_ADDR_SENDER));
    wake_up_counter_at_last_accepted_broadcast =
        secrdc_get_wake_up_counter(secrdc_get_last_wake_up_time());
    p += PAN_ID_LEN;
    break;
#else /* WITH_PAN_IDS */
    /* intentionally no break; */
#endif /* WITH_PAN_IDS */
  default:
    if(!entry || !entry->permanent) {
      if(type == POTR_FRAME_TYPE_HELLO) {
        break;
      }
      PRINTF("potr: Sender is not permanent\n");
      return FRAMER_FAILED;
    }

#if ILOS_ENABLED
    create_normal_otp(otp.u8, 0, entry);
#else /* ILOS_ENABLED */
    create_normal_otp(otp.u8, entry->permanent->group_key);
#endif /* ILOS_ENABLED */
    read_otp();

    if(memcmp(otp.u8, p, POTR_OTP_LEN)) {
      if(type == POTR_FRAME_TYPE_HELLO) {
        break;
      }
      PRINTF("potr: Invalid OTP\n");
      return FRAMER_FAILED;
    }
#if ILOS_ENABLED
    else {
      switch(type) {
      case POTR_FRAME_TYPE_BROADCAST_DATA:
      case POTR_FRAME_TYPE_BROADCAST_COMMAND:
      case POTR_FRAME_TYPE_HELLO:
        linkaddr_copy(&sender_of_last_accepted_broadcast,
            packetbuf_addr(PACKETBUF_ADDR_SENDER));
        wake_up_counter_at_last_accepted_broadcast =
            secrdc_get_wake_up_counter(secrdc_get_last_wake_up_time());
        break;
      default:
        break;
      }
    }
#else /* ILOS_ENABLED */
    if(anti_replay_was_replayed(&entry->permanent->anti_replay_info)) {
      PRINTF("potr: Replayed OTP\n");
      return FRAMER_FAILED;
    }
#endif /* ILOS_ENABLED */
    break;
  }
  p += POTR_OTP_LEN;

  return potr_length_of(type);
}
/*---------------------------------------------------------------------------*/
static void
read_otp(void)
{
  NETSTACK_RADIO_ASYNC.read_payload(POTR_OTP_LEN);
}
/*---------------------------------------------------------------------------*/
static int
parse(void)
{
  uint8_t *hdrptr;
  enum potr_frame_type type;
  int len;

  hdrptr = packetbuf_hdrptr();
  type = hdrptr[0];
  len = potr_length_of(type);

  if(has_seqno(type)) {
    packetbuf_set_attr(PACKETBUF_ATTR_MAC_SEQNO, hdrptr[len - 1]);
  }

  if(!packetbuf_hdrreduce(len)) {
    PRINTF("potr: packetbuf_hdrreduce failed\n");
    return FRAMER_FAILED;
  }

  return len;
}
/*---------------------------------------------------------------------------*/
const struct framer potr_framer = {
  length,
  create,
  parse
};
/*---------------------------------------------------------------------------*/
#endif /* POTR_ENABLED */
