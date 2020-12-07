 */
/*
 * 		Author: Mrunal Ahirao
 *      Description: The source code for MicrocDB.
 *      NOTE: DON'T REFORMAT CODE!
 * */

#include <microcDB_internal.h>
#include <stdbool.h>

/*MISC functions*/

/*
 * This function Copies The flash data to RAM of a Page 32 bit word by word. This is used only when updating the MicrocDB
 * Returns: Number of copied bytes
 * Note: This is inline function
 */
static inline uint16_t CopyFlashToRAM(uint32_t *RAMaddress,
		uint32_t *FlashAddress) {
	uint16_t counter = 0;
	while (counter != FLASH_PAGE_SIZE) {
		*RAMaddress = *FlashAddress;
		RAMaddress++;
		FlashAddress++;
		counter = counter + 4;
	};
	return counter;
}

/*
 * This function Copies The number of given flash data bytes to RAM of a Page byte by byte. This is used only when updating the MicrocDB
 * Returns: Number of copied bytes
 * Note: This is inline function
 */
static inline uint16_t CopyFlashBytesToRAM(uint8_t *RAMaddress,
		uint8_t *FlashAddress, uint16_t NumberofBytes) {
	uint16_t counter = 0;
	while (counter < NumberofBytes) {
		*RAMaddress = *FlashAddress;
		RAMaddress++;
		FlashAddress++;
		counter++;
	};
	return counter;
}

/*
 * This function will return the page number of the flash memory in which the address pointed by pointer lies.
 */
static inline uint16_t CalculateFlashPageNum(uint32_t *ptrToData) {
	/*
	 * Logic of calculation:
	 * 1.Count bytes from MICROCDB_START_ADDR till the address of the ptrToData. While counting check if the count is equal to
	 * the page size if it is then increment the pagecounter.
	 * 2.And then after reaching the address of the ptrToData the latest pagecounter will be the needed page.
	 */
	uint16_t pagecntr = 0, bytecntr = 0;
	uint32_t *ptr = (uint32_t*) MICROCDB_START_ADDR; /*Set the pointer to point to MICROCDB_START_ADDR*/

	while (ptr < ptrToData) {
		if (bytecntr == FLASH_PAGE_SIZE) {
			pagecntr++; /*Increment the page counter*/
			bytecntr = 0; /*Reset the bytecntr to again start*/
		}
		bytecntr = bytecntr + 4; /*Increment the bytecntr on every increment of the pointer*/
		ptr++; /*Increment the pointer which started with the MICROCDB_START_ADDR*/
	};
	return pagecntr;
}

/*
 * This function calculates the length of string by searching the first occurrence of slash character.
 * Returns the length of string with size_t struct.
 * */
static inline size_t CalculateStringLength(uint8_t *string) {
	size_t len = 0;
	/*loop while the slash(/)*/
	while (*string != '/') {
		string++;
		len++;
	}
	return len;
}

/*
 * This function finds and returns the index of '.'
 * */
static inline uint16_t getindexofDot(uint8_t *string) {
	uint16_t i = 0;
	while (*string != '.') {
		i++;
		string++;
	}
	return i;
}

/*
 * This function calculates the parts of the query string.
 * As MicrocDB needs the query to be like "users.name.Jack/"
 * Each string part separated by dot is part. So this function
 * returns the Total number of parts.
 */
static inline uint16_t CalculateTotalNumberOfParts(uint8_t *query) {
	uint16_t parts = 0, dots = 0;
	while (*query != '/') {
		dots = getindexofDot(query);
		if (dots) {
			query = query + dots + 1; /*Increment the query by each part*/
			parts++;
		}
	}
	return parts;
}

/*
 * This function will replace the ' with " in a JSON String
 * As the JSMN parser needs the "" to recognize a string but in C to use strings "" is used
 * and for JSON the string fields needs to be replaced with escape \". To avoid using
 * escape chars, the microcDB uses the single quote string fields in JSON which this function
 * converts to double quoted when giving to parser.
 * */
static inline void replacesingleTodouble(uint8_t *JSON_String, size_t len) {
	uint16_t counter = 0;

	while (counter < len) {
		if (*JSON_String == 0x27) {
			*JSON_String = 0x22;
		}
		JSON_String++;
		counter++;
	};
}

/*
 * This function checks if the diff(which is number of bytes) when added to pre-stored data
 * crosses the MICROCDB_END_ADDR.
 * Returns: True if crosses or False
 * */
static inline bool checkCrossesMem(uint16_t diff) {
	uint16_t len = CalculateStringLength((uint8_t*) MICROCDB_START_ADDR);
	uint32_t Address = MICROCDB_START_ADDR + len + diff;
	if (Address > MICROCDB_END_ADDR) {
		return true;
	} else
		return false;
}

/* This function shifts the database right by given number of bytes. Shifting will continue until the void block of memory is not created TillAddress.
 * This is used only in updating the database with new data hence creating a
 * void block for updating the new data in DB. This only shifts database and will not copy the new data or erase any old data.
 * Arguments: NumberofBytes -  The number of bytes by which the database needs to be shifted right
 * 			  update_Page_Addr - The address of the start of first byte of the page till where the updated data will be filled
 * 			  updatePath - This the find result having information of the object where update needs to be performed.
 * Returns: flash_mem_Stat SHIFT_SUCCESS if successful or ERASE_FAILED if erase was unsuccessful or FL_STORE_FAILED if Flash writing was unsuccessful
 * NOTE: This function should be used only if the checkCrossesMem return is false*/
