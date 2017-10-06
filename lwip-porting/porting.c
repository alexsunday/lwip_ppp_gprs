#include "lwip/etharp.h"

const struct eth_addr ethbroadcast = {{0xff,0xff,0xff,0xff,0xff,0xff}};
const struct eth_addr ethzero = {{0,0,0,0,0,0}};


unsigned int system_tick_num = 0;
unsigned int sys_now(void)
{
    return system_tick_num;
}

const char* lwip_strerr(err_t code)
{
 return "lwip_strerr, code not implement.";
}

