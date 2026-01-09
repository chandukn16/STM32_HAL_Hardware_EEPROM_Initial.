#ifndef EEPROM_24LC16B_H
#define EEPROM_24LC16B_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

// EEPROM 24LC16B Parameters
#define EEPROM_I2C_ADDR          0xA0    // 1010 + A2 A1 A0 = 0 + Write bit
#define EEPROM_PAGE_SIZE         16      // Bytes per page
#define EEPROM_TOTAL_SIZE        2048    // 16Kbits = 2048 bytes
#define EEPROM_WRITE_DELAY       5       // ms delay for write cycle

// EEPROM memory block structure (8 blocks of 256 bytes each)
typedef enum {
    EEPROM_BLOCK_0 = 0x00,  // Addresses 0x000-0x0FF
    EEPROM_BLOCK_1 = 0x01,  // Addresses 0x100-0x1FF
    EEPROM_BLOCK_2 = 0x02,  // Addresses 0x200-0x2FF
    EEPROM_BLOCK_3 = 0x03,  // Addresses 0x300-0x3FF
    EEPROM_BLOCK_4 = 0x04,  // Addresses 0x400-0x4FF
    EEPROM_BLOCK_5 = 0x05,  // Addresses 0x500-0x5FF
    EEPROM_BLOCK_6 = 0x06,  // Addresses 0x600-0x6FF
    EEPROM_BLOCK_7 = 0x07   // Addresses 0x700-0x7FF
} EEPROM_Block;

// Status enumeration
typedef enum {
    EEPROM_OK       = 0x00,
    EEPROM_ERROR    = 0x01,
    EEPROM_BUSY     = 0x02,
    EEPROM_TIMEOUT  = 0x03
} EEPROM_Status;

// Function prototypes
EEPROM_Status EEPROM_Init(I2C_HandleTypeDef *hi2c);
EEPROM_Status EEPROM_WriteByte(uint16_t address, uint8_t data);
EEPROM_Status EEPROM_ReadByte(uint16_t address, uint8_t *data);
EEPROM_Status EEPROM_WritePage(uint16_t address, uint8_t *data, uint16_t size);
EEPROM_Status EEPROM_ReadBuffer(uint16_t address, uint8_t *data, uint16_t size);
EEPROM_Status EEPROM_WriteBuffer(uint16_t address, uint8_t *data, uint16_t size);
EEPROM_Status EEPROM_Clear(uint16_t start_address, uint16_t end_address, uint8_t value);
EEPROM_Status EEPROM_Test(void);
//EEPROM_Status EEPROM_ADC_Voltage_Test(void);
// Utility functions
uint8_t EEPROM_GetBlockFromAddress(uint16_t address);
uint8_t EEPROM_GetI2CAddress(uint16_t address);
uint16_t EEPROM_GetAddressInBlock(uint16_t address);

#ifdef __cplusplus
}
#endif

#endif /* EEPROM_24LC16B_H */