static flash_mem_Stat ShiftDatabaseRight(uint16_t NumberofBytes,
		uint8_t *update_Page_Addr, microcDB_Data *updatePath) {
	uint8_t StoredData[FLASH_PAGE_SIZE]; /*This will hold the data which is stored in flash*/
	uint32_t pageToErase, diff = 0, BytesRemaining = 1; //diff will act as a index for a buffer, BytesRemaining is the counter
	uint8_t *addressOfPage, *priorAddr, *latterAddr, *gnrlptr;
	uint8_t FirstChunkRemaining = 1; //setting this to indicate the first chunk is remaining for copying from the End of DB

	/*Logic of the database shift for NumberOfBytes to be updated less than PAGE_SIZE:
	 *  Logic is simple. This is flash memory so we cannot erase byte but need to erase whole page Database shift is nothing but copying a page values
	 *  in RAM and erasing that page in flash and storing the copied page to next address or incremented address by NumberofBytes.
	 *  The database right will shift begin from last page of the end of database (not MICROCDB_END_ADDR! but till this "/") and will go on copying
	 *  till the point where the space needs to be created.
	 *  So below are the steps:
	 *	1.Take Two pointers ptr1, ptr2
	 *	2.Assign the address to ptr2 which will point to the latter memory address (where the shifted page will be stored) and ptr1 to prior
	 *	memory(where the pre-stored data is stored) this will be the (address of ptr2) - Number of bytes data needs to be shifted right.
	 *	3.Prepare the buffer to be written at the page of the ptr2 address. This buffer will have data pointed by ptr1 till ptr2 in the beginning.
	 *	End of buffer will have data pointed by ptr2 memory page till the buffer fills fully.
	 *	4.Now erase the page where the memory address of ptr2 lies and store the prepared buffer in step 3 at this location.
	 *	5.Again assign the ptr2 with address of page where ptr1 lies so ptr2 will point to first byte of the page where ptr1 lies. So in this way we
	 *	are going in descending order by going lower pages. And assign ptr1=ptr2 - (Number of bytes the data needs to be shifted)
	 *	6.Now repeat steps 3-4-5 until the ptr2 reaches address where the update needs to be done. And ptr1 will point to ptr2-(Number of bytes the data needs to be shifted)
	 *	Till now database is shifted and a empty space would have been created in memory where update needs to be done.
	 *  */

	/* Logic of database shift for NumberOfBytes to be updated greater than PAGE_SIZE:
	 * 1.Take three pointers ptr1,ptr2,ptr3
	 * 2.One pointer(ptr1) will point to ending brace of the Object which needs to updated and other will be dangling
	 * 3.Now add the NumberOfBytes to be updated to the address of ptr1
	 * 4.Now copy ptr1 to ptr2,ptr3. Now all pointers are pointing to address till where the data to be updated will be filled. So the data from ending
	 * brace till end of DB needs to be starting ptr1 address which will create a free space from ending brace to ptr1 address.
	 * 5.Now set the ptr3 to point to current End of DB and calculate the new End of DB which till where the DB will end after expansion of DB and
	 * set this address to ptr2.
	 * 6.Now copy data page by page to ptr2 till ptr1 by decrementing by page size.
	 * 7.To copy first decrement the ptr2 by PAGE_SIZE so it will point to address where the old data needs to be stored.
	 * 8.Now prepare buffer to be stored, decrement ptr3 by PAGE_SIZE. Now ptr3 will point to page which needs to be stored to address pointed
	 * by ptr2.
	 * 9.Do steps 9-10 till ptr2 won't point to page of the address where update needs to be performed.*/

	/*Proceed with Shifting*/

	/*If the Number of Bytes to updated is less than the PAGE_SIZE*/
	if (NumberofBytes < PAGE_SIZE) {

		/*Calculate the flash address to which the database needs to be shifted right
		 *Logic is:
		 *- First calculate length of whole database.
		 *- Add it to MICROCDB_START_ADDR. This will give the flash address where the last byte of database is stored
		 */

		latterAddr = (uint8_t*) MICROCDB_START_ADDR
				+ CalculateStringLength((uint8_t*) MICROCDB_START_ADDR);/*From here the database which is shifted will be copied*/
		priorAddr = latterAddr - NumberofBytes; /*This will point to prior memory address by number of bytes that needs to be shifted*/

		/*Find the first address of the page in which the latterAddr lies*/
		pageToErase = CalculateFlashPageNum((uint32_t*) latterAddr);
		addressOfPage = (uint8_t*) (MICROCDB_START_ADDR
				+ (pageToErase * FLASH_PAGE_SIZE));

		/*Do shifting until the latterAddr is not equal to the address of page where update needs to be done*/
		while (latterAddr != update_Page_Addr) {
			diff = 0;

			/*Prepare buffer to be stored*/

			/*copying the data pointed by priorAddr till the addressOfPage and fill the start of Buffer*/
			while (priorAddr < addressOfPage) {
				StoredData[diff] = *priorAddr;
				diff++;
				priorAddr++;
			}

			/*copying the data pointed by addressOfPage it is the address of first byte of the page where the latterAddr lies to fill the end part
			 * of Buffer*/
			while (addressOfPage < latterAddr + 1) {
				StoredData[diff] = *addressOfPage;
				diff++;
				addressOfPage++;
			}

			/*Find the first address of the page in which the StartShiftAddress lies*/
			pageToErase = CalculateFlashPageNum((uint32_t*) latterAddr);
			addressOfPage = (uint8_t*) (MICROCDB_START_ADDR
					+ (pageToErase * FLASH_PAGE_SIZE));

			/*Proceed with storing the prepared buffer*/
			if (ErasePage(addressOfPage) != ERASE_SUCCESS) {
				return ERASE_FAILED;
			}

			if (WritePage((uint32_t*) StoredData, (uint32_t*) addressOfPage,
			FLASH_PAGE_SIZE) != FL_STORE_SUCCESS) {
				return FL_STORE_FAILED;
			}

			/*Make addressOfPage variable to point to previous page*/
			addressOfPage = addressOfPage - PAGE_SIZE;
			latterAddr = addressOfPage; // Assign the latterAddr with the address of addressOfPage

			/*Assign the priorAddr pointer according to addressOfPage var*/
			priorAddr = addressOfPage - NumberofBytes;
		}

	} else { /*If Number of bytes is greater than the PAGE_SIZE*/

		/*Copy the address of last byte/Ending brace of the object*/
		addressOfPage = updatePath->DBEndptr;

		/*Calculate the maximum address*/

		/*Calculate the start of Address till where the updated data will be filled*/
		addressOfPage = addressOfPage + NumberofBytes;/*This will act as ptr1 according to logic*/
		latterAddr = addressOfPage;/*This will act as ptr2 according to logic*/
		gnrlptr = addressOfPage;/*This will act as ptr3 according to logic*/

		/*This will point to old end address of DB*/
		gnrlptr = (uint8_t*) MICROCDB_START_ADDR
				+ CalculateStringLength((uint8_t*) MICROCDB_START_ADDR);

		/*Calculate the new End address of DB which till where the database would end after expanding by number of bytes*/
		latterAddr = gnrlptr + NumberofBytes;

		/*Till now the latterAddr will point to the new End of DB address*/

		/*Now proceed with copying and storing old data to new address to perform "shift/expand" of DB*/

		while (addressOfPage != update_Page_Addr) {

			/*Prepare buffer to be stored*/

			//First make the buffer filled with flash memory empty bytes
			for (BytesRemaining = 0; BytesRemaining < PAGE_SIZE;
					BytesRemaining++) {
				StoredData[BytesRemaining] = FL_EMPTY_BYTE;
			}

			//Copy bytes from flash to buffer
			BytesRemaining = 0;

			/*As we are copying from End of DB so if this is the first chunk to be copied of old data then copying should be done
			 from the first byte and only till latterAddr. This is because the new ending address may be in middle of the page, so to avoid wastage
			 of memory. For this calculate the number of bytes from the new end of DB (latterAddr) till the address of first byte of the page
			 where the latterAddr lies and use this number of bytes to calculate the address from where the copy should begin by subtracting
			 from gnrlptr*/
			if (FirstChunkRemaining) {

				/*Calculate the address of the first byte of the page where the latterAddr lies*/

				/*Get the page number to be erased*/
				pageToErase = CalculateFlashPageNum((uint32_t*) latterAddr);

				/* Now calculate the starting Address of the pageToErase
				 * For it multiply the pageToErase with page size. This will give the number of bytes to be incremented from the MICROCDB_START_ADDR
				 * And add the above calculated value to MICROCDB_START_ADDR. This will give the first Address of the page to be erased
				 * */
				addressOfPage = (uint8_t*) (MICROCDB_START_ADDR
						+ (pageToErase * FLASH_PAGE_SIZE));

				/*Subtract the amount of bytes from gnrlptr(which is pointing to old End of DB) this will give the address from where the
				 * copy of data should begin*/
				addressOfPage = gnrlptr - (latterAddr - addressOfPage);

				//Copy pre stored data end of DB(old)
				while (addressOfPage < (gnrlptr + 1)) {
					StoredData[BytesRemaining] = *addressOfPage;
					addressOfPage++;
					BytesRemaining++;
				}
				//resetting the gnrlptr by subtracting the amount of bytes copied in StoreData buffer
				gnrlptr = gnrlptr - (latterAddr - addressOfPage);
				gnrlptr--;

				/*Calculate the address of First byte of the page where latterAddr lies*/

				/*Get the page number to be erased*/
				pageToErase = CalculateFlashPageNum((uint32_t*) latterAddr);

				/* Now calculate the starting Address of the pageToErase
				 * For it multiply the pageToErase with page size. This will give the number of bytes to be incremented from the MICROCDB_START_ADDR
				 * And add the above calculated value to MICROCDB_START_ADDR. This will give the first Address of the page to be erased
				 * */
				addressOfPage = (uint8_t*) (MICROCDB_START_ADDR
						+ (pageToErase * FLASH_PAGE_SIZE));

			} else { //if first chunk is copied as above then just decrease the gnrlptr by PAGE_SIZE and copy it to buffer
				gnrlptr = gnrlptr - PAGE_SIZE;

				while (BytesRemaining < (PAGE_SIZE + 1)) {
					StoredData[BytesRemaining] = *gnrlptr;
					BytesRemaining++;
					gnrlptr++;
				}

				//Again subtract the PAGE_SIZE as due to above action gnrlptr would have incremented.
				gnrlptr = gnrlptr - PAGE_SIZE;
				gnrlptr--;
			}

			/*Now store the buffer to Flash pointed by latterAddr(ptr2)*/

			//Erase the page first where the buffer is to be stored
			if (ErasePage(addressOfPage) == ERASE_SUCCESS) {
				if (WritePage((uint32_t*) StoredData, (uint32_t*) addressOfPage,
				PAGE_SIZE) != FL_STORE_SUCCESS) {
					return SHIFT_FAILED;
				}
			} else
				return SHIFT_FAILED;

			/*Subtract the latterAddr to point to lesser address by subtracting the amount of bytes copied*/
			/*if first chunk is written*/
			if (FirstChunkRemaining) {
				latterAddr = latterAddr - (latterAddr - addressOfPage);
				latterAddr--;
				FirstChunkRemaining = 0; // reset this so next iteration should not reach here again

				/*Calculate the address of First byte of the page where latterAddr lies*/

				/*Get the page number to be erased*/
				pageToErase = CalculateFlashPageNum((uint32_t*) latterAddr);

				/* Now calculate the starting Address of the pageToErase
				 * For it multiply the pageToErase with page size. This will give the number of bytes to be incremented from the MICROCDB_START_ADDR
				 * And add the above calculated value to MICROCDB_START_ADDR. This will give the first Address of the page to be erased
				 * */
				addressOfPage = (uint8_t*) (MICROCDB_START_ADDR
						+ (pageToErase * FLASH_PAGE_SIZE));
			} else {/*If first chunk was written before then just decrease the latterAddr by PAGE_SIZE this is because after first chunk we will be
			 writing page by page*/
				latterAddr = latterAddr - PAGE_SIZE;
				latterAddr--;

				/*Calculate the address of First byte of the page where latterAddr lies*/

				/*Get the page number to be erased*/
				pageToErase = CalculateFlashPageNum((uint32_t*) latterAddr);

				/* Now calculate the starting Address of the pageToErase
				 * For it multiply the pageToErase with page size. This will give the number of bytes to be incremented from the MICROCDB_START_ADDR
				 * And add the above calculated value to MICROCDB_START_ADDR. This will give the first Address of the page to be erased
				 * */
				addressOfPage = (uint8_t*) (MICROCDB_START_ADDR
						+ (pageToErase * FLASH_PAGE_SIZE));
			}
		}
	}

	return SHIFT_SUCCESS;
}

