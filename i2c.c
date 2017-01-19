#include <util/delay.h>

#include "i2c.h"

#define TWI_DELAY _delay_us(20);
//#define TWI_DELAY _delay_ms(500);
//#define TWI_DELAY _delay_ms(100);

#define SCL_HIGH(pI2C) (*(pI2C)->pDDR) &= ~(1 << pI2C->scl);
#define SCL_LOW(pI2C) (*(pI2C)->pDDR) |= (1 << pI2C->scl);

#define SDA_HIGH(pI2C) (*(pI2C)->pDDR) &= ~(1 << pI2C->sda);
#define SDA_LOW(pI2C) (*(pI2C)->pDDR) |= (1 << pI2C->sda);


// ----------------------------------------------------------------------------
// Helper functions
// ----------------------------------------------------------------------------
void _i2c_high(volatile uint8_t* pDDR, uint8_t pin)
{
    *pDDR &= ~(1 << pin); // let the line go (make as inpuer), pull-up will make this high
}
// ----------------------------------------------------------------------------
void _i2c_low(volatile uint8_t* pDDR, uint8_t pin)
{
    *pDDR |= (1 << pin); // make this line as output, pin should stay as low 
}
// ----------------------------------------------------------------------------
void _i2c_dump(struct I2C* pI2C, uint8_t* pError)
{
    if (pI2C->pDumpCallback)
    {
        pI2C->pDumpCallback(pError);
    }
}

// ----------------------------------------------------------------------------
// Generic I2C functions
// ----------------------------------------------------------------------------
void _i2c_init(struct I2C* pI2C)
{
    *(pI2C->pDDR) &= ~(1 << pI2C->sda | 1 << pI2C->scl); // make both pins as input, external pull-up should bring these high
    
    // No need to mess with PORTX during I2C operations because there are only two states to support and both require PORT to be low.
    //   When pin is high => pin is set as input (no-internal pull-up used)
    //   When pin is low  => pin is set as output and low state
    *(pI2C->pPort) &= ~(1 << pI2C->sda | 1 << pI2C->scl);     
}
// ----------------------------------------------------------------------------
void _i2c_start(struct I2C* pI2C)
{
    // START signal - bring SDA low while SCL stays high
    SDA_LOW(pI2C)
    
    TWI_DELAY
}
// ----------------------------------------------------------------------------
void _i2c_stop(struct I2C* pI2C)
{
    // Need to make sure SDA is low.  Change SDA only while SCL is low
    SCL_LOW(pI2C)
    SDA_LOW(pI2C)
    TWI_DELAY

    SCL_HIGH(pI2C)
    TWI_DELAY

    // STOP signal - bring SDA high while SCL stays high
    SDA_HIGH(pI2C)
    TWI_DELAY
}
// ----------------------------------------------------------------------------
uint8_t _i2c_sendByte(struct I2C* pI2C, uint8_t byte)
{
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        SCL_LOW(pI2C)
        
        if (byte & (1 << 7 - i))
        {
            SDA_HIGH(pI2C)
        }
        else
        {
            SDA_LOW(pI2C)
        }
    
        TWI_DELAY
        
        SCL_HIGH(pI2C)
        TWI_DELAY
    }

    SCL_LOW(pI2C)
    SDA_HIGH(pI2C) // let SDA go high
    TWI_DELAY
    
    SCL_HIGH(pI2C)
    TWI_DELAY

    // Check ACK
    uint8_t ack = !(*(pI2C->pPin) & (1 << pI2C->sda));
    TWI_DELAY

    return ack;
}
// ----------------------------------------------------------------------------
void _i2c_readByte(struct I2C* pI2C, uint8_t* pByte, uint8_t ack)
{
    uint8_t i;

    for (i = 0; i < 8; i++)
    {
        SCL_LOW(pI2C)
        SDA_HIGH(pI2C) // let SDA go high
        TWI_DELAY

        SCL_HIGH(pI2C)
        
        // read SDA
        if (*(pI2C->pPin) & (1 << pI2C->sda))
        {
            *pByte |= (1 << 7 - i);
        }
        else
        {
            *pByte &= ~(1 << 7 - i);
        }
        
        TWI_DELAY
    }
    
    SCL_LOW(pI2C)
    if (ack)
    {
        SDA_LOW(pI2C)
    }
    
    TWI_DELAY

    SCL_HIGH(pI2C)
    TWI_DELAY
}

// ----------------------------------------------------------------------------
// EEPROM specific functions
// ----------------------------------------------------------------------------
void _eeprom_write(struct I2C* pI2C, uint16_t addr, uint8_t* pData, uint8_t startPos, uint8_t len, uint8_t commit)
{
    _i2c_start(pI2C);
    
    if (_i2c_sendByte(pI2C, EEPROM_CMD_WRITE))
    {
        if (_i2c_sendByte(pI2C, (uint8_t)(addr >> 8)))
        {
            if (_i2c_sendByte(pI2C, (uint8_t)addr))
            {
                uint8_t i;
                for (i = 0; i < len; i++)
                {
                    if (_i2c_sendByte(pI2C, pData[startPos + i]))
                    {
                    }
                    else 
                    {
                        //_i2c_dump(pI2C, "Data Write Fail\0");
                        break;
                    }
                }
                
                _i2c_stop(pI2C);
                
                while (1)
                {
                    _delay_us(10);

                    _i2c_start(pI2C);
                
                    if (_i2c_sendByte(pI2C, EEPROM_CMD_WRITE))
                    {
                        break;
                    }
                    //else { _i2c_dump(pI2C, "Waiting\0"); }
                }

                _i2c_stop(pI2C);
                
                //_i2c_dump(pI2C, "Done\0");
            }
            //else { _i2c_dump(pI2C, "Write High Addr Fail\0"); }
        }
        //else { _i2c_dump(pI2C, "Write Low Addr Fail\0"); }
    }
    //else { _i2c_dump(pI2C, "Write Cmd Fail\0"); }            
}
// ----------------------------------------------------------------------------
void _eeprom_read(struct I2C* pI2C, uint16_t addr, uint8_t* pBuf, uint8_t startPos, uint8_t len)
{
    _i2c_start(pI2C);
    
    if (_i2c_sendByte(pI2C, EEPROM_CMD_WRITE))
    {
        if (_i2c_sendByte(pI2C, (uint8_t)(addr >> 8)))
        {
            if (_i2c_sendByte(pI2C, (uint8_t)addr))
            {
                _i2c_stop(pI2C);
            
                _i2c_start(pI2C);
    
                if (_i2c_sendByte(pI2C, EEPROM_CMD_READ))
                {
                    uint8_t i;
                    
                    for (i = 0; i < len; i++)
                    {
                        _i2c_readByte(pI2C, pBuf + startPos + i, i < len - 1);
                        
                        TWI_DELAY
                    }
                    
                    _i2c_stop(pI2C);
                }
            }
            //else { _i2c_dump(pI2C, "Write High Addr Fail\0"); }
        }
        //else { _i2c_dump(pI2C, "Write Low Addr Fail\0"); }
    }
    //else { _i2c_dump(pI2C, "Write Cmd Fail\0"); }       
}