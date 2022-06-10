#include<xc.h>           // processor SFR definitions
#include<sys/attribs.h>  // __ISR macro
#include <stdio.h>
#include "uart.h"
#include "i2c_master_noint.h"
#include "ssd1306.h"
#include "ws2812b.h"

#define DESIRED_BAUD 230400
#define SYS_FREQ 48000000ul
#define MCP23008 0b0100000 // MCP23008 i2c address
#define ONE_SECOND 24000000

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

void i2c_write(unsigned char adr, unsigned char reg, unsigned char val) {
    i2c_master_start();
    i2c_master_send(adr<<1);
    i2c_master_send(reg);
    i2c_master_send(val);
    i2c_master_stop();
}

unsigned char i2c_read(unsigned char adr, unsigned char reg) {
    unsigned char read;
    i2c_master_start();
    i2c_master_send(adr<<1); // send write address
    i2c_master_send(reg);
    i2c_master_restart();
    i2c_master_send(adr<<1|0b1); // send read address
    read = i2c_master_recv();
    i2c_master_ack(1);
    i2c_master_stop();
    return read;
}


void speaker_setup() {
    TRISBbits.TRISB12 = 0; // motor direction
    LATBbits.LATB12 = 0; // motor forward?

    // B15 OC1
    RPB15Rbits.RPB15R = 0b0101;
    T3CONbits.TCKPS = 3; // T3 prescaler N=4
    PR3 = 500; // 40 kHz PWM, inaudible
    TMR3 = 0;
    OC1CONbits.OCM = 0b110;
    OC1RS = 0; // no volume
    OC1R = 0;

    OC1CONbits.OCTSEL = 1; // use T3
    T3CONbits.ON = 1;
    OC1CONbits.ON = 1;
}

void set_freq(float freq) {
    PR3 = (int)(1/(freq*.0000000125*4) - 1);
    TMR3 = 0;
    OC1RS = PR3/2; // 1/4 volume
}

void speaker_off() {
    OC1RS = 0;
}

void speaker_on() {
    OC1RS = PR3/4;
}

void motor_setup() {
    TRISBbits.TRISB12 = 0; // motor direction
    LATBbits.LATB12 = 0; // motor forward?

    // B13 OC4
    RPB13Rbits.RPB13R = 0b0101;
//    T3CONbits.TCKPS = 3; // T3 prescaler N=4
//    PR3 = 1000; // 20 kHz PWM
//    TMR3 = 0;
    OC4CONbits.OCM = 0b110;
    OC4RS = 0; // no speed
    OC4R = 0;

    OC4CONbits.OCTSEL = 0; // use T2
//    T3CONbits.ON = 1;
    OC4CONbits.ON = 1;

}
    
void set_speed(int speed) {
    if (speed > 0) {
        LATBbits.LATB12 = 0; // motor forward
        if (speed > 100) {
            speed = 100;
        }
    }
    if (speed < 0) {
        LATBbits.LATB12 = 1; // motor backwards
        if (speed < -100) {
            speed = -100;
        }
        speed = -speed;
    }

    OC4RS = 500*speed;
}

    
void set_angle(int angle) {
    if (angle < -90) {
        angle = -90;
    }
    if (angle > 90) {
        angle = 90;
    }

    OC2RS = 2200 + angle*16;
}

void servo_setup() {
    RPA1Rbits.RPA1R = 0b0101; // A1 is OC2
    T2CONbits.TCKPS = 0b101; // T2 prescaler N=32
    PR2 = 49999; // period = (PR2+1) * N * 12.5 ns = 10 ms, 100Hz
    TMR2 = 0; // initial count is zero
    OC2CONbits.OCM = 0b110; // pwm mode without fault pin, other OC2CONbits are default
    OC2RS = 2200; // duty = OC1RS/(PR2+1) = 50%
    OC2R = 2200; // initialize before turning on OC2, after use OC2RS
    OC2CONbits.OCTSEL = 0; // timer 2 is input source
    T2CONbits.ON = 1; // turn on T2
    OC2CONbits.ON = 1; // turn on OC1
}


