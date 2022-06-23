#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

#define N 1024

int main(){
    char commd[N];
    struct sockaddr_in addr;        //目标服务器地址
    int len;
    int opt;
    int connect_socket = 0;         //通信套接字
    int i, j;

    bzero(&addr, sizeof(addr));     
    addr.sin_family = AF_INET;      //AF_INET代表TCP／IP协议
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(/* 服务器端口 */);    
    len = sizeof(addr);

    LO:                             //尝试登录，2s周期
    connect_socket = login(addr);   //此处返回了通信套接字
    if(connect_socket <= 0){
        printf("连接失败，重新连接...\n");
        sleep(2);
        goto LO;
    }

    
}

int login(struct sockaddr_in addr){
    int sockfd;

    //创建套接字
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Socket错误!\n");
        return -1;
    }
    
    if(connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        printf("服务器离线!\n");
        return -2;
    }
    //连接完成，返回socket描述符
    return sockfd;
}