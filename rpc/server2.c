//头文件————————————————————
#include <netinet/in.h> //sockaddr_in、htonl()、htons()
#include <sys/socket.h> // socket()、setsockopt()、bind()、listen()、accept()、recv()、send()
#include <stdio.h>      //perror()
#include <stdlib.h>     //exit()、EXIT_FAILURE
#include <string.h>     //bzero()、strncpy()
#include <unistd.h>     //close()

//宏————————————————————
#define SERV_PORT 3335     //服务端端口号
#define LISTEN_MAX_COUNT 1 //所监听的最大连接数
#define BUFF_SIZE 64       //传递消息缓冲区大小

//函数声明————————————————————

//主函数————————————————————
int main(int arg, char *argv[])
{
    //网络连接————————————————————
    int listen_fd; //监听套接字文件描述符

    //创建套接字,获取套接字文件描述符
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Failed to create the server's socket");
        exit(EXIT_FAILURE);
    }

    //设置套接字选项为可重用本地地址
    int reuse = 1;

    if ((setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))) == -1)
    {
        close(listen_fd);

        perror("Failed to set the socket's options");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serv_addr; //服务端网络信息结构体

    //初始化服务端网络信息结构体
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(SERV_PORT);

    //绑定套接字与网络信息
    if ((bind(listen_fd, (struct sockaddr *)(&serv_addr), sizeof(serv_addr))) == -1)
    {
        close(listen_fd);

        perror("Failed to bind the socket");
        exit(EXIT_FAILURE);
    }

    //套接字设置被动监听状态
    if ((listen(listen_fd, LISTEN_MAX_COUNT)) == -1)
    {
        close(listen_fd);

        perror("Failed to configure the socket's listening status");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in clie_addr; //客户端网络信息结构体
    int addr_size;                //网络信息结构体大小
    int connect_fd;               //连接套接字文件描述符

    bzero(&clie_addr, sizeof(clie_addr));
    addr_size = sizeof(struct sockaddr);

    //与客户端建立连接
    if ((connect_fd = accept(listen_fd, (struct sockaddr *)(&clie_addr), &addr_size)) == -1)
    {
        close(connect_fd);
        close(listen_fd);

        perror("Failed to accept connection");
        exit(EXIT_FAILURE);
    }

    //传输数据————————————————————
    char msg_recv[BUFF_SIZE]; //接收客户端消息缓冲区
    char msg_send[BUFF_SIZE]; //发送客户端消息缓冲区

    bzero(&msg_recv, sizeof(*msg_recv));
    bzero(&msg_send, sizeof(*msg_send));

    if ((recv(connect_fd, msg_recv, BUFF_SIZE, 0)) < 0) //接收数据
    {
        close(connect_fd);

        perror("Failed to receive messages from the server");
        exit(EXIT_FAILURE);
    }

    printf("%s\n", msg_recv); //接收的消息

    if ((send(connect_fd, "YES", BUFF_SIZE, 0)) == -1) //发送数据
    {
        close(connect_fd);

        perror("Failed to send messages to the server");
        exit(EXIT_FAILURE);
    }

    if ((recv(connect_fd, msg_recv, BUFF_SIZE, 0)) < 0) //接收数据
    {
        close(connect_fd);

        perror("Failed to receive messages from the server");
        exit(EXIT_FAILURE);
    }

    printf("%s\n", msg_recv); //接收的消息

    if ((send(connect_fd, "ACK", BUFF_SIZE, 0)) == -1) //发送数据
    {
        close(connect_fd);

        perror("Failed to send messages to the server");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        /* code */
    }

    //关闭套接字文件描述符
    close(connect_fd);
    close(listen_fd);

    return 0;
}

//函数定义————————————————————
