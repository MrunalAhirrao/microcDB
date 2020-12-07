/**
 * ******************************************************************************
 * @file            microcDB.h
 * @brief           This is the interface file for the microcDB having all the function prototypes.
 * @author          Mrunal Ahirao.
 ******************************************************************************
 **************************************************************************************************************************************************
 *      NOTE: YOU SHOULD NOT EDIT THIS FILE! Just include this in your source and microcDB is good to go.
 **************************************************************************************************************************************************
 **/

#ifndef MICROCDB_H_
#define MICROCDB_H_

#include <stdint.h>
#include "microcDB_config.h"
#include "flash_drivers.h"

/**
 * @brief This enum typedef will indicate different database statuses.
 */
/*MicrocDB Status enums typedef*/
typedef enum {
	/** This status indicates that the insert operation was successful */
	STORE_SUCCESS = 0,
	/** This status indicates that the insert operation failed */
	STORE_FAILED = 1,
	/** This status indicates that the path given in query is not found by find operation  */
	NOT_FOUND = 2,
	/** This status indicates that the find operation found the value of the query */
	FOUND_SUCCESS = 3,
	/** This status indicates that the microcDB is initialized succesfully */
	INIT_CMPLT = 4,
	/** This status indicates that the microcDB initialization failed */
	INIT_FAILED = 5,
	/** This status indicates that the given JSON string is not valid */
	INVALID_JSON = 6,
	/** This status indicates that the assigned memory of DB is full */
	FLASH_FULL = 7,
	/** This status indicates that the given query string is not according to microcDB format */
	QUERY_INVALID = 8,
	/** This status indicates that the update operation is successful */
	UPDATE_SUCCESSFUL = 9,
	/** This status indicates that the update operation is failed */
	UPDATE_FAILED = 10,
	/** This status indicates that the path given for update operation not found  */
	PATH_NOT_FOUND = 11,
	/** This status indicates that the path given for update operation is not a ArrayList type */
	PATH_NOT_ARRAYLIST = 12,
	/** This status indicates that the append operation is successful */
	APPEND_SUCCESS = 13,
	/** This status indicates that the append operation failed */
	APPEND_FAILED = 14,
	/** This status indicates that the update operation cannot be executed as it would cross the boundries of DB address */
	NO_MEMORY = 15,
	/** This status indicates that the path given for update operation is ArrayList type */
	DATA_IS_ARRAY = 16,
	/** This status indicates that the Database is empty */
	DB_EMPTY = 17
} microcDB_Status;
/*MicrocDB Status enums typedef*/

/**
 * @brief This enum typedef will indicate different JSON types after retrieving.
 */
/*
 * JSON types typedef used to indicate the data indicated is of what type of JSON.
 */
typedef enum {
	/** This indicates that the retrieved JSON is of type Object*/
	JSON_OBJ,
	/** This indicates that the retrieved JSON is of type String*/
	JSON_STRING,
	/** This indicates that the retrieved JSON is of type ArrayList*/
	JSON_ARRAY,
	/** This indicates that the retrieved JSON is of type boolean*/
	JSON_BOOL,
	/** This indicates that the retrieved JSON is of type Primitive*/
	JSON_PRIMITIVE,
	JSON_END,
	/** This indicates that the retrieved JSON is of type undefined*/
	JSON_UNDEFINED
} JSON_Type;

/**
 * @brief This struct typedef have the details of retrieved values from DB.
 */
/*microcDB_Data typedef Struct*/
typedef struct {
	/** This field will have different Database statuses*/
	microcDB_Status DBstatus;
	/** This field will have type of JSON data of the retrieved values*/
	JSON_Type JSON_type;
	/** This field will have the start pointer of the retrieved values*/
	uint8_t *DBStartptr;
	/** This field will have the End pointer of the retrieved values*/
	uint8_t *DBEndptr;
} microcDB_Data;
/*microcDB_Data typedef Struct*/

/*Function prototypes of MicrocDB*/

/**
 * @brief This function initializes the Flash memory for MicrocDB. This should be called before using
 * MicrocDB other functions.
 ** @returns #microcDB_Status INIT_CMPLT = 4, INIT_FAILED = 5, FLASH_FULL = 7
 * */
microcDB_Status MicrocDB_Init();

/**
 * @brief This function will insert the JSON object to Database. The JSON string may have multiple objects
 * but should be separated by '/'. If there is already a JSON object then using this command will add JSON object next to old object.
 * @param *JSONString : JSON String
 * @param numberofobjects : The number of JSON objects
 *
 * @note Though you are storing only one object but you should add '/' at the end of object string.
 * @returns  The #microcDB_Status enum. #STORE_SUCCESS = 0, #STORE_FAILED = 1
 * */
