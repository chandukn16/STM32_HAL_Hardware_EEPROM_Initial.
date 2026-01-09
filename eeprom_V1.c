#include <eeprom_V1.h>
#include "main.h"
#include <string.h>

// External I2C handle (must be initialized in main.c)
static I2C_HandleTypeDef *hi2c_eeprom = NULL;

/**
  * @brief  Initialize EEPROM interface
  * @param  hi2c: Pointer to I2C handle
  * @retval EEPROM status
  */
EEPROM_Status EEPROM_Init(I2C_HandleTypeDef *hi2c)
{
    if (hi2c == NULL)
    {
        return EEPROM_ERROR;
    }

    hi2c_eeprom = hi2c;

    // Optional: Test communication by reading a byte
    uint8_t dummy;
    if (HAL_I2C_Mem_Read(hi2c_eeprom, EEPROM_I2C_ADDR, 0x0000,
                         I2C_MEMADD_SIZE_8BIT, &dummy, 1, 100) != HAL_OK)
    {
        return EEPROM_ERROR;
    }

    return EEPROM_OK;
}

/**
  * @brief  Get block number from address
  * @param  address: Memory address (0-2047)
  * @retval Block number (0-7)
  */
uint8_t EEPROM_GetBlockFromAddress(uint16_t address)
{
    return (address >> 8) & 0x07;  // Bits 8-10 determine the block
}

/**
  * @brief  Get I2C address for specific memory address
  * @param  address: Memory address (0-2047)
  * @retval I2C device address
  */
uint8_t EEPROM_GetI2CAddress(uint16_t address)
{
    uint8_t block = EEPROM_GetBlockFromAddress(address);
    // For 24LC16B: Base address (0xA0) + block number in bits 1-3
    return (EEPROM_I2C_ADDR | (block << 1));
}

/**
  * @brief  Get address within block
  * @param  address: Memory address (0-2047)
  * @retval Address within block (0-255)
  */
uint16_t EEPROM_GetAddressInBlock(uint16_t address)
{
    return address & 0xFF;  // Lower 8 bits
}

/**
  * @brief  Write single byte to EEPROM
  * @param  address: Memory address (0-2047)
  * @param  data: Data byte to write
  * @retval EEPROM status
  */
EEPROM_Status EEPROM_WriteByte(uint16_t address, uint8_t data)
{
    if (hi2c_eeprom == NULL || address >= EEPROM_TOTAL_SIZE)
    {
        return EEPROM_ERROR;
    }

    uint8_t i2c_addr = EEPROM_GetI2CAddress(address);
    uint8_t mem_addr = EEPROM_GetAddressInBlock(address);

    if (HAL_I2C_Mem_Write(hi2c_eeprom, i2c_addr, mem_addr,
                          I2C_MEMADD_SIZE_8BIT, &data, 1, 100) != HAL_OK)
    {
        return EEPROM_ERROR;
    }

    // Wait for write cycle to complete
    HAL_Delay(EEPROM_WRITE_DELAY);

    return EEPROM_OK;
}

/**
  * @brief  Read single byte from EEPROM
  * @param  address: Memory address (0-2047)
  * @param  data: Pointer to store read data
  * @retval EEPROM status
  */
EEPROM_Status EEPROM_ReadByte(uint16_t address, uint8_t *data)
{
    if (hi2c_eeprom == NULL || data == NULL || address >= EEPROM_TOTAL_SIZE)
    {
        return EEPROM_ERROR;
    }

    uint8_t i2c_addr = EEPROM_GetI2CAddress(address);
    uint8_t mem_addr = EEPROM_GetAddressInBlock(address);

    if (HAL_I2C_Mem_Read(hi2c_eeprom, i2c_addr, mem_addr,
                         I2C_MEMADD_SIZE_8BIT, data, 1, 100) != HAL_OK)
    {
        return EEPROM_ERROR;
    }

    return EEPROM_OK;
}

/**
  * @brief  Write a page of data (max 16 bytes)
  * @param  address: Starting memory address (must be page-aligned)
  * @param  data: Pointer to data buffer
  * @param  size: Number of bytes to write (max 16)
  * @retval EEPROM status
  */
EEPROM_Status EEPROM_WritePage(uint16_t address, uint8_t *data, uint16_t size)
{
    if (hi2c_eeprom == NULL || data == NULL || size == 0 || size > EEPROM_PAGE_SIZE)
    {
        return EEPROM_ERROR;
    }

    // Check if address is within bounds and doesn't cross page boundary
    if (address >= EEPROM_TOTAL_SIZE ||
        (address % EEPROM_PAGE_SIZE) + size > EEPROM_PAGE_SIZE)
    {
        return EEPROM_ERROR;
    }

    uint8_t i2c_addr = EEPROM_GetI2CAddress(address);
    uint8_t mem_addr = EEPROM_GetAddressInBlock(address);

    if (HAL_I2C_Mem_Write(hi2c_eeprom, i2c_addr, mem_addr,
                          I2C_MEMADD_SIZE_8BIT, data, size, 100) != HAL_OK)
    {
        return EEPROM_ERROR;
    }

    // Wait for write cycle to complete
    HAL_Delay(EEPROM_WRITE_DELAY);

    return EEPROM_OK;
}

/**
  * @brief  Read buffer from EEPROM (can cross page boundaries)
  * @param  address: Starting memory address
  * @param  data: Pointer to data buffer
  * @param  size: Number of bytes to read
  * @retval EEPROM status
  */
