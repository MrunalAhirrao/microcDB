/*      Author: Mrunal Ahirao
 *      Description: The JSON parser file for microcDB.
 */

#include "microcDB_jsonparser.h"

int levelCounter = 0; /*This will indicate the deepness of the JSON document currently parser is. The 0 will indicate that the parser is at
 at root of the JSON document. It will increment on each occurrence of the JSON object or ArrayList and will decrement
 after detecting the closing brace or bracket of each object or arrayList respectively*/

int inlevelcntr = 0;

uint8_t firstTime = 0;

microcDB_json_parser jsonParser;
uint8_t *microcDBStartAddr, *inStartAddr;
uint8_t *microcDBEndAddr,*inEndAddr;
uint8_t *memptr, *inptr;

void json_parser_init() {
	levelCounter = -1; /*while init the levelCounter would indicate the level of parsing below the whole JSON object which is -1*/
	inlevelcntr = -1;
	jsonParser.End = 0;
	jsonParser.Start = 0;
	jsonParser.parsed_type = JSON_UNDEFINED;
	memptr = microcDBStartAddr;
	firstTime = 0;
}

microcDB_json_parser json_parse() {
	uint8_t data;
	if (memptr < microcDBEndAddr) {
		data = *memptr;

		switch (data) {

		case '{':

			/*If parsing started then proceed by searching the end of the object. This is because on start this parser returns the starting of root
			 * object and ending of root object*/
			if (firstTime == 0) {
				jsonParser.Start = memptr;
				memptr++;

				while (*memptr != '/') {
					if (*memptr == '{' || *memptr == '[')
						levelCounter++; /*increment the levelCounter on occurrence of Object. The more the value of levelCounter the more the deep
						 in json tree we are. Its 0 value indicates we are at root*/
					else if ((*memptr == '}' || *memptr == ']') && levelCounter != -1)
						levelCounter--;
					memptr++;
				};
				microcDBEndAddr = memptr - 1;

				memptr--;

				firstTime = 1;

				jsonParser.parsed_type = JSON_OBJ;
				jsonParser.End = memptr;
				inStartAddr = jsonParser.Start;
				inEndAddr = jsonParser.End;
				memptr = microcDBStartAddr;
				memptr++;

			} else {

				levelCounter++;
				jsonParser.Start = memptr;
				inptr = memptr;
				/*Set inlevelcntr the previous level so that it can be used to find the level by looping deeper*/
				inlevelcntr = levelCounter - 1;
				memptr++;

				while (memptr < microcDBEndAddr) {
					if (*memptr == '{' || *memptr == '[')
						levelCounter++; /*increment the levelCounter on occurrence of Object. The more the value of levelCounter the more the deep
						 in json tree we are. Its 0 value indicates we are at root*/
					else if ((*memptr == '}' || *memptr == ']')
							&& levelCounter != inlevelcntr)
						levelCounter--;
					/*if the current level is same as the old level or we are out of the object started then break the loop*/
					else if (levelCounter == inlevelcntr)
						break;
					memptr++;
				};

				jsonParser.parsed_type = JSON_OBJ;
				jsonParser.End = memptr - 1;
				inStartAddr = jsonParser.Start;
				inEndAddr = jsonParser.End;
				memptr = inptr;
				memptr++;
			}

			break;

		case '[':

			jsonParser.parsed_type = JSON_ARRAY;
			jsonParser.Start = memptr;/*Assign the start of the ArrayList*/
			memptr++;/*Increment to point to data in the arrayList*/
			inptr = memptr; /*Copy the address of data in array to inptr variable. This is because memptr will be incremented till the ending or
			 array list. As this time the parser should point to the ArrayList*/

			levelCounter++; /*Increment the levelCounter*/

			/*Set inlevelcntr the previous level so that it can be used to find the level by looping deeper*/
			inlevelcntr = levelCounter - 1;

			/*Increment the memptr till the end of array list*/
			while (memptr < microcDBEndAddr) {
				if (*memptr == '{' || *memptr == '[')
					levelCounter++; /*increment the levelCounter on occurrence of Object. The more the value of levelCounter the more the deep
					 in json tree we are. Its 0 value indicates we are at root*/
				else if ((*memptr == '}' || *memptr == ']')
						&& levelCounter != inlevelcntr)
					levelCounter--;
				/*if the current level is same as the old level or we are out of the object started then break the loop*/
				else if (levelCounter == inlevelcntr)
					break;
				memptr++;
			}
			;

			jsonParser.End = memptr - 1;/*Assign the end of array list*/
			memptr = inptr;/*Reinitialize the memptr to the starting address data in arrayList So that data in that can be seen in next loop*/
			inStartAddr = jsonParser.Start;
			inEndAddr = jsonParser.End;
			break;

		case '\"':

			jsonParser.parsed_type = JSON_STRING;
			memptr++; /*Increment the memptr to point to first char of string after \" */
			jsonParser.Start = memptr; /*Assign the address of the first char of string*/

			/*Increment the memptr till next \" . It will indicate till here the string has ended*/
			while (*memptr != '\"') {
				memptr++;
			}
			;

			jsonParser.End = memptr - 1;/*Assign the address of last char of string by subtracting 1.
			 This is because till here the memptr would point to \" (Ending quote of the string)*/
			memptr++; /*Increment the memptr to point to next char. it can be : or , and we not interested in that*/

			if (*memptr == ':' || *memptr == ',' || *memptr == ']'
					|| *memptr == '}') {
				while (*memptr != '\"') {
					if (*memptr != 'f' && *memptr != 't' && *memptr != '1'
							&& *memptr != '2' && *memptr != '3'
							&& *memptr != '4' && *memptr != '5'
							&& *memptr != '6' && *memptr != '7'
							&& *memptr != '8' && *memptr != '9'
							&& *memptr != '0' && *memptr != '['
							&& *memptr != '{') {
						memptr++;
					} else
						break;
				};
			}

			break;

		case 'f':

			jsonParser.parsed_type = JSON_BOOL;
			jsonParser.Start = memptr;

			memptr = memptr + 5;

			jsonParser.End = memptr;

			memptr++;

			break;

		case 't':

			jsonParser.parsed_type = JSON_BOOL;
			jsonParser.Start = memptr;

			memptr = memptr + 4;

			jsonParser.End = memptr;

			memptr++;

			break;

			/*If numeric chars*/
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':

			jsonParser.parsed_type = JSON_PRIMITIVE;
			jsonParser.Start = memptr;

			while ((*memptr != ',') && (memptr < microcDBEndAddr)&& (memptr < inEndAddr))
				memptr++;

	if (*memptr != ',') {
				inptr = memptr;
				while (inptr != jsonParser.Start) {
					inptr--;
					if (*inptr == '1' || *inptr == '2' || *inptr == '3'
							|| *inptr == '4' || *inptr == '5' || *inptr == '6'
							|| *inptr == '7' || *inptr == '8' || *inptr == '9'
							|| *inptr == '0')
						break;
				};
				jsonParser.End = inptr;
				memptr++;
			} else
				jsonParser.End = memptr - 1;

			memptr++;

			break;
			
			
			case '}': // increment the memptr as whenever this case comes the JSON string is about to end
			memptr++;
			jsonParser.parsed_type = JSON_UNDEFINED;

			break;
		case ']': // increment the memptr as whenever this case comes the JSON string is about to end
			memptr++;
			jsonParser.parsed_type = JSON_UNDEFINED;

			break;

		case ',':
			memptr++;
			jsonParser.parsed_type = JSON_UNDEFINED;

			break;

		default:
			jsonParser.parsed_type = JSON_UNDEFINED;

			
		};

	} else {
		jsonParser.parsed_type = JSON_END;
		jsonParser.Start = memptr - 1;
		jsonParser.End = memptr;
		return jsonParser;
	}
	return jsonParser;
}
