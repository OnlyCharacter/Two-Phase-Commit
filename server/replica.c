#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>
/*
    memset()
    strncmp()
    strncpy()
*/
#include <stdio.h>
/*
    fprintf()
    printf()
    snprintf()
*/
#include <unistd.h>
/*
    access()
    socket()
    struct sockaddr
*/
#include <netinet/in.h>
/*
    struct sockaddr_in
*/
#include <errno.h>
/*
    errno
*/
#include "wrapper.h"

char* TABLE     = "kv";
char* filename  = "db.sqlite";

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

ssize_t
Read(int fd, void *buf, size_t size) {
   ssize_t retval;
   while (retval = read(fd, buf, size), retval == -1 && errno == EINTR) ;
   return retval;
}

ssize_t						/* Write "n" bytes to a descriptor. */
writen(int fd, const void *vptr, size_t n)
{
	size_t		nleft, nwritten;
	const char	*ptr;

	ptr = vptr;	/* can't do pointer arithmetic on void* */
	nleft = n;
	while (nleft > 0) {
		if ( (nwritten = write(fd, ptr, nleft)) <= 0)
			return(nwritten);		/* error */

		nleft -= nwritten;
		ptr   += nwritten;
	}
	return(n);
}

ssize_t
send_YES(int sockfd)
{
    char* yes = "YES";
    return writen(sockfd, yes, strlen(yes));
}

ssize_t
send_NO(int sockfd)
{
    char* no = "NO";
    return writen(sockfd, no, strlen(no));
}

int
wait_yes(int sockfd)
{
    int retval;
    char buff[BUFF_LEN];
    memset(buff, 0, BUFF_LEN);
    retval = Read(sockfd, buff, BUFF_LEN);
    // 连接断开
    if (retval == 0) {
        return retval;
        close(sockfd);
    }
    else if (retval == -1) {
        return retval;
        close(sockfd);
    }
    else if (retval > 0 && strcmp(buff, "YES") == 0)
        return 1;
    else
        return retval;
}


