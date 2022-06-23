#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

char* TABLE = "kv";

/**
 * @data        sqlite3_exec()提供的第三个参数
 * @argc        列数
 * @argv        该列的值
 * @azColName   该列的名字
*/
static int
callback(void* data, int argc, char** argv, char** azColName)
{
    // int i;

    // for (i = 0; i < argc; i++)
    //     printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    // printf("\n");
    snprintf(data, BUFF_LEN, "%s", argv[argc - 1]);
    return 0;
}

/**
 * 插入 key value
*/
int
sqlite_insert(sqlite3** db, char* key, char* value)
{
    int  rc;
    char* zErrMsg;
    char sql[SQLITE_MAX_SQL_LENGTH];

    if (key == NULL)
        return KEY_NULL;
    if (value == NULL)
        return VALUE_NULL;

    snprintf(sql, SQLITE_MAX_SQL_LENGTH,
                "INSERT INTO %s (key, value) VALUES ('%s', '%s');",
                TABLE, key, value);

    rc = sqlite3_exec(*db, sql, NULL, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "%d sqlite3_exec(): %s\n", rc, zErrMsg);
        sqlite3_free(zErrMsg);
        return INSERT_FAILED;
    }
    else {
        // fprintf(stderr, "PUT successfully\n");
        return INSERT_SUCCESS;
    }
}


/**
 * 按照key查找，如果有会复制到value
*/
int
sqlite_select(sqlite3** db, char* key, char* value)
{
    int  rc;
    char* zErrMsg;
    char sql[SQLITE_MAX_SQL_LENGTH];

    if (key == NULL)
        return KEY_NULL;
    
    snprintf(sql, SQLITE_MAX_SQL_LENGTH,
                "SELECT value FROM %s WHERE key = '%s';", TABLE, key);

    rc = sqlite3_exec(*db, sql, callback, value, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "%d sqlite3_exec(): %s\n", rc, zErrMsg);
        sqlite3_free(zErrMsg);
        return SELECT_FAILED;
    }
    if (value == NULL) {
        return VALUE_NULL;
    }
    else {
        // fprintf(stderr, "SELECT successfully\n");
        return SELECT_SUCCESS;
    }
}


/**
 * 按照key删除
*/
int
sqlite_delete(sqlite3** db, char* key)
{
    int  rc;
    char* zErrMsg;
    char sql[SQLITE_MAX_SQL_LENGTH];

    if (key == NULL)
        return KEY_NULL;
    
    snprintf(sql, SQLITE_MAX_SQL_LENGTH,
                "DELETE FROM %s where key = '%s';", TABLE, key);

    rc = sqlite3_exec(*db, sql, NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "%d sqlite3_exec(): %s\n", rc, zErrMsg);
        sqlite3_free(zErrMsg);
        return DELETE_FAILED;
    }
    else {
        // fprintf(stderr, "DELETE successfully\n");
        return DELETE_SUCCESS;
    }
}

void
read_input(char* buffer, int size)
{
	char *nl = NULL;
	memset(buffer, 0, size);
	if ( fgets(buffer, size, stdin) != NULL ) {
		nl = strchr(buffer, '\n');
		if (nl)
            *nl = '\0';
	}
}


int
main(int argc, char** argv)
{
    int         rc;
    char*       pstr;
    char        key[BUFF_LEN];
    char        value[BUFF_LEN];
    char        buffer[BUFF_LEN*4];
    sqlite3*    db;

    rc = sqlite3_open_v2("db1.sqlite", &db, SQLITE_OPEN_READWRITE, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "sqlite3_open: %s\n", sqlite3_errmsg(db));
        sqlite3_close_v2(db);
        return OPEN_FAILED;
    }
    else {
        fprintf(stderr, "Opened database successfully!\n");
    }

    while (1) {
        read_input(buffer, BUFF_LEN * 4);
        if (buffer == NULL)
            continue;

        memset(key, 0, BUFF_LEN);
        memset(value, 0, BUFF_LEN);

        pstr = strtok(buffer, " \t");
        if (pstr == NULL)
            continue;

        if (strncmp(pstr, "GET", 3) == 0) {
            pstr = strtok(NULL, " \t");
            if (pstr == NULL)
                continue;
            strncpy(key, pstr, BUFF_LEN);

            sqlite_select(&db, key, value);
            if (value[0] != '\0')
                fprintf(stderr, "%s\n", value);
        }

        else if (strncmp(pstr, "PUT", 3) == 0) {
            pstr = strtok(NULL, " \t");
            if (pstr == NULL)
                continue;
            strncpy(key, pstr, BUFF_LEN);

            pstr = strtok(NULL, " \t");
            if (pstr == NULL)
                continue;
            strncpy(value, pstr, BUFF_LEN);

            sqlite_insert(&db, key, value);
        }

        else if (strncmp(pstr, "DEL", 3) == 0) {
            pstr = strtok(NULL, " \t");
            if (pstr == NULL)
                continue;
            strncpy(key, pstr, BUFF_LEN);

            sqlite_delete(&db, key);
        }

        else if (strncmp(pstr, "QUIT", 4) == 0) {
            break;
        }
    }

    sqlite3_close(db);
    return 0;
}