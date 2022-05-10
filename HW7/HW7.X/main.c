#include<xc.h>           // processor SFR definitions
#include<sys/attribs.h>  // __ISR macro
#include <stdio.h>
#include "uart.h"
#include "i2c_master_noint.h"
#include "mpu6050.h"

#define DESIRED_BAUD 230400
#define SYS_FREQ 48000000ul
#define MCP23008 0b0100000 // MCP23008 i2c address

// DEVCFG0
#pragma config DEBUG = OFF // disable debugging
#pragma config JTAGEN = OFF // disable jtag
#pragma config ICESEL = ICS_PGx1 // use PGED1 and PGEC1
#pragma config PWP = OFF // disable flash write protect
#pragma config BWP = OFF // disable boot write protect
#pragma config CP = OFF // disable code protect

// DEVCFG1
#pragma config FNOSC = FRCPLL // use fast frc oscillator with pll
#pragma config FSOSCEN = OFF // disable secondary oscillator
#pragma config IESO = OFF // disable switching clocks
#pragma config POSCMOD = OFF // primary osc disabled
#pragma config OSCIOFNC = OFF // disable clock output
#pragma config FPBDIV = DIV_1 // divide sysclk freq by 1 for peripheral bus clock
#pragma config FCKSM = CSDCMD // disable clock switch and FSCM
#pragma config WDTPS = PS1048576 // use largest wdt value
#pragma config WINDIS = OFF // use non-window mode wdt
#pragma config FWDTEN = OFF // wdt disabled
#pragma config FWDTWINSZ = WINSZ_25 // wdt window at 25%

// DEVCFG2 - get the sysclk clock to 48MHz from the 8MHz fast rc internal oscillator
#pragma config FPLLIDIV = DIV_2 // divide input clock to be in range 4-5MHz
#pragma config FPLLMUL = MUL_24 // multiply clock after FPLLIDIV
#pragma config FPLLODIV = DIV_2 // divide clock after FPLLMUL to get 48MHz

// DEVCFG3
#pragma config USERID = 0b0000111100001111 // some 16bit userid, doesn't matter what
#pragma config PMDL1WAY = OFF // allow multiple reconfigurations
#pragma config IOL1WAY = OFF // allow multiple reconfigurations

void delay(int counts) {
    _CP0_SET_COUNT(0);
    while (_CP0_GET_COUNT() < counts) {}
}

void blink() {
    LATAbits.LATA4 = !LATAbits.LATA4;
}

int main() {

    __builtin_disable_interrupts(); // disable interrupts while initializing things

    // set the CP0 CONFIG register to indicate that kseg0 is cacheable (0x3)
    __builtin_mtc0(_CP0_CONFIG, _CP0_CONFIG_SELECT, 0xa4210583);

    // 0 data RAM access wait states
    BMXCONbits.BMXWSDRM = 0x0;

    // enable multi vector interrupts
    INTCONbits.MVEC = 0x1;

    // disable JTAG to get pins back
    DDPCONbits.JTAGEN = 0;

    // do your TRIS and LAT commands here
    TRISAbits.TRISA4 = 0;
    LATAbits.LATA4 = 1;
    TRISBbits.TRISB4 = 1;

    UART_Startup();
    init_mpu6050();
    __builtin_enable_interrupts();
    
    char m_in[100];
    char m_out[200];
    int i;
    
    #define NUM_DATA_PNTS 300
    float ax[NUM_DATA_PNTS], ay[NUM_DATA_PNTS], az[NUM_DATA_PNTS],
            gx[NUM_DATA_PNTS], gy[NUM_DATA_PNTS], gz[NUM_DATA_PNTS],
            temp[NUM_DATA_PNTS];
    
    char who = whoami();
    if (who != 0x68) {
        while(1) {
            LATAbits.LATA4 = 0;
            
        }
    }
    
    char IMU_buf[IMU_ARRAY_LEN]; // 8 bit data array for imu data
    
    while (1) {
        
        ReadUART1(m_in, 100); // wait for newline
        blink();
        
        for (i=0; i<NUM_DATA_PNTS; i++) {
            _CP0_SET_COUNT(0);
            // read IMU
            burst_read_mpu6050(IMU_buf);
            ax[i] = conv_xXL(IMU_buf);
            ay[i] = conv_yXL(IMU_buf);
            az[i] = conv_zXL(IMU_buf);
            gx[i] = conv_xG(IMU_buf);
            gy[i] = conv_yG(IMU_buf);
            gz[i] = conv_zG(IMU_buf);
            temp[i] = conv_temp(IMU_buf);
            
            while (_CP0_GET_COUNT()<24000000/2/100){}
        }
        
        for (i=0; i<NUM_DATA_PNTS; i++) {
            sprintf(m_out, "%d %f %f %f %f %f %f %f\r\n", 
                    NUM_DATA_PNTS-i, ax[i], ay[i], az[i], gx[i], gy[i], gz[i], temp[i]);
            WriteUART1(m_out);
        }
    }
}