int
main(int argc, char** argv)
{
    struct sockaddr_in  replica;        // 自身作为 replica 使用
    struct sockaddr_in  server;         // 记录 server 连接信息
    socklen_t           len;
    int                 listen_sock;
    int                 connect_sock;
    ssize_t             read_size;

    int                 rc;
    char                buffer[BUFF_LEN * 4];
    char*               pstr;
    char*               zErrMsg;
    char*               null_value = "NULL VALUE";
    char                key[BUFF_LEN];
    char                value[BUFF_LEN];
    sqlite3*            db;


    // 数据库文件存在
    if (access(filename, F_OK) == 0) {
        rc = sqlite3_open_v2(filename, &db, SQLITE_OPEN_READWRITE, NULL);
        
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Open %s failed: %s\n", filename, sqlite3_errmsg(db));
            sqlite3_close_v2(db);
            return OPEN_FAILED;
        }
        else {
            fprintf(stderr, "Open %s successfully.\n", filename);
        }
    }
    // 数据库文件未创建
    else {
        // 创建数据库
        rc = sqlite3_open_v2(filename, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Create %s failed: %s\n", filename, sqlite3_errmsg(db));
            sqlite3_close_v2(db);
            return OPEN_FAILED;
        }
        else {
            fprintf(stderr, "Open %s successfully.\n", filename);
        }

        // 创建数据表
        snprintf(buffer, BUFF_LEN * 4,
            "CREATE TABLE %s (key varchar(1000) PRIMARY KEY, value varchar(1000));", TABLE);
        rc = sqlite3_exec(db, buffer, callback, 0, &zErrMsg);
        if( rc != SQLITE_OK ) {
            fprintf(stderr, "Create table '%s' error: %s", TABLE, zErrMsg);
            sqlite3_free(zErrMsg);
            return OPEN_FAILED;
        }
        else {
            fprintf(stdout, "Create table '%s' successfully.\n", TABLE);
        }
    }

    // 创建 socket
    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == -1) {
        perror("socket()");
        return -1;
    }

    if ((setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &len, sizeof(socklen_t))) == -1)
    {
        close(listen_sock);
        return -1;
    }

    // 创建 sockaddr_in
    memset(&replica, 0, sizeof(struct sockaddr_in));
    replica.sin_family       = AF_INET;
    replica.sin_port         = htons(atoi(argv[1]));
    replica.sin_addr.s_addr  = htonl(INADDR_ANY);

    // bind
    len = sizeof(struct sockaddr_in);
    if (bind(listen_sock, (struct sockaddr*)&replica, len) == -1) {
        perror("bind()");
        close(listen_sock);
        return -1;
    }

    // listen
    if (listen(listen_sock, 1) == -1) {
        perror("listen()");
        close(listen_sock);
        return -1;
    }

    for ( ; ; ) {
        // 等待连接
        if ((connect_sock = accept(listen_sock, (struct sockaddr*)&server, &len)) == -1) {
            if (errno == EINTR)
                continue;
            else
                perror("accept()");
        }

        // 连接完成，开始等待数据
        for ( ; ; ) {
            memset(buffer, 0, BUFF_LEN * 4);

            // 接受命令
            read_size = recv(connect_sock, buffer, BUFF_LEN * 4, 0);
            // 断开
            if (read_size == 0) {
                close(connect_sock);
                break;
            }
            // 出错
            else if (read_size == -1) {
                perror("recv()");
                close(connect_sock);
                break;
            }

            fprintf(stderr, "RECV: %s\n", buffer);
            // 处理命令
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

                // 查询
                rc = sqlite_select(&db, key, value);
                // 执行失败 或 空值
                if (value[0] == '\0')
                    writen(connect_sock, null_value, strlen(null_value));
                // 执行成功，返回 value
                else
                    writen(connect_sock, value, strlen(value));
                close(connect_sock);
                break;
            }

            else if (strncmp(pstr, "PUT", 3) == 0) {
                /*        获取 key      */
                pstr = strtok(NULL, " \t");
                if (pstr == NULL) {
                    send_NO(connect_sock);
                    close(connect_sock);
                    break;
                }
                strncpy(key, pstr, BUFF_LEN);

                /*        获取 value      */
                pstr = strtok(NULL, " \t");
                if (pstr == NULL) {
                    send_NO(connect_sock);
                    close(connect_sock);
                    break;
                }
                strncpy(value, pstr, BUFF_LEN);

                /*  执行    */
                rc = sqlite_insert(&db, key, value);
                // 执行失败
                if (rc != INSERT_SUCCESS) {
                    fprintf(stderr, "%d insert failed\n", rc);
                    send_NO(connect_sock);
                    close(connect_sock);
                    break;
                }
                /*  执行成功    */
                else {
                    /*   发送 YES，失败回滚   */
                    rc = send_YES(connect_sock);
                    if (rc <= 0) {
                        fprintf(stderr, "发送YES失败\n");
                        rollback(&db);
                        close(connect_sock);
                        break;
                    }
                }

                /*   需要回滚   */
                if (wait_yes(connect_sock) != 1)
                    rollback(&db);
                /*   提交   */
                else {
                    commit(&db);
                    send_YES(connect_sock);
                }

                close(connect_sock);
                break;
            }

            else if (strncmp(pstr, "DEL", 3) == 0) {
                pstr = strtok(NULL, " \t");
                if (pstr == NULL)
                    continue;
                strncpy(key, pstr, BUFF_LEN);

                /*  执行    */
                rc = sqlite_delete(&db, key);
                /*  发送 YES 给 master  */
                if (send_YES(connect_sock) <= 0) {
                    fprintf(stderr, "发送YES失败\n");
                    rollback(&db);
                    close(connect_sock);
                    break;
                }

                /*  回滚   */
                if (wait_yes(connect_sock) != 1)
                    rollback(&db);
                else {
                    commit(&db);
                    send_YES(connect_sock);
                }

                close(connect_sock);
            }

            else if (strncmp(pstr, "QUIT", 4) == 0) {
                close(connect_sock);
                break;
            }
        }
    }
}