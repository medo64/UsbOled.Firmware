/* Josip Medved <jmedved@jmedved.com> * www.medo64.com * MIT License */

#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include "app.h"

void i2c_master_16f_start(void) {
    SSPCON2bits.SEN = 1;      // initiate Start condition
    while (SSPCON2bits.SEN);  // wait until done
}

void i2c_master_16f_restart(void) {
    SSPCON2bits.RSEN = 1;      // initiate Repeated Start condition
    while (SSPCON2bits.RSEN);  // wait until done
}

void i2c_master_16f_stop(void) {
    SSPCON2bits.PEN = 1;      // initiate Stop condition
    while (SSPCON2bits.PEN);  // wait until done
}

void i2c_master_16f_waitIdle(void) {
    while ((SSPCON2 & 0x1F) | SSPSTATbits.R_nW);  // wait until idle
}

void i2c_master_16f_resetBus(void) {
    //reset I2C bus
    LATC0 = 0;                                // set clock low
    LATC1 = 0;                                // set data low
    TRISC0 = 1;                               // clock pin configured as output
    TRISC1 = 1;                               // data pin configured as input
    for (unsigned char j=0; j<3; j++) {       // repeat 3 times with alternating data
        for (unsigned char i=0; i<10; i++) {  // 9 cycles + 1 extra
            TRISC0 = 0; __delay_us(50);       // force clock low
            TRISC0 = 1; __delay_us(50);       // release clock high
        }
        TRISC1 = !TRISC1;                     // toggle data line
    }
}

bool i2c_master_16f_writeByte(const uint8_t value) {
    i2c_master_16f_waitIdle();
    SSPBUF = value;                             // set data
    if (SSPCON1bits.WCOL) { return false; }     // fail if there is a collision
    while (SSPSTATbits.BF);                     // wait until write is done
    return SSPCON2bits.ACKSTAT ? false : true;  // return if successful
}

bool i2c_master_16f_startWrite(const uint8_t address) {
    i2c_master_16f_waitIdle();
    i2c_master_16f_start();                                    // start operation
    return i2c_master_16f_writeByte((uint8_t)(address << 1));  // load address
}

bool i2c_master_16f_readByte(uint8_t* value) {
    if (PIR2bits.BCL1IF) { return false; }

    SSPCON2bits.RCEN = 1;                       // start receive
    while ( !SSPSTATbits.BF );                  // wait for byte

    *value = SSPBUF;                             // read byte
    if (SSPCON2bits.ACKSTAT) { return false; }  // end prematurely if there's an error

    SSPCON2bits.ACKDT = 0;                      // ACK byte
    SSPCON2bits.ACKEN = 1;                      // initiate acknowledge sequence
    while (SSPCON2bits.ACKEN);                  // wait for done

    return true;                                 // return success
}

void i2c_master_setup(const uint8_t baudRateCounter) {
    SSPCON1 = 0;  SSPCON2 = 0;  SSPSTAT = 0;  // reset all

    i2c_master_16f_resetBus();
    SSPCON1bits.SSPM = 0b1000;                // I2C master mode
    SSPCON1bits.SSPEN = 1;                    // enable I2C master mode
    SSP1STATbits.CKE = 1;                     // slew control enabled, low voltage input (SMBus) enables
    SSP1ADD = baudRateCounter;                // setup speed

    TRISC0 = 1;                               // clock pin configured as input
    TRISC1 = 1;                               // data pin configured as input}
}

#if defined(_I2C_MASTER_CUSTOM_INIT)
    void i2c_master_init(uint8_t rate) {
        if (rate < 10) {
            i2c_master_setup(_XTAL_FREQ / 4 / 100000 - 1);
        } else if (rate > 100) {
            i2c_master_setup(_XTAL_FREQ / 4 / 1000000 - 1);
        } else {
            i2c_master_setup((uint8_t)((uint32_t)_XTAL_FREQ / 4 / 10000 / rate - 1));
        }
    }
#elif defined(_I2C_MASTER_RATE_KHZ)
    void i2c_master_init(void) {
        i2c_master_setup(_XTAL_FREQ / 4 / _I2C_MASTER_RATE_KHZ / 1000 - 1);
    }
#else
    void i2c_master_init(void) {
        i2c_master_setup(_XTAL_FREQ / 4 / 100000 - 1);
    }
#endif


bool i2c_master_readRegisterBytes(const uint8_t deviceAddress, const uint8_t registerAddress, uint8_t* readData, const uint8_t readCount) {
    if (!i2c_master_16f_startWrite(deviceAddress)) { return false; }
    if (!i2c_master_16f_writeByte(registerAddress)) { return false; }

    i2c_master_16f_restart();
    for (uint8_t i = 0; i < readCount; i++) {
        if (!i2c_master_16f_readByte(readData)) { return false; }
        readData++;
    }

    i2c_master_16f_stop();
    return true;
}


bool i2c_master_writeRegisterBytes(const uint8_t deviceAddress, const uint8_t registerAddress, const uint8_t* data, const uint8_t count) {
    if (!i2c_master_16f_startWrite(deviceAddress)) { return false; }

    if (!i2c_master_16f_writeByte(registerAddress)) { return false; }
    for (uint8_t i = 0; i < count; i++) {
        if (!i2c_master_16f_writeByte(*data)) { return false; }
        data++;
    }

    i2c_master_16f_stop();
    return true;
}

bool i2c_master_writeRegisterZeroBytes(const uint8_t deviceAddress, const uint8_t registerAddress, const uint8_t zeroCount) {
    if (!i2c_master_16f_startWrite(deviceAddress)) { return false; }

    if (!i2c_master_16f_writeByte(registerAddress)) { return false; }
    for (uint8_t i = 0; i < zeroCount; i++) {
        if (!i2c_master_16f_writeByte(0)) { return false; }
    }

    i2c_master_16f_stop();
    return true;
}


bool i2c_master_writeBytes(const uint8_t deviceAddress, const uint8_t* data, const uint8_t count) {
    return i2c_master_writeRegisterBytes(deviceAddress, *data, data + 1, count - 1);
}

bool i2c_master_writeZeroBytes(const uint8_t deviceAddress, const uint8_t zeroCount) {
    return i2c_master_writeRegisterZeroBytes(deviceAddress, 0, zeroCount - 1);
}
