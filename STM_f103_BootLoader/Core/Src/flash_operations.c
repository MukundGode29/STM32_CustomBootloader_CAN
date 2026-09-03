/*
 * flash_operations.c
 *
 *  Created on: 03-Jan-2026
 *      Author: arunrawat
 */

#include "main.h"
#include "flash_operations.h"

void flash_read_page(uint32_t page_start_addr, uint32_t *buffer)
{
    uint32_t *flash_ptr = (uint32_t *)page_start_addr;

    for (uint32_t i = 0; i < 5; i++)
    {
    	buffer[i] = flash_ptr[i];
    }
}

void flash_erase_page(uint32_t page_address)
{
    FLASH_EraseInitTypeDef erase;
    uint32_t error;

    erase.TypeErase    = FLASH_TYPEERASE_PAGES;
    erase.PageAddress  = page_address;
    erase.NbPages    = 1;

    HAL_FLASHEx_Erase(&erase, &error);
}

void flash_write_page(uint32_t page_start_addr, uint32_t *buffer)
{
    uint32_t addr = page_start_addr;

    for (uint32_t i = 0; i < 5; i++)
    {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, buffer[i]);
        addr += 4;
    }
}
