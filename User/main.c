#include <stdio.h>
#include <string.h>

#include "stm32f10x.h"
#include "serial.h"


void delay(void)
{
	int i, j;
	for(i=0; i!=2000; ++i) 
		for(j=0; j!=2000; ++j);
}

#define AT_CMD "AT\r\n"
void test(void)
{
	while(1) {
		//printf("HELLO, WORLD\n");
		delay();
		modemWriteData(AT_CMD, strlen(AT_CMD));
	}
}

int main(void)
{
	modemSerialInitialize(115200);
	mainSerialInitialize(115200);
	
	test();
	
  while (1);
}
