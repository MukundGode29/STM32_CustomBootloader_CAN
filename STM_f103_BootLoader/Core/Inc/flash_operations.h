/*
 * flash_operations.h
 *
 *  Created on: 03-Jan-2026
 *      Author: arunrawat
 */

#ifndef INC_FLASH_OPERATIONS_H_
#define INC_FLASH_OPERATIONS_H_


void flash_read_page(uint32_t page_start_addr, uint32_t *buffer);
void flash_erase_page(uint32_t page_address);
void flash_write_page(uint32_t page_start_addr, uint32_t *buffer);



#endif /* INC_FLASH_OPERATIONS_H_ */
