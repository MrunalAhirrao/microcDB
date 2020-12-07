# microcDB
microcDB is a light weight NoSQL database for mostly resource limited embedded/IoT systems. The database can be stored in local flash memory or the external SD card or external memory(EEPROM,FLASH,etc). It can also act as a server in which a dedicated MCU will handle requests from Clients on UART,MODBUS,Ethernet,SPI,I2C,CAN or any wireless communication. microcDB also supports authentication,encryption or access levels for specific users. microcDB is supported on major IoT OSes and RTOS.  Currently its developed for STM32 but soon it will be developed for other MCUs too.

The most disrupting feature of microcDB is it has a "Hard Index". As microcDB directly interfaces with raw memory without filesystem so we can give a new type of index for fast DB operations. This is a new type of index which will directly point to memory address of a JSON object which needs to be indexed. The data_type will be :

typedef struct {
uint8_t* startAddressOfObject;
uint8_t* endAddressOfObject;
}hard_index;

This is not yet implemented nor its algorithm is designed but will do it soon once all CRUD operations are implemented.

The only con of this disrupting feature is extra memory usage because all these "hard indexes" are stored at some predefined location. 

The other disrupting feature is it has schema like SQL type databases.This schema will be JSON document with specific keywords that are interpreted by microcDB.And microcDB will let DB grow only according to this schema. Such feature is not available in other NoSQL DBs. 

Again this feature is not yet implemented nor its algorithm is developed.

What microcDB lacks currently compared to other databases?
1. Don't support the range query currently
2. Don't support the transactions supporting ACID.
3. Don't support capping to specific document, instead the whole database has the maximum limit address which is indirectly capped in flash memory usage sense!
4. Currently supports only C language.
5. Don't support sorting.
