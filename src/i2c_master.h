#ifndef I2C_MASTER_H
#define	I2C_MASTER_H

#include <stdbool.h>
#include <stdint.h>


/** Initializes I2C as a master. */
void i2c_master_init(const uint16_t speedKHz);


/** Starts a read operation. */
bool i2c_master_startRead(const uint8_t address);

/** Reads single byte. */
bool i2c_master_readByte(uint8_t* value);

/** Reads multiple bytes. */
bool i2c_slave_readBytes(uint8_t* value, const uint8_t count);


/** Starts a writing operation. */
bool i2c_master_startWrite(const uint8_t address);

/** Writes single byte. */
bool i2c_master_writeByte(const uint8_t value);


/** Sends a start command. */
void i2c_master_start();

/** Sends a stop command. */
void i2c_master_stop();

/** Sends a restart command. */
void i2c_master_restart();

#endif
