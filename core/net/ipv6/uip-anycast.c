#include "uip-anycast.h"

#include <stdio.h>

static uip_ipaddr_t anycast_ip_addr;
static uip_lladdr_t anycast_ll_addr;

void
init_uip_anycast()
{
    uip_ip6addr(&anycast_ip_addr, 0xff00, 0, 0, 0, 0, 0, 0, 0x0001);
    anycast_ll_addr.addr[0] = 0xfe;
    anycast_ll_addr.addr[1] = 0xfe;
    anycast_ll_addr.addr[2] = 0xfe;
    anycast_ll_addr.addr[3] = 0xfe;
    anycast_ll_addr.addr[4] = 0xfe;
    anycast_ll_addr.addr[5] = 0xfe;
    anycast_ll_addr.addr[6] = 0xfe;
    anycast_ll_addr.addr[7] = 0xfe;
    // anycast_ll_addr.addr[0] = 0x00;
    // anycast_ll_addr.addr[1] = 0x12;
    // anycast_ll_addr.addr[2] = 0x4b;
    // anycast_ll_addr.addr[3] = 0x00;
    // anycast_ll_addr.addr[4] = 0x04;
    // anycast_ll_addr.addr[5] = 0x30;
    // anycast_ll_addr.addr[6] = 0x53;
    // anycast_ll_addr.addr[7] = 0x2f;
    
}

void
create_anycast_addr(uip_ipaddr_t *addr)
{
    /* use memcpy to avoid conversion from host to network byte order */
    memcpy(addr, &anycast_ip_addr, sizeof(anycast_ip_addr));
}

void
uip_create_ll_anycast_addr(uip_lladdr_t *addr) 
{
    memcpy(addr, &anycast_ll_addr, sizeof(anycast_ll_addr));
}

int
uip_is_anycast_addr(uip_ipaddr_t *addr)
{
    return uip_ip6addr_cmp(addr, &anycast_ip_addr);
}

int
uip_is_anycast_ll_addr(uip_lladdr_t *addr)
{
    return (memcmp(addr, &anycast_ll_addr, sizeof(anycast_ll_addr)) == 0);
}

void
print_addr(uip_ipaddr_t *addr)
{
  int i;
  for(i = 0; i < 8; ++i)
  {
    printf("%04x:", uip_htons(addr->u16[i]));
  }
  printf("\n");
}
