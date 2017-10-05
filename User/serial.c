#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "stm32f10x.h"
#include "serial.h"

static volatile uint8_t modem_rx_buf[1600];
static volatile uint8_t modem_tx_buf[1600];
static volatile uint8_t serial1_rx_buf[128];
static volatile uint8_t serial1_tx_buf[128];

__asm unsigned int CPU_SR_Save(void)
{
     MRS     R0, PRIMASK
     CPSID   I
     BX      LR
}

__asm void CPU_SR_Restore(unsigned int cpu_sr)
{
     MSR     PRIMASK, R0
     BX      LR  
}

void reset_dma_ptr(DMA_Channel_TypeDef* channel, int length)
{
	unsigned int sr;
	
	sr = CPU_SR_Save();
	DMA_Cmd(channel, DISABLE);
	DMA_SetCurrDataCounter(DMA1_Channel6, length);
	DMA_Cmd(channel, ENABLE);
	CPU_SR_Restore(sr);
}

void modemWriteData(const char* buf, uint32_t in_length)
{
	if(in_length > sizeof(modem_tx_buf)) {
		return;
	}
	if(in_length == 0) {
		return;
	}
	
	while(!DMA_GetFlagStatus(DMA1_FLAG_TC7));
	DMA_Cmd(DMA1_Channel7, DISABLE);

	memcpy((void*)modem_tx_buf, buf, in_length);
	DMA_ClearFlag(DMA1_FLAG_TC7);
	DMA_SetCurrDataCounter(DMA1_Channel7, in_length);
	DMA_Cmd(DMA1_Channel7, ENABLE);
}


void uart1SendContent(const uint8_t *buf, uint16_t in_length)
{
	if(in_length > sizeof(serial1_tx_buf)) {
		return;
	}
	if(in_length == 0) {
		return;
	}
	
	while(!DMA_GetFlagStatus(DMA1_FLAG_TC4));
	DMA_Cmd(DMA1_Channel4, DISABLE);

	memcpy((char*)serial1_tx_buf, buf, in_length);
	DMA_ClearFlag(DMA1_FLAG_TC4);
	DMA_SetCurrDataCounter(DMA1_Channel4, in_length);
	DMA_Cmd(DMA1_Channel4, ENABLE);
}


int fputc(int ch, FILE *f)  
{  
  USART_SendData(USART1, (uint8_t) ch);  
  
  while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);  
  
  return (ch);  
}

void modemSerialInitialize(uint32_t baud)
{
	DMA_InitTypeDef dma;
	GPIO_InitTypeDef gpio;
	USART_InitTypeDef usart;
	NVIC_InitTypeDef nvic;
	
	//rcc
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	
	//gpio
	GPIO_StructInit(&gpio);
	gpio.GPIO_Pin = GPIO_Pin_2;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	gpio.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &gpio);
	gpio.GPIO_Pin = GPIO_Pin_3;
	gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &gpio);
	
	//usart
	USART_StructInit(&usart);
	usart.USART_BaudRate = baud;

	usart.USART_WordLength = USART_WordLength_8b;
	usart.USART_StopBits = USART_StopBits_1;
	usart.USART_Parity = USART_Parity_No;
	usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART2, &usart);
	//USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
	USART_ITConfig(USART2, USART_IT_RXNE, DISABLE);
	USART_ITConfig(USART2, USART_IT_TC, DISABLE);  
	USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);

	//dma rx
	DMA_DeInit(DMA1_Channel6);
	dma.DMA_PeripheralBaseAddr = (uint32_t)(&USART2->DR);
	dma.DMA_MemoryBaseAddr = (uint32_t) modem_rx_buf;
	dma.DMA_DIR = DMA_DIR_PeripheralSRC;
	dma.DMA_BufferSize = sizeof(modem_rx_buf);
	dma.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	dma.DMA_MemoryInc = DMA_MemoryInc_Enable;
	dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	dma.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	dma.DMA_Mode = DMA_Mode_Normal;
	dma.DMA_Priority = DMA_Priority_VeryHigh;
	dma.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel6, &dma);
	DMA_Cmd(DMA1_Channel6, ENABLE);

	//dma tx
	dma.DMA_BufferSize          = 1;
	dma.DMA_DIR                 = DMA_DIR_PeripheralDST;
	dma.DMA_M2M                 = DMA_M2M_Disable;
	dma.DMA_MemoryBaseAddr      = (uint32_t) modem_tx_buf;
	dma.DMA_MemoryDataSize      = DMA_MemoryDataSize_Byte;
	dma.DMA_MemoryInc           = DMA_MemoryInc_Enable;
	dma.DMA_Mode                = DMA_Mode_Normal;
	dma.DMA_PeripheralBaseAddr  = (uint32_t)&USART2->DR;
	dma.DMA_PeripheralDataSize  = DMA_PeripheralDataSize_Byte;
	dma.DMA_PeripheralInc       = DMA_PeripheralInc_Disable;
	dma.DMA_Priority            = DMA_Priority_High;
	DMA_DeInit(DMA1_Channel7);
	DMA_Init(DMA1_Channel7, &dma);
	DMA_Cmd(DMA1_Channel7, ENABLE);
	//nvic
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	nvic.NVIC_IRQChannel = USART2_IRQn;
	nvic.NVIC_IRQChannelPreemptionPriority = 1;
	nvic.NVIC_IRQChannelSubPriority = 1;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic);

	USART_DMACmd(USART2, USART_DMAReq_Rx, ENABLE);
	USART_DMACmd(USART2, USART_DMAReq_Tx, ENABLE);
	USART_Cmd(USART2, ENABLE);

	memset((char *)modem_rx_buf, 0, sizeof(modem_rx_buf));
	memset((char *)modem_tx_buf, 0, sizeof(modem_tx_buf));
}


