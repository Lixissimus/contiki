/*
 * Copyright (c) 2013, Hasso-Plattner-Institut.
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
 *         CCM* convenience functions for LLSEC use
 * \author
 *         Justin King-Lacroix <justin.kinglacroix@gmail.com>
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "llsec/ccm-star-packetbuf.h"
#include "net/llsec/anti-replay.h"
#include "net/linkaddr.h"
#include "net/packetbuf.h"
#include "net/llsec/llsec802154.h"
#include "net/mac/contikimac/secrdc.h"
#include "net/llsec/adaptivesec/potr.h"
#include <string.h>

#include <stdio.h>

/*---------------------------------------------------------------------------*/
static const uint8_t *
get_extended_address(const linkaddr_t *addr)
#if LINKADDR_SIZE == 1
{
  /* workaround for short addresses: derive EUI64 as in RFC 6282 */
  static linkaddr_extended_t template = { { 0x00 , 0x00 , 0x00 ,
                                            0xFF , 0xFE , 0x00 , 0x00 , 0x00 } };

  template.u8[7] = addr->u8[0];

  return template.u8;
}
#elif LINKADDR_SIZE == 2
{
  /* workaround for short addresses: derive EUI64 as in RFC 6282 */
  static linkaddr_extended_t template = { { 0x00 , 0x00 , 0x00 ,
                                            0xFF , 0xFE , 0x00 , 0x00 , 0x00 } };
  
  template.u16[3] = LLSEC802154_HTONS(addr->u16);
  
  return template.u8;
}
#else /* LINKADDR_SIZE == 2 */
{
  return addr->u8;
}
#endif /* LINKADDR_SIZE == 2 */
/*---------------------------------------------------------------------------*/
#if ILOCS_ENABLED
ilocs_wake_up_counter_t
ccm_star_packetbuf_predict_wake_up_counter(struct secrdc_phase *phase)
{
  ilocs_wake_up_counter_t count;

  count.u32 = phase->his_wake_up_counter_at_t.u32
      + (rtimer_delta(phase->t, secrdc_get_next_strobe_start()) >> SECRDC_WAKEUP_INTERVAL_BITS)
      + 1;

  return count;
}
/*---------------------------------------------------------------------------*/
static ilocs_wake_up_counter_t
restore_wake_up_counter(struct secrdc_phase *phase)
{
  rtimer_clock_t diff;
  uint32_t div;
  uint32_t mod;
  ilocs_wake_up_counter_t count;

  diff = secrdc_get_last_wake_up_time() - phase->t;
  div = diff >> SECRDC_WAKEUP_INTERVAL_BITS;
  count.u32 = phase->his_wake_up_counter_at_t.u32 + div;

  if(count.u32 & 1) {
    /* odd --> we need to round */
    mod = diff & (~(SECRDC_WAKEUP_INTERVAL - 1));
    if(mod < (SECRDC_WAKEUP_INTERVAL / 2)) {
      count.u32--;
    } else  {
      count.u32++;
    }
  }

  return count;
}
#endif /* ILOCS_ENABLED */
/*---------------------------------------------------------------------------*/
#if POTR_CONF_WITH_ANYCAST
ilocs_wake_up_counter_t 
restore_anycast_wakeup_counter(struct secrdc_phase *phase)
{
  static ilocs_wake_up_counter_t count;
  /* no phase is passed, this is because we are creating an ACK */
  if(!phase) {
    return count;
  }

  const rtimer_clock_t t_rec = secrdc_get_strobe_start_time();
  const rtimer_clock_t t_strobe = potr_calculate_strobe_time() + secrdc_get_ack_window_length();
  /* time when he started first strobe */
  const uint32_t t_start = t_rec - t_strobe * potr_get_strobe_index_received();
  count.u32 = phase->his_wake_up_counter_at_t.u32 + (t_start - phase->t) / secrdc_get_wakeup_interval();
  // printf("his time: %d\n", rtimer_delta(t_start, phase->t) % secrdc_get_wakeup_interval());
  printf("calc strobe time: %d\n", t_strobe);

  switch(potr_get_last_anycast_type()) {
  case POTR_FRAME_TYPE_ANYCAST_EVEN_0:
    /* round up to even */
    count.u32 += count.u32 & 1 ? 1 : 0;
    break;
  case POTR_FRAME_TYPE_ANYCAST_EVEN_1:
    /* round down to even */
    count.u32 -= count.u32 & 1 ? 1 : 0;
    break;
  case POTR_FRAME_TYPE_ANYCAST_ODD_0:
    /* round up to odd */
    count.u32 += count.u32 & 1 ? 0 : 1;
    break;
  case POTR_FRAME_TYPE_ANYCAST_ODD_1:
    /* round down to odd */
    count.u32 -= count.u32 & 1 ? 0 : 1;
    break;

  }
  return count;
}
#endif /* POTR_CONF_WITH_ANYCAST */
/*---------------------------------------------------------------------------*/
void
ccm_star_packetbuf_set_nonce(uint8_t *nonce, int forward
#if ILOCS_ENABLED
, struct secrdc_phase *phase
#endif /* ILOCS_ENABLED */
)
{
  const linkaddr_t *source_addr;
#if SECRDC_WITH_SECURE_PHASE_LOCK
  uint8_t *hdrptr;
#if ILOCS_ENABLED
  ilocs_wake_up_counter_t count;
#endif /* ILOCS_ENABLED */
#endif /* SECRDC_WITH_SECURE_PHASE_LOCK */
  
  source_addr = forward ? &linkaddr_node_addr : packetbuf_addr(PACKETBUF_ADDR_SENDER);
  /* set ID_A of the nonce to be the source address */
  memcpy(nonce, get_extended_address(source_addr), 8);
#if ILOCS_ENABLED
  hdrptr = packetbuf_hdrptr();
  /* set the strobe index, if this frame type uses a strobe index */
  nonce[8] = potr_has_strobe_index(hdrptr[0]) ? hdrptr[POTR_HEADER_LEN] : 0;
  if(potr_is_helloack() || potr_is_ack()) {
    count = ilocs_parse_wake_up_counter(((uint8_t *)packetbuf_dataptr()) + 1);
    /* alpha = 0 doesn't need to be encoded */
  } else if(packetbuf_holds_broadcast()) {
    count = forward
        ? secrdc_get_wake_up_counter(secrdc_get_next_strobe_start() + SECRDC_WAKEUP_INTERVAL)
        : restore_wake_up_counter(phase);
    /* store alpha = 3 together with wake-up counter */
    count.u32 += 0xC0000000;
#if POTR_CONF_WITH_ANYCAST
  } else if(packetbuf_holds_anycast()) {
    if (forward) {
      /* this should be our wake-up counter when we send the next strobe */
      count = secrdc_get_wake_up_counter(secrdc_get_next_strobe_start());
    } else {
      /* we are receiving */
      count = restore_anycast_wakeup_counter(phase);
    }
    
    /* store alpha = 4 together with wake-up counter */
    count.u32 += 0xE0000000;
#endif /* POTR_CONF_WITH_ANYCAST */
  } else {
    count = forward
        ? ccm_star_packetbuf_predict_wake_up_counter(phase)
        : secrdc_get_wake_up_counter(secrdc_get_last_wake_up_time());
    /* store alpha = 1 together with wake-up counter */
    count.u32 += 0x40000000;
  }
  ilocs_write_wake_up_counter(nonce + 9, count);
#elif LLSEC802154_USES_FRAME_COUNTER
#if SECRDC_WITH_SECURE_PHASE_LOCK
  hdrptr = packetbuf_hdrptr();
  nonce[8] = potr_has_strobe_index(hdrptr[0]) ? hdrptr[POTR_HEADER_LEN] : 0;
#else /* SECRDC_WITH_SECURE_PHASE_LOCK */
  nonce[8] = packetbuf_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_2_3) >> 8;
#endif /* SECRDC_WITH_SECURE_PHASE_LOCK */
  nonce[9] = packetbuf_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_2_3) & 0xff;
  nonce[10] = packetbuf_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_0_1) >> 8;
  nonce[11] = packetbuf_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_0_1) & 0xff;
#if LLSEC802154_USES_AUX_HEADER
  nonce[12] = packetbuf_attr(PACKETBUF_ATTR_SECURITY_LEVEL);
#else /* LLSEC802154_USES_AUX_HEADER */
  nonce[12] = packetbuf_holds_broadcast() ? 0xFF : packetbuf_attr(PACKETBUF_ATTR_NEIGHBOR_INDEX);
#endif /* LLSEC802154_USES_AUX_HEADER */
#endif /* ILOCS_ENABLED */
}
/*---------------------------------------------------------------------------*/
void
ccm_star_packetbuf_to_acknowledgement_nonce(uint8_t *nonce)
{
#if ILOCS_ENABLED
  nonce[12] |= (1 << 7);
  nonce[12] &= ~(1 << 6);
#else /* ILOCS_ENABLED */
  nonce[12] = 0xFE;
#endif /* ILOCS_ENABLED */
}
/*---------------------------------------------------------------------------*/
