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
send_yes(int sockfd)
{
    char* yes = "yes";
    return writen(sockfd, yes, 4);
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
    char                key[BUFF_LEN];
    char                value[BUFF_LEN];
    sqlite3*            db;


    // 文件存在
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
    // 文件未创建
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
        if ((connect_sock = accept(listen_sock, &server, &len)) == -1) {
            if (errno == EINTR)
                continue;
            else
                perror("accept()");
        }

        // 连接完成，开始等待数据
        for ( ; ; ) {
            memset(buffer, 0, BUFF_LEN * 4);

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

            if (send_yes(connect_sock) <= 0) {
                fprintf(stderr, "发送yes失败\n");
                close(connect_sock);
                break;
            }

            

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
    }
}