void mainSerialInitialize(uint32_t baud)
{
	DMA_InitTypeDef dma;
	GPIO_InitTypeDef gpio;
	USART_InitTypeDef usart;
	NVIC_InitTypeDef nvic;
	
	//rcc
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	
	//gpio
	GPIO_StructInit(&gpio);
	gpio.GPIO_Pin = GPIO_Pin_9;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	gpio.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &gpio);
	
	gpio.GPIO_Pin = GPIO_Pin_10;
	gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &gpio);
	
	//usart
	USART_StructInit(&usart);
	usart.USART_BaudRate = baud;

	usart.USART_WordLength = USART_WordLength_8b;
	usart.USART_StopBits = USART_StopBits_1;
	usart.USART_Parity = USART_Parity_No;
	usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &usart);
	//USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
	USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
	USART_ITConfig(USART1, USART_IT_TC, DISABLE);  
	USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);

	//dma rx
	DMA_DeInit(DMA1_Channel5);
	dma.DMA_PeripheralBaseAddr = (uint32_t)(&USART1->DR);
	dma.DMA_MemoryBaseAddr = (uint32_t) serial1_rx_buf;
	dma.DMA_DIR = DMA_DIR_PeripheralSRC;
	dma.DMA_BufferSize = sizeof(serial1_rx_buf);
	dma.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	dma.DMA_MemoryInc = DMA_MemoryInc_Enable;
	dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	dma.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	dma.DMA_Mode = DMA_Mode_Normal;
	dma.DMA_Priority = DMA_Priority_VeryHigh;
	dma.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel5, &dma);
	DMA_Cmd(DMA1_Channel5, ENABLE);

	//dma tx
	dma.DMA_BufferSize          = 1;
	dma.DMA_DIR                 = DMA_DIR_PeripheralDST;
	dma.DMA_M2M                 = DMA_M2M_Disable;
	dma.DMA_MemoryBaseAddr      = (uint32_t)serial1_tx_buf;
	dma.DMA_MemoryDataSize      = DMA_MemoryDataSize_Byte;
	dma.DMA_MemoryInc           = DMA_MemoryInc_Enable;
	dma.DMA_Mode                = DMA_Mode_Normal;
	dma.DMA_PeripheralBaseAddr  = (uint32_t)&USART1->DR;
	dma.DMA_PeripheralDataSize  = DMA_PeripheralDataSize_Byte;
	dma.DMA_PeripheralInc       = DMA_PeripheralInc_Disable;
	dma.DMA_Priority            = DMA_Priority_High;
	DMA_DeInit(DMA1_Channel4);
	DMA_Init(DMA1_Channel4, &dma);
	DMA_Cmd(DMA1_Channel4, ENABLE);
	//nvic
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	nvic.NVIC_IRQChannel = USART1_IRQn;
	nvic.NVIC_IRQChannelPreemptionPriority = 0;
	nvic.NVIC_IRQChannelSubPriority = 1;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic);

	USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);
	USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);
	USART_Cmd(USART1, ENABLE);
		
	memset((char *)serial1_rx_buf, 0, sizeof(serial1_rx_buf));
	memset((char *)serial1_tx_buf, 0, sizeof(serial1_tx_buf));
}


// serial 1
void USART1_IRQHandler(void)
{
  uint8_t clear = clear;

	if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET) {
		clear = USART1->SR;
		clear = USART1->DR;
		memset((char*)serial1_rx_buf, 0, sizeof(serial1_rx_buf));
		reset_dma_ptr(DMA1_Channel5, sizeof(serial1_rx_buf));
	}
}


// modem
void USART2_IRQHandler(void)
{
	uint8_t clear = clear;

	if(USART_GetITStatus(USART2, USART_IT_IDLE) != RESET) {
		clear = USART2->SR;
		clear = USART2->DR;
		
		uart1SendContent((void*)modem_rx_buf, strlen((const char*)modem_rx_buf));
		memset((char*)modem_rx_buf, 0, sizeof(modem_rx_buf));
		reset_dma_ptr(DMA1_Channel6, sizeof(modem_rx_buf));
	}
}