/*
 * This function will return the index of the character between the pointers if found or 0 if not found
 */
static uint8_t GetIndexofChar(uint8_t* StartPtr, uint8_t* EndPtr,
		uint8_t *CharTobeSearched) {
	uint16_t counter = 0;
	while (StartPtr != EndPtr) {
		if (*StartPtr == *CharTobeSearched) {
			return counter;
		}
		StartPtr++;
		counter++;
	};
	return 0;
}
/*MISC functions*/
/**************************************************************************************************************************************/

/*TODO: Need to find a way to avoid storing the primitive type data(JSMN_PRIMITIVE) as uint8_t and instead convert it
 * to integer to save space. As for example the 28 in the JSONString will be '2''8' which will acquire two
 * bytes, but instead in integer it will acquire only 1 byte! as 28 is one byte <255!*/

/*MicrocDB high level functions*/
microcDB_Status MicrocDB_Init() {
	uint8_t initflag = 0;
	uint8_t *i;
	i = (uint8_t*) MICROCDB_START_ADDR; // Assign the start address of the database to the pointer

	/*Check the 0xDB flag in the last address */
	initflag = *(uint8_t*) MICROCDB_END_ADDR;

	/*Check if the database was initialized before, it means having the flag 0xDB stored at last address*/
	if (initflag == 0xDB) {
		if (EraseDB() == ERASE_SUCCESS) {
			return INIT_CMPLT;
		} else
			return INIT_FAILED;
	} else {
		/*If DB was initialized before, then assign the FlashAddresscntr the address
		 of first empty memory*/

		/*Get the address of the first occurring Empty memory*/
		while (i < (uint8_t*) MICROCDB_END_ADDR) {
			if (*i == FL_EMPTY_BYTE)
				break;
			i++;
		}
		if (i >= (uint8_t*) MICROCDB_END_ADDR - 1) {
			return FLASH_FULL;
		} else
			FlashAddresscntr = (uint32_t) i;/*Assign the address of empty location to FlashAddresscntr
			 So next time the object will stored to empty location only*/

		return INIT_CMPLT;
	}
}