EEPROM_Status EEPROM_ReadBuffer(uint16_t address, uint8_t *data, uint16_t size)
{
    if (hi2c_eeprom == NULL || data == NULL || size == 0)
    {
        return EEPROM_ERROR;
    }

    // Check if address range is valid
    if (address + size > EEPROM_TOTAL_SIZE)
    {
        return EEPROM_ERROR;
    }

    uint16_t bytes_read = 0;

    while (bytes_read < size)
    {
        uint16_t current_addr = address + bytes_read;
        uint8_t i2c_addr = EEPROM_GetI2CAddress(current_addr);
        uint8_t mem_addr = EEPROM_GetAddressInBlock(current_addr);

        // Calculate how many bytes we can read in this block
        uint16_t block_start = (current_addr & 0xFF00);
        uint16_t block_end = block_start + 0xFF;
        uint16_t remaining_in_block = block_end - current_addr + 1;
        uint16_t remaining_to_read = size - bytes_read;
        uint16_t read_size = (remaining_in_block < remaining_to_read) ?
                             remaining_in_block : remaining_to_read;

        // Limit read to I2C buffer size if needed
        if (read_size > 256) read_size = 256;

        if (HAL_I2C_Mem_Read(hi2c_eeprom, i2c_addr, mem_addr,
                             I2C_MEMADD_SIZE_8BIT, &data[bytes_read],
                             read_size, 100) != HAL_OK)
        {
            return EEPROM_ERROR;
        }

        bytes_read += read_size;
    }

    return EEPROM_OK;
}

/**
  * @brief  Write buffer to EEPROM (handles page boundaries)
  * @param  address: Starting memory address
  * @param  data: Pointer to data buffer
  * @param  size: Number of bytes to write
  * @retval EEPROM status
  */
EEPROM_Status EEPROM_WriteBuffer(uint16_t address, uint8_t *data, uint16_t size)
{
    if (hi2c_eeprom == NULL || data == NULL || size == 0)
    {
        return EEPROM_ERROR;
    }

    // Check if address range is valid
    if (address + size > EEPROM_TOTAL_SIZE)
    {
        return EEPROM_ERROR;
    }

    uint16_t bytes_written = 0;

    while (bytes_written < size)
    {
        uint16_t current_addr = address + bytes_written;
        uint16_t page_offset = current_addr % EEPROM_PAGE_SIZE;
        uint16_t remaining_in_page = EEPROM_PAGE_SIZE - page_offset;
        uint16_t remaining_to_write = size - bytes_written;
        uint16_t write_size = (remaining_in_page < remaining_to_write) ?
                              remaining_in_page : remaining_to_write;

        // Write one page at a time
        if (EEPROM_WritePage(current_addr, &data[bytes_written], write_size) != EEPROM_OK)
        {
            return EEPROM_ERROR;
        }

        bytes_written += write_size;
    }

    return EEPROM_OK;
}

/**
  * @brief  Clear EEPROM memory region
  * @param  start_address: Start address
  * @param  end_address: End address
  * @param  value: Value to write (usually 0xFF for cleared EEPROM)
  * @retval EEPROM status
  */
EEPROM_Status EEPROM_Clear(uint16_t start_address, uint16_t end_address, uint8_t value)
{
    if (start_address > end_address || end_address >= EEPROM_TOTAL_SIZE)
    {
        return EEPROM_ERROR;
    }

    uint16_t size = end_address - start_address + 1;
    uint8_t *buffer = (uint8_t*)malloc(size);

    if (buffer == NULL)
    {
        return EEPROM_ERROR;
    }

    // Fill buffer with clear value
    memset(buffer, value, size);

    // Write buffer
    EEPROM_Status status = EEPROM_WriteBuffer(start_address, buffer, size);

    free(buffer);
    return status;
}

/**
  * @brief  Test EEPROM functionality
  * @retval EEPROM status
  */
EEPROM_Status EEPROM_Test(void)
{
    uint8_t write_data[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    uint8_t read_data[6];
    uint16_t test_address = 0x0100;  // Test in block 1

    // Test 1: Write and read single byte
    if (EEPROM_WriteByte(test_address, 0x55) != EEPROM_OK)
    {
        return EEPROM_ERROR;
    }

    uint8_t single_byte;
    if (EEPROM_ReadByte(test_address, &single_byte) != EEPROM_OK)
    {
        return EEPROM_ERROR;
    }

    if (single_byte != 0x55)
    {
        return EEPROM_ERROR;
    }

    // Test 2: Write and read page
    if (EEPROM_WritePage(test_address + 10, write_data, sizeof(write_data)) != EEPROM_OK)
    {
        return EEPROM_ERROR;
    }

    if (EEPROM_ReadBuffer(test_address + 10, read_data, sizeof(read_data)) != EEPROM_OK)
    {
        return EEPROM_ERROR;
    }

    // Verify data
    for (int i = 0; i < sizeof(write_data); i++)
    {
        if (read_data[i] != write_data[i])
        {
            return EEPROM_ERROR;
        }
    }

    return EEPROM_OK;
}


/**
  * @brief  Test function for ADC voltage saving/retrieval
  * @param  hadc: ADC handle pointer
  * @param  huart: UART handle pointer for debug output
  * @retval None
  */
