#ifndef I2C_H
#define I2C_H

#include "sht21ctl.h"
#include <linux/i2c-dev.h>

/* function declarations */
void i2cOpen(uint8_t);
void i2cClose();
void i2cWrite (uint8_t , uint8_t*, uint32_t);
uint8_t* i2cRead (uint32_t);

#endif