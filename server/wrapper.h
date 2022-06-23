#include <sqlite3.h>

#define SQLITE_MAX_SQL_LENGTH 1000000

#define BUFF_LEN        1024

/* NULL */
#define KEY_NULL        100
#define VALUE_NULL      101
#define TABLE_NULL      102

/* SUCCESS */
#define OPEN_SUCCESS    200
#define INSERT_SUCCESS  201
#define SELECT_SUCCESS  202
#define DELETE_SUCCESS  203

/* FAILED */
#define OPEN_FAILED     400
#define INSERT_FAILED   401
#define SELECT_FAILED   402
#define DELETE_FAILED   403

char* TABLE     = "kv";
char* filename  = "db.sqlite";

static int
callback(void* data, int argc, char** argv, char** azColName);

int
sqlite_insert(sqlite3** db, char* key, char* value);

int
sqlite_select(sqlite3** db, char* key, char* value);

int
sqlite_delete(sqlite3** db, char* key);

void
read_input(char* buffer, int size);