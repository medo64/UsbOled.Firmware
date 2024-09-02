#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include "i2c_master.h"

#define READ  1
#define WRITE 0
#define ACK   0
#define NACK  1


void waitIdle() {
    while ((SSPCON2 & 0x1F) | SSPSTATbits.R_nW);  // wait until idle    
}

void i2c_master_init(const uint8_t baudRateCounter) {
    SSPCON1 = 0;  SSPCON2 = 0;  SSPSTAT = 0;  // reset all

    SSPCON1bits.SSPM = 0b1000;                // I2C master mode
    SSPCON1bits.SSPEN = 1;                    // enable I2C master mode
    SSP1STATbits.CKE = 1;                     // slew control enabled, low voltage input (SMBus) enables
    SSP1ADD = baudRateCounter;                // setup speed

    TRISC0 = 1;                               // clock pin configured as input
    TRISC1 = 1;                               // data pin configured as input}
}

bool i2c_master_startRead(const uint8_t address) {
    i2c_master_start();                                  // Start
    return i2c_master_writeByte((uint8_t)(address << 1) | READ);  // load address
}

bool i2c_master_readByte(uint8_t* value) {
    if (PIR2bits.BCL1IF) { return false; }

    SSPCON2bits.RCEN = 1;                       // start receive
    while ( !SSPSTATbits.BF );                  // wait for byte

    *value = SSPBUF;                             // read byte
    if (SSPCON2bits.ACKSTAT) { return false; }  // end prematurely if there's an error

    SSPCON2bits.ACKDT = ACK;                    // ACK for the last byte
    SSPCON2bits.ACKEN = 1;                      // initiate acknowledge sequence
    while (SSPCON2bits.ACKEN);                  // wait for done

    return true;                                 // return success
}

bool i2c_slave_readBytes(uint8_t* value, const uint8_t count) {
    for (char i=0; i<count; i++) {
        SSPCON2bits.RCEN = 1;                       // start receive
        while ( !SSPSTATbits.BF );                  // wait for byte

        *value = SSPBUF;                             // read byte
        if (SSPCON2bits.ACKSTAT) { return false; }  // end prematurely if there's an error

        value++;

        ACKDT = (i < count-1) ? ACK : NACK;          // NACK for last byte
        SSPCON2bits.ACKEN = 1;                      // initiate acknowledge sequence
        while (SSPCON2bits.ACKEN);                  // wait for done
    }
    return true;
}


bool i2c_master_startWrite(const uint8_t address) {
    waitIdle();
    i2c_master_start();                                   // start operation
    return i2c_master_writeByte((uint8_t)(address << 1) | WRITE);  // load address
}

bool i2c_master_writeByte(const uint8_t value) {
    waitIdle();
    SSPBUF = value;                                  // set data
    if (SSPCON1bits.WCOL) { return false; }          // fail if there is a collision
    while (SSPSTATbits.BF);                          // wait until write is done
    return SSPCON2bits.ACKSTAT ? false : true;       // return if successful
}


void i2c_master_start(void) {
    SSPCON2bits.SEN = 1;      // initiate Start condition
    while (SSPCON2bits.SEN);  // wait until done
}

void i2c_master_stop(void) {
    SSPCON2bits.PEN = 1;      // initiate Stop condition
    while (SSPCON2bits.PEN);  // wait until done
}

void i2c_master_restart(void) {
    SSPCON2bits.RSEN = 1;      // initiate Repeated Start condition
    while (SSPCON2bits.RSEN);  // wait until done
}
