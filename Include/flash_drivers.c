/*
 * flash_drivers.c
 *
 *  Created on: Aug 18, 2019
 *  Author: Mrunal Ahirao, LSK Technologies
 *  Description: This file should have the low level functions to interface with Flash memory (internal or external) which will be used by
 *  			 microcDB. You can add your own function implementation if your flash memory is different than this but the ultimate result
 *  			 should be as described in comments of function.
 */

#include "flash_drivers.h"

/*Flash related struct for STM32*/
static FLASH_EraseInitTypeDef EraseInitStruct;
static uint32_t PageError = 0;

/**************************************************************************************************************************************/
/*MicrocDB Low level functions*/


/*
 * This function erases a page of the flash memory.
 * Arguments: The start address of the page
 * Returns: the flash_mem_Stat ERASE_SUCCESS or ERASE_FAILED
 * */
inline flash_mem_Stat ErasePage(uint8_t *AddressOfPage) {

	PageError = 0;

	HAL_FLASH_Unlock();

	/* Fill EraseInit structure*/
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
	EraseInitStruct.PageAddress = (uint32_t) AddressOfPage;
	EraseInitStruct.NbPages = 1;

	if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) == HAL_OK) {
		HAL_FLASH_Lock();
		return ERASE_SUCCESS;
	} else {
		HAL_FLASH_Lock();
		return ERASE_FAILED;
	}
}

/*
 * This function will write to a page of flash memory
 * Argument: uint32_t *ptrToEditedData :The start address of the page
 * 			 uint32_t *AddressOfPage: The address of the page
 * Returns: The flash_mem_Stat FL_STORE_SUCCESS = 2,
 FL_STORE_FAILED = 3
 */
inline flash_mem_Stat WritePage(uint32_t *ptrToEditedData,
		uint32_t *AddressOfPage, size_t NumberOfBytes) {

	HAL_FLASH_Unlock();

	uint32_t storeddata;
	uint16_t bytecntr = 0;
	while (bytecntr < NumberOfBytes) {

		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, (uint32_t) AddressOfPage,
				*ptrToEditedData) == HAL_OK) {

			storeddata = *(uint32_t*) AddressOfPage;
			/*Verify if stored correctly*/
			if (storeddata == *ptrToEditedData) {
				AddressOfPage++;
				bytecntr = bytecntr + 4;
				ptrToEditedData++;
			}
		}
	};

	HAL_FLASH_Lock();

	if (bytecntr >= PAGE_SIZE) {
		return FL_STORE_SUCCESS;
	} else
		return FL_STORE_FAILED;
}

/*
 * This function will erase the Flash between the addresses given by MICROCDB_START_ADDR and MICROCDB_END_ADDR if
 * not erased before and writes 0xDB to end address to indicate the flash is initialized for microcDB.
 * Returns: the flash_mem_Stat ERASE_SUCCESS or ERASE_FAILED
 * */
flash_mem_Stat EraseDB() {

	uint8_t *i;
	i = (uint8_t*) MICROCDB_START_ADDR; // Assign the start address of the database to the pointer
	PageError = 0;

	HAL_FLASH_Unlock();

	/* Fill EraseInit structure*/
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
	EraseInitStruct.PageAddress = MICROCDB_START_ADDR;
	EraseInitStruct.NbPages = (MICROCDB_END_ADDR - MICROCDB_START_ADDR)
			/ PAGE_SIZE;
	EraseInitStruct.NbPages++;
	HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);

	/*Write the flag 0xDB to last byte to indicate that memory is initialized for microcDB*/
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, (uint32_t) MICROCDB_END_ADDR,
			0xDB);

	HAL_FLASH_Lock();
	uint16_t emp_cntr = 0;

	/*Check if the pages in database are erased successfully*/
	while (i < (uint8_t*) MICROCDB_END_ADDR) {
		if (*i == FL_EMPTY_BYTE) {
			emp_cntr++; //increment the counter for every empty byte
		} else {
			emp_cntr = emp_cntr;
		}
		i++;
	}

	/*The emp_cntr should be equal to MAX bytes as byte by byte checking is done for empty memory area */
	if (((uint16_t) MAX_DB_SIZE - emp_cntr) <= 2) {
		FlashAddresscntr = MICROCDB_START_ADDR; // Assign the Start of DB address to counter as this is the first time the database will will be initialized.
		return ERASE_SUCCESS;
	} else
		return ERASE_FAILED;
}

/*
 * This function will write the data bytes to Flash memory. It accepts arguments:
 * uint8_t data1..4 which are 4 bytes as flash writing is done using a word access
 * Returns: the flash_mem_Stat FL_STORE_SUCCESS or FL_STORE_FAILED
 * */

flash_mem_Stat WriteToFLASH(uint8_t data1, uint8_t data2, uint8_t data3,
		uint8_t data4) {

	HAL_FLASH_Unlock();

	if (data3 != 0) { /*If full 32 bit data is given to write then proceed with word write*/
		uint32_t u32data_to_store, storeddata;

		u32data_to_store = data4 << 24;
		u32data_to_store |= data3 << 16;
		u32data_to_store |= data2 << 8;
		u32data_to_store |= data1;

		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FlashAddresscntr,
				u32data_to_store) == HAL_OK) {

			storeddata = *(uint32_t*) FlashAddresscntr;
			/*Verify if stored correctly*/
			if (storeddata == u32data_to_store) {
				FlashAddresscntr = FlashAddresscntr + 4;

				HAL_FLASH_Lock();
				return FL_STORE_SUCCESS;
			} else
				return FL_STORE_FAILED;
		} else
			return FL_STORE_FAILED;
	} else {
		/*Proceed with Half word write i.e 16-bit data. This is to save memory from storing
		 Null uint8_ts.As sometimes the string may not be 32 bit aligned so storing such string to flash
		 with word write caused null uint8_ts to be stored to flash now using this only 2 null uint8_ts
		 can be written. It can be avoided fully if the flash has BYTE WRITE procedure, but
		 very few have this.*/

		uint16_t datatostore, storedata;
		datatostore = data2 << 8;
		datatostore |= data1;
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, FlashAddresscntr,
				datatostore) == HAL_OK) {

			storedata = *(uint16_t*) FlashAddresscntr;
			/*Verify if stored correctly*/
			if (storedata == datatostore) {
				FlashAddresscntr = FlashAddresscntr + 2;

				HAL_FLASH_Lock();
				return FL_STORE_SUCCESS;
			} else
				return FL_STORE_FAILED;
		} else
			return FL_STORE_FAILED;
	}
}

/*MicrocDB low level functions*/
/*****************************************************************************************************************************************************************/
