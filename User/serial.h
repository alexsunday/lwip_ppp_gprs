// serial include header file.

#ifndef _GUARD_H_SERIAL_H_
#define _GUARD_H_SERIAL_H_

#include <stdint.h>

void modemWriteData(const char* buf, uint32_t in_length);
void uart1SendContent(const uint8_t *buf, uint16_t in_length);
void modemSerialInitialize(uint32_t baud);
void mainSerialInitialize(uint32_t baud);

#endif
