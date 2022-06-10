#ifndef _SPI_H
#define _SPI_H
#include<xc.h>           // processor SFR definitions
#include<sys/attribs.h>  // __ISR macro

// initialize SPI1
void initSPI();

// send a byte via spi and return the response
unsigned char spi_io(unsigned char o);

#endif