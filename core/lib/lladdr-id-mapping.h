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

#ifndef LLADDR_ID_MAPPING_H_
#define LLADDR_ID_MAPPING_H_

#include "net/linkaddr.h"
#include "net/ip/uip.h"

uint16_t lladdr_id_mapping_own_id(void);
uint16_t lladdr_id_mapping_id_from_ll(const linkaddr_t *addr);
int lladdr_id_mapping_ll_from_id(const uint16_t id, linkaddr_t *addr);
uint16_t lladdr_id_mapping_id_from_ipv6(const uip_ipaddr_t *ip_addr);
int lladdr_id_mapping_ipv6_from_id(const uint16_t id, uip_ipaddr_t *ip_addr);
int lladdr_id_mapping_ll_from_ipv6(const uip_ipaddr_t *ipaddr, linkaddr_t *lladdr);
int lladdr_id_mapping_are_nbrs(const uint16_t id1, const uint16_t id2);

#endif /* LLADDR_ID_MAPPING_H_ */
