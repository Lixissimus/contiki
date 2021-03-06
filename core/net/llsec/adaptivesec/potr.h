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

#ifndef POTR_H_
#define POTR_H_

#include "contiki.h"
#include "net/mac/framer.h"
#include "net/linkaddr.h"
#include "net/llsec/llsec802154.h"
#include "net/mac/contikimac/ilos.h"
#include "net/llsec/anti-replay.h"

#ifdef POTR_CONF_ENABLED
#define POTR_ENABLED POTR_CONF_ENABLED
#else /* POTR_CONF_ENABLED */
#define POTR_ENABLED 0
#endif /* POTR_CONF_ENABLED */

#ifdef POTR_CONF_OTP_LEN
#define POTR_OTP_LEN POTR_CONF_OTP_LEN
#else /* POTR_CONF_OTP_LEN */
#define POTR_OTP_LEN 3
#endif /* POTR_CONF_OTP_LEN */

#if ILOS_ENABLED
#define POTR_FRAME_COUNTER_LEN 0
#else /* ILOS_ENABLED */
#if LLSEC802154_USES_AUX_HEADER
#define POTR_FRAME_COUNTER_LEN 4
#else /* LLSEC802154_USES_AUX_HEADER */
#define POTR_FRAME_COUNTER_LEN 1
#endif /* LLSEC802154_USES_AUX_HEADER */
#endif /* ILOS_ENABLED */

#if POTR_CONF_WITH_ANYCAST
#ifdef POTR_CONF_LL_ANYCAST_ADDR
#define POTR_LL_ANYCAST_ADDR POTR_CONF_LL_ANYCAST_ADDR
#else /* POTR_CONF_LL_ANYCAST_ADDR */
#if LINKADDR_SIZE == 8
#define POTR_LL_ANYCAST_ADDR { 0xfe , 0xfe , 0xfe , 0xfe , \
                               0xfe , 0xfe , 0xfe , 0xfe }
#else /* LINKADDR_SIZE == 8 */
#error "Potr-Anycast just works with LINKADDR_SIZE == 8"
#endif /* LINKADDR_SIZE == 8 */
#endif /* POTR_CONF_LL_ANYCAST_ADDR */
#endif /* POTR_CONF_WITH_ANYCAST */

#define POTR_HEADER_LEN (1 \
                         + LINKADDR_SIZE \
                         + POTR_FRAME_COUNTER_LEN \
                         + POTR_OTP_LEN)

enum potr_frame_type {
  POTR_FRAME_TYPE_UNICAST_DATA = 0,
  POTR_FRAME_TYPE_UNICAST_COMMAND,
  POTR_FRAME_TYPE_HELLOACK,
  POTR_FRAME_TYPE_HELLOACK_P,
  POTR_FRAME_TYPE_ACK,
  POTR_FRAME_TYPE_BROADCAST_DATA,
  POTR_FRAME_TYPE_BROADCAST_COMMAND,
  POTR_FRAME_TYPE_HELLO,
  POTR_FRAME_TYPE_ACKNOWLEDGEMENT,
  /* types for anycast support */
  POTR_FRAME_TYPE_ANYCAST_EVEN_0,
  POTR_FRAME_TYPE_ANYCAST_EVEN_1,
  POTR_FRAME_TYPE_ANYCAST_ODD_0,
  POTR_FRAME_TYPE_ANYCAST_ODD_1
};

typedef union {
  uint8_t u8[POTR_OTP_LEN];
} potr_otp_t;

#if ILOS_ENABLED
extern uint8_t potr_my_broadcast_seqno;
#endif /* ILOS_ENABLED */
extern const struct framer potr_framer;

#if !ANTI_REPLAY_WITH_SUPPRESSION
/*void potr_set_seqno(struct akes_nbr *receiver);*/
#endif /* !ANTI_REPLAY_WITH_SUPPRESSION */
int potr_received_duplicate(void);
void potr_clear_cached_otps(void);
void potr_create_special_otp(potr_otp_t *result, const linkaddr_t *src, uint8_t *challenge);
void potr_init(void);
int potr_parse_and_validate(void);
int potr_is_helloack(void);
int potr_is_ack(void);
#if POTR_CONF_WITH_ANYCAST
void potr_recreate_otp(uint8_t *p, uint8_t *group_key, uint8_t *block, uint8_t type);
void potr_create_ll_anycast_addr(linkaddr_t *addr);
int potr_is_ll_anycast_addr(const linkaddr_t *addr);
int potr_is_anycast(void);
uint8_t potr_get_last_anycast_type(void);
uint8_t potr_get_strobe_index_received(void);
rtimer_clock_t potr_calculate_strobe_time(void);
#endif /* POTR_CONF_WITH_ANYCAST */
int potr_length_of(enum potr_frame_type type);

#endif /* POTR_H_ */