microcDB_Status MicrocDB_Insert(uint8_t *JSONString,
		unsigned int numberofobjects) {

	uint16_t count = 0;/*This variable is used for general purpose counter*/
	size_t len; /*The variable which holds the length of the string passed*/
	unsigned int num = 0; /*initialize the number of object counter*/

	/*Validate JSON first*/
	len = CalculateStringLength(JSONString);
	replacesingleTodouble(JSONString, len);/*Replace single to double quotes without which the JSMN Parser parses the strings as JSMN_PRIMITIVE*/
	/*If JSON is Valid then proceed*/
	while (num < numberofobjects) {
		/*Get the length of first object*/
		len = len + 1; /*Increment as this '/' should also be written to memory
		 for future object splitting*/
		count = 0;
		while (count < len) {
			/*Write to Flash*/
			if (WriteToFLASH(*(JSONString), *(JSONString + 1),
					*(JSONString + 2), *(JSONString + 3)) == FL_STORE_SUCCESS) {
				JSONString = JSONString + 4;
				count = count + 4; /*Increment the JSON string counter*/
			}
		};

		num = num + 1;/*Increment the object counter to get next object*/
		JSONString++; /*Increment this to point next object. Because at this stage it will be pointing to '/'!*/
	};
	return STORE_SUCCESS;

}

microcDB_Data MicrocDB_Find(uint8_t *query) {
	microcDB_Data data_out_struct;

	microcDB_json_parser db_parser;

	microcDBStartAddr = (uint8_t*) MICROCDB_START_ADDR;
	microcDBEndAddr = (uint8_t*) MICROCDB_END_ADDR;

	uint16_t dotIndex = 0; /*This will hold the index of the dot in query*/

	uint16_t eqBytescounter = 0, dotbytescounter = 0; /*This is just a byte counter to count the number of equal bytes*/

	uint8_t* inptr; /*This pointer is used to check the query with memory */

	uint8_t *outptr; /*This pointer is used to store the maximum memory address till which needs to search*/

	/*This will be used to count the parts of query. In MicrocDB the query is done as
	 "users.name./" for {"users":"name"}/ Hence the query is separated by dot(.)
	 So this variable will count the parts of the query string which is separated by dot.
	 It will be initialized with 1 as the query will start from first part.
	 */
	uint16_t queryPartcntr = 0, partlooper = 0;

	/*Calculate the maximum number of parts in query*/
	queryPartcntr = CalculateTotalNumberOfParts(query);

	/*Now search the given query*/

	/*Initialize the json parser on each query process*/
	json_parser_init();

	outptr = microcDBEndAddr; /*Firstly it will point to end of microcDB*/

	/*Now loop for all the parts of query string*/
	for (partlooper = 0; partlooper < queryPartcntr; partlooper++) {

		/*Get the Dot index*/
		dotIndex = getindexofDot(query);

		/*Search the query in database*/
		while (db_parser.parsed_type != JSON_END) {
			db_parser = json_parse();

			/*We are considering only keys hence keys are only strings in JSON and that should be less than inptr(as it will have the maximum memory address) to search for*/
			if (db_parser.parsed_type == JSON_STRING) {

				/*if the parsed data End address is greater than the maximum search address then it means that some part of query not matched
				 * and hence no need to search further and waste time so return with not found*/
				if (db_parser.End > outptr) {
					data_out_struct.DBstatus = NOT_FOUND;
					data_out_struct.JSON_type = JSON_UNDEFINED;
					data_out_struct.DBStartptr = db_parser.Start;
					data_out_struct.DBEndptr = db_parser.End;
					return data_out_struct;
				}

				dotbytescounter = 0; /*reset to point again to the first uint8_t of the query string*/
				eqBytescounter = 0; /*reset this on every new loop*/
				inptr = db_parser.Start;

				/*If first byte of the data pointed by inptr matches with query's first byte then proceed*/
				if (*inptr == *query) {
					db_parser.End++; /*Increment this to point to last char*/

					/*Check if all chars of the db_parser pointed data match with the query*/
					while (inptr != db_parser.End) {
						/*check if the byte pointed by the flashptr is equal with the query+dotbytescounter
						 * Here dotbytescounter acts as variable which adds to query pointer to point to the next
						 * byte of the query string. But it is reset once all the bytes till the dotIndex are compared
						 * */
						if (*inptr == *(query + dotbytescounter)) {
							eqBytescounter++;
							dotbytescounter++;/*Increment the dotbytescounter to point to next byte of the querystring*/
							if (dotbytescounter > dotIndex)
								dotbytescounter = 0;/*reset to point again to the first byte of the query string*/
							inptr++;/*Increment the pointer till the memory address db_parser.End*/
						} else
							break;
					};

					/*
					 * Check if the eqBytescounter is equal to or greater than the dotIndex. Because this would mean that
					 * flash memory pointed by inptr had all the bytes pointed by the querystring.
					 * */
					if (eqBytescounter >= dotIndex) {

						db_parser = json_parse(); /*Get next JSON data*/

						/*If it is Object or Array then assign the end address of it to inptr. By this search will continue only within this limits*/
						if ((db_parser.parsed_type == JSON_OBJ)
								|| (db_parser.parsed_type == JSON_ARRAY))
							outptr = db_parser.End;

						break; /*Break this loop on match of string with query*/
					}
				}
			}
		};

		/*If not a single part of query matched and parsing reached to end of the DB then return not found status*/
		if (db_parser.parsed_type == JSON_END) {
			data_out_struct.DBstatus = NOT_FOUND;
			data_out_struct.JSON_type = JSON_UNDEFINED;
			data_out_struct.DBStartptr = db_parser.Start;
			data_out_struct.DBEndptr = db_parser.End;
			return data_out_struct;
		}

		/*Now prepare the output data. Parse only once to get the next JSON data(OBJ,ARRAY,etc) which would be the value of current key*/
		if (partlooper == queryPartcntr - 1) {
			data_out_struct.DBstatus = FOUND_SUCCESS;
			data_out_struct.JSON_type = db_parser.parsed_type;
			data_out_struct.DBStartptr = db_parser.Start;
			data_out_struct.DBEndptr = db_parser.End;
			return data_out_struct;
		}
		query = query + dotIndex + 1;
	};

	data_out_struct.DBstatus = NOT_FOUND;
	data_out_struct.DBStartptr = db_parser.Start;
	data_out_struct.DBEndptr = db_parser.End;
	return data_out_struct;

}

