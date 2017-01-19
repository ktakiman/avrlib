#ifndef __I2C_H
#define __I2C_H

// ----------------------------------------------------------------------------
// Requires pull-up registor on both SDA and SCL lines. Internal pull-up resitor is NOT used
// ----------------------------------------------------------------------------

#define EEPROM_CMD_WRITE 0b10100000 // 1010 ~ EEPROM's control code, 000 ~ chip select (status of A1, A2, A3 pins)
#define EEPROM_CMD_READ  0b10100001
#define EEPROM_PAGE_SIZE 64

// ----------------------------------------------------------------------------
struct I2C
{
    volatile uint8_t* pDDR;
    volatile uint8_t* pPort;
    volatile uint8_t* pPin;
    uint8_t sda;
    uint8_t scl;
    void (*pDumpCallback)(uint8_t*);
};

//typedef struct _I2C I2C;

// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Generic I2C functions
// ----------------------------------------------------------------------------
void _i2c_init(struct I2C* pI2C);
void _i2c_start(struct I2C* pI2C);
void _i2c_stop(struct I2C* pI2C);
uint8_t _i2c_sendByte(struct I2C* pI2C, uint8_t byte);
void _i2c_readByte(struct I2C* pI2C, uint8_t* pByte, uint8_t ack);

// ----------------------------------------------------------------------------
// EEPROM specific functions
// ----------------------------------------------------------------------------
void _eeprom_write(struct I2C* pI2C, uint16_t addr, uint8_t* pData, uint8_t startPos, uint8_t len, uint8_t commit);
void _eeprom_read(struct I2C* pI2C, uint16_t addr, uint8_t* pBuf, uint8_t startPos, uint8_t len);

#endif