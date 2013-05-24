
/**
 * \file
 *         Blom's scheme
 */

#include "blom.h"

/*---------------------------------------------------------------------------*/
uint64_t
mod_0x100000001(uint64_t divident) {
  uint32_t least;
  uint32_t most;

  least = divident & 0xFFFFFFFF;
  most = divident >> 32;

  if (least >= most) {
    return least - most;
  } else {
    return 0x100000001ULL + least - most;
  }
}
/*---------------------------------------------------------------------------*/
uint64_t
mult_0x100000001(uint64_t x, uint64_t y)
{
  if ((x <= 0x100000001 - 2) || (y <= 0x100000001 - 2)) {
    return mod_0x100000001(x * y);
  } else if ((x == 0x100000001 - 1) && (y == 0x100000001 - 1)) {
    return 0;
  } else {
    return 0x100000000;
  }
}
/*---------------------------------------------------------------------------*/
/**
 * \brief blom wihout using % operators
 * \param a coefficients
 * \param id identifier
 */
uint32_t
blom_0x100000001_optimized(uint32_t *a, uint32_t id)
{
  uint8_t i;
  uint64_t exp;
  uint64_t sum;
  sum = a[0];
  exp = id;

  for (i = 1; i < LAMBDA; i++) {
    sum += mult_0x100000001(a[i], exp);
    exp *= id;
    
    sum = mod_0x100000001(sum);
    exp = mod_0x100000001(exp);
  }
  return (uint32_t) sum;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief blom by means of % operators
 * \param a coefficients
 * \param id identifier
 */
uint32_t
blom_0x100000001(uint32_t *a, uint32_t id)
{
  uint8_t i;
  uint64_t exp;
  uint64_t sum;

  sum = a[0];
  exp = id;

  for (i = 1; i < LAMBDA; i++) {
    sum += ((a[i] * exp) % 0x100000001);
    exp *= id;

    sum %= 0x100000001;
    exp %= 0x100000001;
  }

  return (uint32_t) sum;
}
/*---------------------------------------------------------------------------*/
uint32_t
mod_0x10001(uint32_t divident)
{
  uint16_t least;
  uint16_t most;

  least = divident & 0xFFFF;
  most = divident >> 16;

  if (least >= most) {
    return least - most;
  } else {
    return 0x10001 + least - most;
  }
}
/*---------------------------------------------------------------------------*/
uint32_t
mult_0x10001(uint32_t x, uint32_t y)
{
  if ((x <= 0x10001 - 2) || (y <= 0x10001 - 2)) {
    return mod_0x10001(x * y);
  } else if ((x == 0x10001 - 1) && (y == 0x10001 - 1)) {
    return 1;
  } else {
    return 2;
  }
}
/*---------------------------------------------------------------------------*/
/**
 * \brief blom wihout using % operators
 * \param a coefficients
 * \param id identifier
 */
uint16_t
blom_0x10001_optimized(uint16_t *a, uint16_t id)
{
  uint8_t i;
  uint32_t exp;
  uint32_t sum;
  sum = a[0];
  exp = id;

  for (i = 1; i < LAMBDA; i++) {
    sum += mult_0x10001(a[i], exp);
    exp *= id;
    
    sum = mod_0x10001(sum);
    exp = mod_0x10001(exp);
  }
  return (uint16_t) sum;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief blom by means of % operators
 * \param a coefficients
 * \param id identifier
 */
uint16_t
blom_0x10001(uint16_t *a, uint16_t id)
{
  uint8_t i;
  uint32_t exp;
  uint32_t sum;

  sum = a[0];
  exp = id;

  for (i = 1; i < LAMBDA; i++) {
    sum += ((a[i] * exp) % 0x10001);
    exp *= id;

    sum %= 0x10001;
    exp %= 0x10001;
  }

  return (uint16_t) sum;
}
/*---------------------------------------------------------------------------*/
uint32_t
mod_0xFFF1(uint32_t divident)
{
  uint16_t least;
  uint16_t most;
  uint32_t remainder;

  least = divident & 0xFFFF;
  most = divident >> 16;

  /**
   * divident mod 2^16-15
   * = (most * 2^N + least) mod 2^16-15
   * = [(most * 2^N mod 2^16-15) + (least mod 2^16-15)] mod 2^16-15
   * = [ 15 * most               + least              ] mod 2^16-15
   */
  remainder = 15 * most         + least;
  
  while (remainder >= 0xFFF1) {
    remainder -= 0xFFF1;
  }
  
  return remainder;
}
/*---------------------------------------------------------------------------*/
uint16_t
blom_0xFFF1_optimized(uint16_t *a, uint16_t id)
{
  uint8_t i;
  uint32_t exp;
  uint32_t sum;
  sum = a[0];
  exp = id;

  for (i = 1; i < LAMBDA; i++) {
    sum += mod_0xFFF1(a[i] * exp);
    exp *= id;
    
    sum = mod_0xFFF1(sum);
    exp = mod_0xFFF1(exp);
  }
  return (uint16_t) sum;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief blom by means of % operators
 * \param a coefficients
 * \param id identifier
 */
uint16_t
blom_0xFFF1(uint16_t *a, uint16_t id)
{
  uint8_t i;
  uint32_t exp;
  uint32_t sum;

  sum = a[0];
  exp = id;

  for (i = 1; i < LAMBDA; i++) {
    sum += ((a[i] * exp) % 0xFFF1);
    exp *= id;

    sum %= 0xFFF1;
    exp %= 0xFFF1;
  }

  return (uint16_t) sum;
}