/*TODO: Need to find a way to update the JSON data type. For example if any body updates a field
 * which was JSON_STRING before with JSON_PRIMITIVE then parser won't parse it as JSON_PRMITIVE
 * because, it was JSON_STRING before and has '\"' quotes before and after data pointed by the path. */
microcDB_Status MicrocDB_Update(uint8_t *path, uint8_t *value) {
	microcDB_Data FindResult;
	uint16_t pageToErase = 0, bytecntr = 0, diff = 0;
	uint32_t NumberofPages = 0, PageCntr = 0;
	uint8_t *addressOfPage, *generalptr, *UpdateEndPtr; /*pointer variables to point to different memory addresses*/
	uint8_t EditedData[FLASH_PAGE_SIZE], PreStoredData[FLASH_PAGE_SIZE]; /*Allocate two buffers of the size of Flash page size from RAM for storing
	 the edited data and the other is the PreStoredData buffer its kept the size of page of the flash. This buffer is used to hold the data which is
	 after the edited data till the end of flash page. And after shifting database this will be the first data to be stored from StartShiftAddress as
	 this is the continuation piece of the edited data */
	uint8_t UpdateCompleteFlag = 0; //This flag will be used to just indicate the last page of update is done and the while loop to be breaked

	FindResult.DBstatus = NOT_FOUND; /*Initialize the find result for NOT_FOUND*/

	FindResult = MicrocDB_Find(path);
	uint16_t len = CalculateStringLength(value);

	/*if strings are passed then replace ' to \"*/
	if (*value == '\'') {
		replacesingleTodouble(value, len);
	}

	if (FindResult.DBstatus == FOUND_SUCCESS) {

		/*Check if the FindResult pointer are not pointing to array if it is then return with error as this is not the function to be used with array*/
		if (FindResult.JSON_type == JSON_ARRAY) {
			return DATA_IS_ARRAY; /*This function cannot be used for arrays*/
		}

		/*Calculate the address till where the updated data would be filled*/
		UpdateEndPtr = FindResult.DBEndptr + len + 1;

		/*Calculate the Address of first byte of page where UpdateEndPtr lies*/

		/*Get the page number to be erased*/
		pageToErase = CalculateFlashPageNum((uint32_t*) UpdateEndPtr);

		/* Now calculate the starting Address of the pageToErase
		 * For it multiply the pageToErase with page size. This will give the number of bytes to be incremented from the MICROCDB_START_ADDR
		 * And add the above calculated value to MICROCDB_START_ADDR. This will give the first Address of the page to be erased
		 * */
		addressOfPage = (uint8_t*) (MICROCDB_START_ADDR
				+ (pageToErase * FLASH_PAGE_SIZE));

		/*Check if the field to be updated is object if it is, then the data to be updated will be next to the old data by adding
		 * a comma next to old data*/
		if (FindResult.JSON_type == JSON_OBJ) {
			/*This means that the data to be updated will erase the pre stored data in flash which is beyond
			 FindResult.DBEndptr and hence the data in flash memory needs to be shifted right with that number of
			 bytes(Bytes from FindResult.DBEndptr), this is the expanding operation of Database.For this operation
			 these extra bytes will be stored in the PreStoredData buffer*/

			/*Check if this operation would cross the boundaries of MICROCDB_END_ADDR*/
			if (checkCrossesMem(
			PAGE_SIZE - (FindResult.DBEndptr - addressOfPage)) == true) {
				return NO_MEMORY;
			} else {
				/*Proceed with update as the data which will be stored won't make database shift right go beyond MICROCDB_END_ADDR*/

				/*Check if database size is less than 1 FLASH_PAGE_SIZE and after update will be less than flash page.
				 *This is very unlikely but case should be handled!*/
				if (CalculateStringLength(
						(uint8_t*) MICROCDB_START_ADDR)<PAGE_SIZE && (CalculateStringLength(
										(uint8_t*) MICROCDB_START_ADDR)+1+ len)<PAGE_SIZE) {
					/*If the database size is less than one flash page size then no need of database shifting, only copy page in RAM and
					 * update new data and write it again!*/

					/*Copy the data to be updated to Edited data buffer*/
					diff = 0;

					/*First copy the data at the page till DBEndptr*/
					while (addressOfPage < FindResult.DBEndptr) {
						EditedData[diff] = *addressOfPage;
						diff++;
						addressOfPage++;
					};

					/*Adding comma only if there is something in the object*/
					if ((FindResult.DBEndptr - FindResult.DBStartptr) > 1) {
						EditedData[diff] = ','; /*Adding comma, as till here the data inside the DBStart-Endptr might be copied*/
						diff++;
					}
					len = diff + len; /*get the number of bytes after adding data to be updated*/

					/*Now copy the the data to be updated*/
					while (diff < len) {
						EditedData[diff] = *value;
						diff++;
						value++;
					};

					/*Copy the data in PrestoredData to Edited Data*/

					while (diff < FLASH_PAGE_SIZE) {
						EditedData[diff] = *addressOfPage;
						diff++;
						addressOfPage++;
					};

					/*Copy the Edited data to Flash by erasing the page*/
					addressOfPage = (uint8_t*) (MICROCDB_START_ADDR
							+ (pageToErase * FLASH_PAGE_SIZE));

					/*Erase the page where the addressOfPage points.*/
					if (ErasePage(addressOfPage) != ERASE_SUCCESS) {
						return UPDATE_FAILED;
					} else {
						if (WritePage((uint32_t*) EditedData,
								(uint32_t*) addressOfPage, FLASH_PAGE_SIZE)
								!= FL_STORE_SUCCESS) {
							return UPDATE_FAILED;
						} else {
							return UPDATE_SUCCESSFUL;
						}
					}

				} else {

					/*First copy the extra bytes after the address (till where the data will be filled after update) till page end as beyond that
					 * the DB shift would have taken care. This will be required during writing last page of EditedData to flash*/
					generalptr = UpdateEndPtr;
					while (generalptr < (addressOfPage + PAGE_SIZE)) {
						PreStoredData[bytecntr] = *generalptr;
						generalptr++;
						bytecntr++;
					}

					/*Calculate the Address of first byte of page where DBEndptr lies*/

					/*Get the page number to be erased*/
					pageToErase = CalculateFlashPageNum(
							(uint32_t*) FindResult.DBEndptr);

					/* Now calculate the starting Address of the pageToErase
					 * For it multiply the pageToErase with page size. This will give the number of bytes to be incremented from the MICROCDB_START_ADDR
					 * And add the above calculated value to MICROCDB_START_ADDR. This will give the first Address of the page to be erased
					 * */
					addressOfPage = (uint8_t*) (MICROCDB_START_ADDR
							+ (pageToErase * FLASH_PAGE_SIZE));

					/*Now copy the data to be updated to this EditedData Buffer*/

					/*Calculate the difference between the addressOfPage and (FindResult.DBEndptr - 1) so that this difference can be added to index of
					 * EditedData buffer so that new data can be updated after it. */
					diff = (FindResult.DBEndptr - 1) - addressOfPage;
					EditedData[diff] = ',';/*Add comma as the new data will be the next object/field in the FindResult boundaries*/
					diff++;/*increment to point to next byte after comma*/

					/*Now copy the data pointed by the value pointer to EditedData from index calculated as diff above till the length of the value string
					 *.This will edit the buffer and replace old values with the new ones*/
					if (len > (PAGE_SIZE - diff))
						generalptr = value + (PAGE_SIZE - diff); //set general pointer only till the remaining size of EditedData so it will not overflow
					else
						generalptr = value + len; //set general pointer to point to last byte of value

					while (value != generalptr) {
						EditedData[diff] = *value;
						value++;
						diff++;
					}

					/*Now copy the bytes in the page before updated data starts*/
					diff = 0;
					generalptr = addressOfPage;
					while (diff < ((FindResult.DBEndptr - 1) - addressOfPage)) {
						EditedData[diff] = *generalptr;
						diff++;
						generalptr++;
					}

					/*Move the database right to create free space to fit the given data at the location where update needs to be done.
					 *For it, the database will be copied to next locations by the number of bytes equal to the length of data that needs to be
					 *updated */

					/*copying the page to be edited to Flash is completed now proceed with shifting whole database right. This can also be called
					 * database expansion operation. Argument is len. The len is the number of bytes of the value(data to be updated), addressOfPage
					 * is the address of first byte of the page where update needs to be performed, FinResult is the microcdb_status after finding
					 * Hence this number of bytes free space is needed*/
					if (ShiftDatabaseRight(len, addressOfPage, &FindResult)
							!= SHIFT_SUCCESS) {
						return UPDATE_FAILED;
					}

					/*Till here the database might have been expanded and a free space equal to len+1 has been created in Flash. Now proceed with
					 * Storing the EditedData to that space thereby storing the first chunk of the data to be update at flash page*/

					/*Erase the page where the addressOfPage points. In this case it will be the page where the FindResult.DBStartptr lies*/
					if (ErasePage(addressOfPage) != ERASE_SUCCESS) {
						return UPDATE_FAILED;
					} else {
						if (WritePage((uint32_t*) EditedData,
								(uint32_t*) addressOfPage, FLASH_PAGE_SIZE)
								!= FL_STORE_SUCCESS) {
							return UPDATE_FAILED;
						} else {
							addressOfPage = addressOfPage + FLASH_PAGE_SIZE;
							diff = 0;/*Making this to zero to point to first index of EditedData buffer*/
						}
					}

					/*Set the generalptr to point to first byte of the page where UpdateEndPtr lies*/
					/*Get the page number to be erased*/
					pageToErase = CalculateFlashPageNum(
							(uint32_t*) UpdateEndPtr);

					/* Now calculate the starting Address of the pageToErase
					 * For it multiply the pageToErase with page size. This will give the number of bytes to be incremented from the MICROCDB_START_ADDR
					 * And add the above calculated value to MICROCDB_START_ADDR. This will give the first Address of the page to be erased
					 * */
					generalptr = (uint8_t*) (MICROCDB_START_ADDR
							+ (pageToErase * FLASH_PAGE_SIZE));

					while (!UpdateCompleteFlag) {

						diff = 0;

						/*Now copy the data pointed by the value pointer to EditedData until the diff is less than the FLASH_PAGE_SIZE as the the
						 * updated data can be more than FLASH_PAGE_SIZE or until the diff is less than the len(number of bytes of data to be updated)
						 * as much of the new data might have stored so this condition was also given to avoid unnecessary pointing*/
						while ((diff < FLASH_PAGE_SIZE) || (diff < len)) {
							if (*value == '/') {
								break; /*Ended the data to be updated hence stop copying*/
							}
							EditedData[diff] = *value;
							value++;
							diff++;
						}

						/*Check if this is the last page to be stored.If it is then need to copy the prestored data at this page which was created
						 * after database shift right operation. Because now that page will be erased as Flash memory allows 32 bit word write operations
						 * So for it the page needs to be erased and whole page will be written. So this time the EditedData will have the remaining
						 * value(data to be updated) and after it the PreStoredData buffer values*/
						if (addressOfPage == generalptr) {

							bytecntr = 0;
							UpdateCompleteFlag = 1; //setting this because this is the last page to be written so this loop needs to be breaked
							while (diff < PAGE_SIZE) {
								EditedData[diff] = PreStoredData[bytecntr]; //Copy the prestored data to next index of EditedData buffer
								diff++;
								bytecntr++;
							}

							if (WritePage((uint32_t*) EditedData,
									(uint32_t*) addressOfPage,
									FLASH_PAGE_SIZE) != FL_STORE_SUCCESS) {
								return UPDATE_FAILED;
							} else {
								addressOfPage = addressOfPage + PAGE_SIZE;
								diff = 0;
								PageCntr++;
							}

						} /*If this is the last page ends here*/

						else {
							/*Now write the EditedData buffer to flash. Here no erase is done, as it will be done by the ShiftDatabaseRight
							 * function*/
							if (WritePage((uint32_t*) EditedData,
									(uint32_t*) addressOfPage,
									PAGE_SIZE) != FL_STORE_SUCCESS) {
								return UPDATE_FAILED;
							} else {
								addressOfPage = addressOfPage + PAGE_SIZE;
								diff = 0;
							}
						}

					};
				}/*Else if database size is less than flash page size*/

			}/*Else checkcrosses memory ends here*/
		} else {

			/*Check the length of data and select the appropriate method of update*/

			/*If data to be updated is greater than the (FindResult.DBEndptr - FindResult.DBStartptr) then the database needs to be shifted right
			 * by the amount of bytes which are more to free up space or if the data to be updated is a object then its natural that the new data
			 * updated will added along with old data in that object so in this case also the database will be expanded*/
			if (len > ((FindResult.DBEndptr - FindResult.DBStartptr) + 1)) {
				/*This means that the data to be updated will erase the pre stored data in flash which is beyond
				 FindResult.DBEndptr and hence the data in flash memory needs to be shifted right with that number of
				 bytes(Bytes from FindResult.DBEndptr), this is the expanding operation of Database.For this operation
				 these extra bytes will be stored in the PreStoredData buffer*/

				/*Check if this operation would cross the boundaries of MICROCDB_END_ADDR*/
				if (checkCrossesMem(
				FLASH_PAGE_SIZE - (FindResult.DBEndptr - addressOfPage)) == true) {
					return NO_MEMORY;
				} else {
					/*Proceed with update as the data which will be stored won't make database shift right go beyond MICROCDB_END_ADDR*/

					/*First copy the extra bytes from the FindResult.DBEndptr to FLASH_PAGE_SIZE to Pre stored buffer*/

					generalptr = FindResult.DBEndptr;

					bytecntr = 0;

					/*Fill prestored data with Empty bytes of Flash memory*/
					while (bytecntr < FLASH_PAGE_SIZE) {
						PreStoredData[bytecntr] = FL_EMPTY_BYTE;
						bytecntr++;
					}

					bytecntr = 0;

					/*Calculate the number of bytes that the PreStoredData will hold*/
					bytecntr = (addressOfPage + FLASH_PAGE_SIZE)
							- FindResult.DBEndptr;

					/*Calculate the number of bytes the DBEndptr is from addressOfPage. This will be the index of the of PreStoredData buffer
					 * below this will be empty FL_EMPTY_BYTE*/

					/*Calculate the number of bytes from which the FindResult.DBEndptr starts including data at DBEndptr*/
					diff = FindResult.DBEndptr - addressOfPage;

					/*Copy the data pointed by the generalptr to pre stored buffer from the diff*/
					while (diff < FLASH_PAGE_SIZE) {
						PreStoredData[diff] = *generalptr;
						generalptr++;
						diff++;
					}

					if (*FindResult.DBEndptr != '}') {
						/*Now copy the Data pointed from addressOfPage to EditedData till the FindResult.DBStartptr as beyond that would be data that needs to
						 * be edited. addressOfPage will be the address of the first byte of the page where the FindResult.DBStartptr lies */
						CopyFlashBytesToRAM(EditedData, addressOfPage,
								(FindResult.DBStartptr - addressOfPage));
					} else {
						/*Now copy the Data pointed from addressOfPage to EditedData till the (FindResult.DBEndptr - 1) as beyond that would be data that needs to
						 * be edited.This is done because this is object so there can be data in the object of FindResult pointers so the new data needs
						 * to be copied with the data within it.AddressOfPage will be the address of the first byte of the page where the FindResult.DBEndptr lies */
						CopyFlashBytesToRAM(EditedData, addressOfPage,
								((FindResult.DBEndptr - 1) - addressOfPage));
					}
					/*Now copy the data to be updated to this EditedData Buffer*/

					if (*FindResult.DBEndptr != '}') {
						/*If FindResult is not a object then set the diff which is index of EditedData to Number of bytes till DBStartptr*/
						diff = FindResult.DBStartptr - addressOfPage;
					} else {
						/*Calculate the difference between the addressOfPage and (FindResult.DBEndptr - 1) so that this difference can be added to index of
						 * EditedData buffer so that new data can be updated after it. */
						diff = (FindResult.DBEndptr - 1) - addressOfPage;
						EditedData[diff] = ',';/*Add comma as the new data will be the next object in the FindResult boundaries*/
						diff++;
					}

					/*Now copy the data pointed by the value pointer to EditedData from index calculated as diff above till the length of the value string
					 *.This will edit the buffer and replace old values with the new ones*/
					while (*value != '/') {
						EditedData[diff] = *value;
						value++;
						diff++;
					}

					/*Check if database size is less than 1 FLASH_PAGE_SIZE. This is very unlikely but case should be handled!*/
					if (CalculateStringLength(
							(uint8_t*) MICROCDB_START_ADDR)<FLASH_PAGE_SIZE) {
						/*If the database size is less than one flash page size then no need of database shifting, only copy page in RAM and
						 * update new data and write it again!*/

						/*First copy the data in PrestoredData to Edited Data*/

						/*Calculate the number of bytes from which the FindResult.DBEndptr starts including data at DBEndptr*/
						diff = FindResult.DBEndptr - addressOfPage;

						while (diff < FLASH_PAGE_SIZE) {
							EditedData[diff + 1] = PreStoredData[diff];
							diff++;
						}

						/*Erase the page where the addressOfPage points.*/
						if (ErasePage(addressOfPage) != ERASE_SUCCESS) {
							return UPDATE_FAILED;
						} else {
							if (WritePage((uint32_t*) EditedData,
									(uint32_t*) addressOfPage,
									FLASH_PAGE_SIZE) != FL_STORE_SUCCESS) {
								return UPDATE_FAILED;
							} else {
								return UPDATE_SUCCESSFUL;
							}
						}

					} else {

						/*Move the database right to create free space to fit the given data at the location where update needs to be done.
						 *For it, the database will be copied to next locations by the number of bytes equal to the length of data that needs to be
						 *updated */

						/*copying the page to be edited to Flash is completed now proceed with shifting whole database right. This can also be called
						 * database expansion operation. Argument is len+bytecntr. The len is the number of bytes of the value(data to be updated)
						 * and in this case the bytecntr is the number of extra bytes which are after FindResult.DBEndptr till End of flash page. Hence
						 * this number of bytes free space is needed*/
						if (ShiftDatabaseRight(len + bytecntr, generalptr,
								&FindResult) != SHIFT_SUCCESS) {
							return UPDATE_FAILED;
						}

						/*Till here the database might have been expanded and a free space equal to len has been created in Flash. Now proceed with
						 * Storing the EditedData to that space*/

						/*set the addressOfPage to FindResult.DBStartptr + 1. So that it will point to flash memory at address next to FindResul.DBStartptr.
						 * This is because the new data needs to be updated/stored under it.*/
						addressOfPage = FindResult.DBStartptr + 1;
						/*Erase the page where the addressOfPage points. In this case it will be the page where the FindResul.DBStartptr lies*/
						if (ErasePage(addressOfPage) != ERASE_SUCCESS) {
							return UPDATE_FAILED;
						} else {
							if (WritePage((uint32_t*) EditedData,
									(uint32_t*) addressOfPage,
									FLASH_PAGE_SIZE) != FL_STORE_SUCCESS) {
								return UPDATE_FAILED;
							} else {
								addressOfPage = addressOfPage + FLASH_PAGE_SIZE;
								diff = 0;/*Making this to zero to point to first index of EditedData buffer*/
							}
						}

						NumberofPages = ((uint32_t) (FindResult.DBStartptr
								+ (len + bytecntr)) - (uint32_t) addressOfPage)
								/ FLASH_PAGE_SIZE;

						while (PageCntr < NumberofPages) {

							/*Now copy the data pointed by the value pointer to EditedData until the diff is less than the FLASH_PAGE_SIZE as the the
							 * updated data can be more than FLASH_PAGE_SIZE or until the diff is less than the len(number of bytes of data to be updated)
							 * as much of the new data might have stored so this condition was also given to avoid unnecessary pointing*/
							while ((diff < FLASH_PAGE_SIZE) || (diff < len)) {
								if (*value == '/') {
									break; /*Ended the data to be updated hence stop copying*/
								}
								EditedData[diff] = *value;
								value++;
								diff++;
							};

							/*Check if this is the last page to be stored.If it is then need to copy the prestored data at this page which was created
							 * after database shift right operation. Because now that page will be erased as Flash memory allows 32 bit word write operations
							 * So for it the page needs to be erased and whole page will be written. So this time the EditedData will have the remaining
							 * value(data to be updated) and after it the PreStoredData buffer values*/
							if ((NumberofPages - PageCntr) != 1) {

								bytecntr = 0;
								while (diff < FLASH_PAGE_SIZE) {
									EditedData[diff] = PreStoredData[bytecntr]; //Copy the prestored data to next index of EditedData buffer
									diff++;
									bytecntr++;
								};

								if (WritePage((uint32_t*) EditedData,
										(uint32_t*) addressOfPage,
										FLASH_PAGE_SIZE) != FL_STORE_SUCCESS) {
									return UPDATE_FAILED;
								} else {
									addressOfPage = addressOfPage
											+ FLASH_PAGE_SIZE;
									diff = 0;
									PageCntr++;
								}

							} /*If this is the last page ends here*/

							else {
								/*Now write the EditedData buffer to flash. Here no erase is done, as it will be done by the ShiftDatabaseRight
								 * function*/
								if (WritePage((uint32_t*) EditedData,
										(uint32_t*) addressOfPage,
										FLASH_PAGE_SIZE) != FL_STORE_SUCCESS) {
									return UPDATE_FAILED;
								} else {
									addressOfPage = addressOfPage
											+ FLASH_PAGE_SIZE;
									diff = 0;
									PageCntr++;
								}
							}

						};
					}/*Else if database size is less than flash page size*/

				}/*Else checkcrosses memory ends here*/

			} /*If len is greater ends here*/
			/*Else data to be updated is equal to the FindResult.DBEndptr - FindResult.DBStartptr and The FindResult pointers are pointing
			 * non JSON object hence can be updated directly*/
			else if (len
					== ((FindResult.DBEndptr - FindResult.DBStartptr) + 1)) {

				/*Now copy the Data pointed from addressOfPage to EditedData till the FLASH_PAGE_SIZE*/
				if (CopyFlashToRAM((uint32_t*) EditedData,
						(uint32_t*) addressOfPage) >= FLASH_PAGE_SIZE) {

					/*Now edit the data pointed by the FindResult.DBStartptr to FindResult.DBEndptr*/

					/*Calculate the difference between the addressOfPage and FindResult.DBStartptr so that this difference can be added to index of
					 * EditedData buffer to get values same as pointed by the FindResult.DBStartptr*/
					diff = FindResult.DBStartptr - addressOfPage;
					bytecntr = 0;

					/*Now copy the data pointed by the value pointer to EditedData from index calculated as diff above till the length of the value string
					 *.This will edit the buffer and replace old values with the new ones*/
					while (bytecntr < len) {
						EditedData[diff] = *value;
						value++;
						diff++;
						bytecntr++;
					};

					/*Now erase the page which has addressOfPage as starting address*/
					if (ErasePage(addressOfPage) == ERASE_SUCCESS) {

						/*Now write to page whose start address is addressOfPage of the Flash the EditedBuffer*/
						if (WritePage((uint32_t*) EditedData,
								(uint32_t*) addressOfPage, FLASH_PAGE_SIZE)
								!= FL_STORE_SUCCESS) {
							return UPDATE_FAILED;
						}

					} else {
						return UPDATE_FAILED;
					}

				} else {
					return UPDATE_FAILED;
				}

			}
			/*Else if the data to be updated is less than the (FindResult.DBEndptr - FindResult.DBStartptr) then the database needs to be
			 * shifted left by amount of bytes by which the data is less.This is the shrinking operation of the database*/
			else if (len < (FindResult.DBEndptr - FindResult.DBStartptr)) {

			}
		}/*If a object check else ends here*/
	} else {
		return PATH_NOT_FOUND;
	}
	return UPDATE_SUCCESSFUL;
}

/*MicrocDB high level functions*/
/**********************************************************************************************************************************/
