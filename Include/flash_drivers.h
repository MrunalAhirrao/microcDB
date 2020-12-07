/**
 * ******************************************************************************
 * @file            flash_drivers.h
 * @brief           This file contains the low level function prototypes for flash memory (external or internal).
 * @author          Mrunal Ahirao
 ******************************************************************************
 **************************************************************************************************************************************************
 *      NOTE: YOU SHOULD NOT EDIT THIS FILE!
 **************************************************************************************************************************************************
 **/

#include "microcDB_config.h"
#include "stm32f0xx_hal.h"

#ifndef FLASH_DRIVERS_H_
#define FLASH_DRIVERS_H_

/**
 * @brief This enum typedef will indicate different flash memory operations statuses.
 */
/*Low level statuses*/
typedef enum {
	/** This status indicates flash memory was successfully erased */
	ERASE_SUCCESS = 0,
	/** This status indicates flash memory erase failed */
	ERASE_FAILED = 1,
	/** This status indicates the data stored to flash successfully */
	FL_STORE_SUCCESS = 2,
	/** This status indicates the data storing to flash failed */
	FL_STORE_FAILED = 3,
	SHIFT_SUCCESS = 4,
	SHIFT_FAILED = 5
} flash_mem_Stat;
/*Low level statuses*/

uint32_t FlashAddresscntr;/** The flash address counter which will always point to next empty address*/

/**
 * @brief This function erases a page of the flash memory.
 * @param *AddressOfPage : The start address of the page
 * @returns  the #flash_mem_Stat #ERASE_SUCCESS or #ERASE_FAILED
 * */
flash_mem_Stat ErasePage(uint8_t *AddressOfPage);

/**
 * @brief This function will write to a page of flash memory
 * @param *ptrToEditedData : The start address of the page
 * @param *AddressOfPage : The address of the page
 * @returns  The #flash_mem_Stat #FL_STORE_SUCCESS = 2, #FL_STORE_FAILED = 3
 */
flash_mem_Stat WritePage(uint32_t *ptrToEditedData, uint32_t *AddressOfPage,
		size_t NumberOfBytes);

/**
 * @brief This function will erase the Flash between the addresses given by MICROCDB_START_ADDR and MICROCDB_END_ADDR if
 * not erased before and writes 0xDB to end address to indicate the flash is initialized for microcDB.
 * @returns the #flash_mem_Stat #ERASE_SUCCESS or #ERASE_FAILED
 * */
flash_mem_Stat EraseDB();

/**
 * @brief This function will write the data bytes to Flash memory. It accepts arguments:
 * uint8_t data1..4 which are 4 bytes as flash writing is done using a word access
 * @returns the #flash_mem_Stat #FL_STORE_SUCCESS or #FL_STORE_FAILED
 * */

flash_mem_Stat WriteToFLASH(uint8_t data1, uint8_t data2, uint8_t data3,
		uint8_t data4);

#endif /* FLASH_DRIVERS_H_ */
