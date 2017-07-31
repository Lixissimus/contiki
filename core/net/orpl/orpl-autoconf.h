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

/* some defines from orpl legacy code */
// #include "orpl-contiki-conf.h"

/* enable anycast support */
#undef POTR_CONF_WITH_ANYCAST
#define POTR_CONF_WITH_ANYCAST 1

/* just works with lladdr size of 8 */
#undef LINKADDR_CONF_SIZE
#define LINKADDR_CONF_SIZE 8

/* disable RPL */
#undef UIP_CONF_IPV6_RPL
#define UIP_CONF_IPV6_RPL 0

/* this also sets the root's rank to 0 */
#undef RPL_CONF_MIN_HOPRANKINC
#define RPL_CONF_MIN_HOPRANKINC 0

/* ORPL does not use RPL's normal downwards routing */
#undef RPL_CONF_MOP
#define RPL_CONF_MOP RPL_MOP_NO_DOWNWARD_ROUTES

/* use bitmap as routing set type */
#undef OPRL_CONF_RS_TYPE
#define OPRL_CONF_RS_TYPE ORPL_RS_TYPE_BITMAP

/* define size of routing set */
#undef ORPL_CONF_ROUTING_SET_M
#define ORPL_CONF_ROUTING_SET_M 32

/* enable ORPL */
#undef ORPL_ENABLED
#define ORPL_ENABLED 1
 