void ssd1306_drawMessage(unsigned char x, unsigned char y, char* message) {
    int i = 0;
    while (message[i] != '\0') {
        ssd1306_drawLetter(x + 5*i, y, message[i]);
        i++;
    }
}   
    
void message(char*msg) {
    ssd1306_clear();
    ssd1306_drawMessage(0,0,msg);
    ssd1306_update();
}

#define D4 587.33
#define Eb4 622.25
#define F4 698.46
#define G4 793.99
#define Bb4 932.33
#define D5 1174.66
#define C5 1046.5
#define F5 1396.91
#define G5 1567.98
#define NA 65535.

static float notes[64] = {D5, C5, Bb4, NA, Bb4, C5, Bb4, F4, D4, Eb4, F4, G4, F4, D4, F4, F4, 
                     Bb4, C5, D5, NA, D5, NA, D5, C5, Bb4, C5, D5, D5, C5, NA, C5, C5, D5, 
                     C5, Bb4, NA, Bb4, C5, Bb4, F4, D4, Eb4, F4, G4, F4, D4, F4, F4, Bb4, 
                     C5, D5, F5, F5, G5, F5, D5, Bb4, C5, D5, D5, C5, C5, Bb4, Bb4};

void T4_setup() {
    T4CONbits.T32 = 0; // 16 bit timer
    T4CONbits.TCKPS = 0b111; // N = 256
    PR4 = 64000;
    
    TMR4 = 0;
    T4CONbits.ON = 1;
    IPC4bits.T4IP = 2;
    IPC4bits.T4IS = 0;
    IFS0bits.T4IF = 0;
    IEC0bits.T4IE = 1;
}

void __ISR(_TIMER_4_VECTOR, IPL2SOFT) T2ISR(void) {
    static int counter = 0;
    set_freq(notes[counter]);
    counter++;
    if (counter == 64) {
        counter = 0;
    }
    LATAbits.LATA4 = !LATAbits.LATA4;
    IFS0bits.T4IF = 0; // clear flag
}


int main() {

    __builtin_disable_interrupts(); // disable interrupts while initializing things

    // set the CP0 CONFIG register to indicate that kseg0 is cacheable (0x3)
    __builtin_mtc0(_CP0_CONFIG, _CP0_CONFIG_SELECT, 0xa4210583);

    delay(ONE_SECOND*5);
    INTCONbits.MVEC = 0x1; // enable interrupts
    T4_setup();
    
    
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
    
    servo_setup();
    motor_setup();
    speaker_setup();
//    UART_Startup();
//    i2c_master_setup();
//    ssd1306_setup();
//    pico_uart_setup();
    __builtin_enable_interrupts();

    
//    char* buf[100];
//    float direction;
////    
    while (1) {
        set_angle(0);
        set_speed(50);
        delay(ONE_SECOND*3);
        set_angle(45);
        set_speed(50);
        delay(ONE_SECOND);
        set_angle(-45);
        set_speed(-50);
        delay(ONE_SECOND);
        set_angle(45);
        set_speed(50);
        delay(ONE_SECOND);
        set_angle(-45);
        set_speed(-50);
        delay(ONE_SECOND);
        set_angle(45);
        set_speed(50);
        delay(ONE_SECOND);
        set_angle(0);
        delay(ONE_SECOND*3);
        set_angle(-30);
        set_speed(100);
        delay(ONE_SECOND*5);
        set_speed(80);
        delay(ONE_SECOND/2);
        set_speed(50);
        delay(ONE_SECOND/2);
        set_speed(30);
        delay(ONE_SECOND/2);
        set_speed(15);
        delay(ONE_SECOND/2);
        set_speed(0);
        delay(ONE_SECOND*10);
             
//        break;
//        _CP0_SET_COUNT(0);
//        while (_CP0_GET_COUNT() < 40000000) {;}
//        set_speed(100);
    }    
}