/* Author: Mrunal Ahirao
 * Description: The JSON header file for JSON parser.
 */

#ifndef MICROCDB_JSONPARSER_H_
#define MICROCDB_JSONPARSER_H_

#include <stdint.h>

/*
 * JSON types typedef used to indicate the data indicated is of what type of JSON.
 */
typedef enum{
	JSON_OBJ,
	JSON_STRING,
	JSON_ARRAY,
	JSON_BOOL,
	JSON_PRIMITIVE,
	JSON_END,
	JSON_UNDEFINED
}JSON_Type;

/*
 * The parser typedef which gives following information:
 * 1. The JSON type
 * 2. The start and End pointers of memory for the given JSON Type
*/
typedef struct {
	JSON_Type parsed_type;
	uint8_t *Start;
	uint8_t *End;
} microcDB_json_parser;

extern uint8_t *microcDBStartAddr;
extern uint8_t *microcDBEndAddr;

/*Function prototypes of json parser*/

/*This function will initialize the variables used in the parser*/
void json_parser_init();

/*This function will parse the JSON in memory and will return the next microcDB_json_parser type on each call. The type JSON_END indicates
 * the memory within which the search needs to be done is completed or the parsing has reached the last address of microcDB memory*/
microcDB_json_parser json_parse();

/*Function prototypes of json parser*/

#endif /* MICROCDB_JSONPARSER_H_ */
