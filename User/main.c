#include <stdio.h>
#include "stm32f10x.h"
#include "serial.h"


int main(void)
{
	modemSerialInitialize(115200);
	mainSerialInitialize(115200);
	
  while (1) {
		printf("HELLO, WORLD\n");
  }
}
