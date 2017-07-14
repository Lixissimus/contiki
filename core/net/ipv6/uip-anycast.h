#ifndef UIP_ANYCAST_H_
#define UIP_ANYCAST_H_

#include "net/ip/uip.h"


void init_uip_anycast();
void uip_create_ipv6_anycast_addr(uip_ipaddr_t *addr);
int uip_is_anycast_addr(uip_ipaddr_t *addr);
void uip_create_ll_anycast_addr(uip_lladdr_t *addr);
int uip_is_anycast_ll_addr(uip_lladdr_t *addr);
void print_addr(uip_ipaddr_t *addr);

#endif /* UIP_ANYCAST_H_ */