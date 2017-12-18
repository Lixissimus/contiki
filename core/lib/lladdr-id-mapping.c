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
#if 0
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
#elif 0
/* wall setup */
static const uint8_t nbrs[11][11] = {
  { 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0 },
  { 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0 },
  { 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1 },
  { 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1 },
  { 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0 },
  { 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0 },
  { 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1 },
  { 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1 },
  { 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0 },
  { 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0 },
  { 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1 }
};
#elif 0
/* only restrict neighborhood of node 1 */
static const uint8_t nbrs[11][11] = {
  { 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0 },
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
  { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
  { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
  { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
  { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
  { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
  { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }
};
#elif 0
/* small desk setup */
static const uint8_t nbrs[2][2] = {
  { 1, 1 },
  { 1, 1 }
}
#elif 0
/* full benchmark setup 1 */
static const uint8_t nbrs[20][20] = {
  /* root */
  { 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 6
  /* layer 1 */
  { 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 8
  { 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 9
  { 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 9
  { 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 9
  { 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 8
  /* layer 2 */
  { 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0 }, // 12
  { 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0 }, // 13
  { 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0 }, // 13
  { 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 }, // 13
  { 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 }, // 12
  /* layer 3 */
  { 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1 }, // 11
  { 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1 }, // 12
  { 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1 }, // 12
  { 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1 }, // 12
  { 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1 }, // 11
  /* layer 4 */
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0 }, // 7
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0 }, // 8
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1 }, // 8
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1 }  // 7
};
#elif 0
/* full benchmark setup 2 */
static const uint8_t nbrs[20][20] = {
  /* root */
  { 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 5
  { 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 6
  { 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 }, // 7
  { 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 }, // 4
  { 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1 }, // 7
  // continue!!
  { 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1 }, // 4
  { 1, 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1 }, // 8
  { 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0 }, // 9
  { 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0 }, // 9
  { 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0 }, // 10

  { 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1 }, // 10
  { 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1 }, // 8
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1 }, // 4
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1 }, // 6
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0 }, // 5

  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0 }, // 6
  { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0 }, // 5
  { 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0 }, // 5
  { 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0 }, // 8
  { 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 0, 1 }, // 9
};
#elif 0
/* full benchmark setup 2 */
static const uint8_t nbrs[20][20] = {
  /* root */
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1 }, // 5
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 5
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 5
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 5
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 5
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 5
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 5
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 5
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 5
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 5

  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 5
  { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 5
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 5
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 5
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 5
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 5
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 5
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 5
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 5
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }  // 5
};
#elif 1
/* full benchmark setup 2 */
static const uint8_t nbrs[20][20] = {
  /* root */
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1 }, // 5
  { 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 5
  { 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 5
  { 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 5
  { 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 5
  { 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 5
  { 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 5
  { 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 5
  { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 5
  { 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 5

  { 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 5
  { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 5
  { 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 5
  { 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 5
  { 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 5
  { 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 5
  { 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 5
  { 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 5
  { 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 5
  { 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } // 5
};
#endif

struct id_linkaddr {
  uint16_t id;
  linkaddr_t addr;
};

static const struct id_linkaddr id_linkaddr_list[] = {
  /* wall network */
#if 0
  { 1, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0x2b}} },
  { 2, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0x29}} },
  { 3, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x52,0xe8}} },
  { 4, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x52,0xf1}} },
  { 5, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0xb8}} },
  { 6, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0x2f}} },
  { 7, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x54,0x03}} },
  { 8, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0x73}} },
  { 9, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0xd9}} },
  { 10, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0x66}} },
  { 11, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0x32}} }
#elif 0
  /* desk test motes uni*/
  /* with tape */
  { 1, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0xd9}} },
  /* without tape */
  { 2, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0xc1}} }
#elif 1
  /* home */
  { 1, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0x29}} },
  { 2, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x52,0xe6}} }
#elif 1
  /* full benchmark setup */
  { 1, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0x2b}} },
  { 2, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0x29}} },
  { 3, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x52,0xe8}} },
  { 4, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x52,0xf1}} },
  { 5, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0xb8}} },
  { 6, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0x2f}} },
  { 7, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0x5d}} },
  { 8, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0x73}} },
  { 9, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0xd9}} },
  { 10, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0x66}} },
  { 11, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0x32}} },
  { 12, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0xc1}} },
  { 13, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x52,0xe7}} },
  { 14, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0x95}} },
  { 15, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0x18}} },
  { 16, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x54,0x6f}} },
  { 17, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0xc4}} },
  { 18, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x52,0xe6}} },
  { 19, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0xc6}} },
  { 20, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0x1a}} },
  { 21, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x54,0x03}} }
#elif 1
  { 1, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x53,0x32}} },
  { 2, {{0x00,0x12,0x4b,0x00,0x04,0x30,0x52,0xe7}} }
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
