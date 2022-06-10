#ifndef _UART_H
#define _UART_H
#include<stdio.h>
#define DESIRED_BAUD 230400
#define SYS_FREQ 48000000ul

void UART_Startup();

void ReadUART1(char * message, int maxLength);

void WriteUART1(const char * string);

#endif