microcDB_Status MicrocDB_Insert(uint8_t *JSONString,
		unsigned int numberofobjects);

/**
 * @brief This function will get the value of the query from database.
 * @returns the #microcDB_Data struct. The struct has the following members:<ul>
 * <li>#microcDB_Status type which will indicate the status of the find operation which is FOUND_SUCCESS = 3 if found or NOT_FOUND = 2 if
 * not found.</li>
 * <li>JSON_type: This indicates the type of JSON data type where the query string points to.It can be Object, Array, String, etc.</li>
 * <li>DBStartptr: This is the pointer to MicrocDB memory which will point to the start of the data which needs to be find by query.</li>
 * <li>DBEndptr: This is the pointer to MicrocDB memory which will point to the end of the data which needs to be find by query.</li></ul>
 * @note Check the #microcDB_Status value first if its #NOT_FOUND then it means the requested data was not found, and hence no need to
 * see at the DBStartptr or DBEndptr.
 * @param *query : The query string by dot operators like "A.B.C./". The "./" is <b>VERY IMPORTANT</b> at the end of query string!
 * */
microcDB_Data MicrocDB_Find(uint8_t *query);

/**
 * @brief 	Updates the DB with given data at given path.
 * @brief This function updates the value/data of path given.Path is the same as query language. For example:
 * @brief if DB is {"users":{"Jack":{"Age":28},"David":{"Age":25}}} so for updating the Age of David the path
 * @brief would be like this: "users.David.Age./","30/". This will change the "Age" field of David object to 30.
 * @brief if your update value is string then no need to add '<b>\"</b>' only the string in " ".
 *
 * @warning If you pass a number where there was a string then, it will considered as JSON_STRING! Currently microcDB
 * don't support <b>JSON data type</b> update.If you want to update a object by adding a new object/field then the data which was already in the object
 * will be kept and new object/field will be added next to it. Hence old data will not be replaced.
 *
 * @param *path : The DB path whose value to be changed.
 * @param *value : The value to be updated.This should be terminated with <b>'/'</b>
 * @returns  The #microcDB_Status. <ul>
 * <li>if Update was successful #UPDATE_SUCCESSFUL = 9</li>
 * <li>if Update failed #UPDATE_FAILED = 10</li>
 * <li>if given path not found #PATH_NOT_FOUND = 11,</li>
 * <li>if given update operation crosses the MICROCDB_END_ADDR boundary then #NO_MEMORY = 15 in this case data is not changed.</li>
 * <li>if the object to be updated is <b>ARRAY_LIST</b> then #DATA_IS_ARRAY = 16, use MicrocDB_UpdateArrayList() instead. </li>
 * </ul>
 * @note <ul>
 * <li>This function should only be use for updating a single key's value or adding a new object to a object.</li>
 * <li>If used for updating a object then it should be correct or else microcDB_Status INVALID_JSON = 6 will be returned and
 * update will fail.</li>
 * <li>Don't use this for appending a value to a array use the MicrocDB_UpdateArrayList instead. Still if you used this then it will return
 * #DATA_IS_ARRAY = 16 status and update won't take place.</li>
 * </ul>
 */
microcDB_Status MicrocDB_Update(uint8_t *path, uint8_t *value);

/**
 * @brief This function will append the given data to the array list at given path. Path is the same as query language. For example:
 * @brief if DB is {"users":{"groups":["Jack","David","Mario"]}} so for appending "Chris" the path and data
 * @brief would be like this: "users.groups./","Chris".
 * @param *path : The DB path whose value to be changed.
 * @param *data : The data to be appended to the path.
 * @returns  The #microcDB_Status. <ul>
 * <li>if Update was successful #UPDATE_SUCCESSFUL = 9</li>
 * <li>if Update failed #UPDATE_FAILED = 10</li>
 * <li>if given path not found #PATH_NOT_FOUND = 11,</li>
 * <li>if given update operation crosses the #MICROCDB_END_ADDR boundary then #NO_MEMORY = 15 in this case data is not changed.</li>
 * <li>if the path to be updated is not <b>ARRAY_LIST</b> then #PATH_NOT_ARRAYLIST = 16</li>
 * </ul>
 */
microcDB_Status MicrocDB_UpdateArrayList(uint8_t *path, uint8_t*data);

/*
 */
microcDB_Status MicrocDB_Delete(uint8_t *path, uint8_t*data);

/*Function prototypes of MicrocDB*/

#endif /* MICROCDB_H_ */
