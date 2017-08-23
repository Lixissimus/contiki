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
 *         An led debug print helper library
 * \author
 *         Felix Wolff <lixissimus@gmail.com>
 */

#include "dev/leds.h"
#include "sys/clock.h"
#include "sys/ctimer.h"

#include "led-debug.h"

#define TIME (2*CLOCK_SECOND / 1)

static struct ctimer red_timer;
static struct ctimer yellow_timer;
static struct ctimer green_timer;
static struct ctimer all_timer;

static void
turn_off(void *d)
{
  leds_off(*((unsigned char*)d));
}

static void
set(unsigned char *color, struct ctimer *timer)
{
  if(!ctimer_expired(timer)) {
    ctimer_restart(timer);
    return;
  } else if (leds_get() & *color) {
    /* this color was on, but we din't turn it on, leave it as is */
    return;
  }
  leds_on(*color);
  ctimer_set(timer, TIME, turn_off, color);
}

void
led_debug_set_red(void)
{
  static unsigned char red = LEDS_RED;
  set(&red, &red_timer);
}

void
led_debug_set_yellow(void)
{
  static unsigned char yellow = LEDS_YELLOW;
  set(&yellow, &yellow_timer);
}

void
led_debug_set_green(void)
{
  static unsigned char green = LEDS_GREEN;
  set(&green, &green_timer);
}

void
led_debug_set_all(void)
{
  static unsigned char all = LEDS_ALL;
  set(&all, &all_timer);
}
