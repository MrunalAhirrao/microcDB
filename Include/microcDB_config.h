/**
 ******************************************************************************
 * @file           : microcDB_config.h
 * @brief           This is a configuration file of microcDB. It contains all the information regarding
 *                    configuration of database
 ******************************************************************************
 **    Configurations like:
 *
 *      1.MICRODB_START_ADDR-> The address of Flash memory from where the microcDB will keep the
 *      memory as DB memory.
 *
 *      2.MICRODB_END_ADDR-> The address of Flash memory from till where microcDB will keep the
 *      memory only for DB purpose.
 *
 *      3.PAGE_SIZE-> The size of a single page of flash memory in bytes
 *
 *      4.FL_EMPTY_BYTE-> This is the empty byte for flash memory. Every Flash memory has some byte which indicates the emptiness of the memory
 *      address.
 *
 *		You should edit the macros by replacing the default values with your values.
 */

#ifndef MICROCDB_CONFIG_H_
#define MICROCDB_CONFIG_H_

/*Flash memory allocation*/
/**
 * @brief This macro is used to set the memory address of Flash memory from where database storage will begin
 */
#define MICROCDB_START_ADDR -1

/**
 * @brief This macro is used to set the memory address of Flash memory till where database will be stored
 */
#define MICROCDB_END_ADDR   -1
/*Flash memory allocation*/

/*Flash memory page size*/
/**
 * @brief The size of a single page of flash memory in bytes
 * */
#define PAGE_SIZE 0
/*Flash memory page size*/

/*Empty Byte of The Flash memory*/
/**
 * @brief This is the empty byte for flash memory. Every Flash memory has some byte which indicates the emptiness of the memory address
 * */
#define FL_EMPTY_BYTE -1

/*The maximum DB size*/
#define MAX_DB_SIZE (MICROCDB_END_ADDR-MICROCDB_START_ADDR)

/**
 * @brief Error checkers and indicator macros
 *  **/

#if PAGE_SIZE == 0
#error "MicrocDB Error:Please edit the macro PAGE_SIZE size of single page of Flash in bytes in microcDB_config.h file"
#endif

#if FL_EMPTY_BYTE == -1
#error "MicrocDB Error:Please edit the macro FL_EMPTY_BYTE in microcDB_config.h file"
#endif

#if MICROCDB_START_ADDR == -1
#error "MicrocDB Error:Please define the macro of starting database address named as MICROCDB_START_ADDR in microcDB_config.h file."
#endif

#if MICROCDB_END_ADDR == -1
#error "MicrocDB Error:Please define the macro of ending database address named as MICROCDB_END_ADDR microcDB_config.h file."
#endif

#endif /* MICROCDB_CONFIG_H_ */
