//头文件————————————————————
#include <netinet/in.h> //sockaddr_in、htonl()、htons()
#include <sys/socket.h> //socket()、connect()、send()、recv()
#include <stdio.h>      //perror()
#include <stdlib.h>     //exit()、EXIT_FAILURE
#include <string.h>     //bzero()、strncpy()
#include <unistd.h>     //close()

//宏————————————————————
#define SERV_PORT 3334
#define BUFF_SIZE 64 //传递消息缓冲区大小

//函数声明————————————————————

//主函数————————————————————
int main(int argc, char *argv[])
{
    //网络连接————————————————————
    int sock_fd; //套接字文件描述符

    //创建套接字并获取套接字文件描述符
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Failed to create the client's socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serv_addr; //服务端网络信息结构体

    //初始化服务端网络信息结构体
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(SERV_PORT);

    //与服务端建立连接
    if ((connect(sock_fd, (struct sockaddr *)(&serv_addr), sizeof(serv_addr))) == -1)
    {
        close(sock_fd);

        perror("Failed to establish connection");
        exit(EXIT_FAILURE);
    }

    //数据传输
    char msg_send[BUFF_SIZE]; //发送服务端消息缓冲区
    char msg_recv[BUFF_SIZE]; //接收服务端消息缓冲区

    bzero(&msg_recv, sizeof(*msg_recv));
    bzero(&msg_send, sizeof(*msg_send));


    if ((recv(sock_fd, msg_recv, BUFF_SIZE, 0)) < 0) //接收数据
    {
        close(sock_fd);

        perror("Failed to receive messages from the server");
        exit(EXIT_FAILURE);
    }

    printf("%s\n", msg_recv); //接收的消息

    if ((send(sock_fd,"YES", BUFF_SIZE, 0)) == -1) //发送数据
    {
        close(sock_fd);

        perror("Failed to send messages to the server");
        exit(EXIT_FAILURE);
    }

    if ((recv(sock_fd, msg_recv, BUFF_SIZE, 0)) < 0) //接收数据
    {
        close(sock_fd);

        perror("Failed to receive messages from the server");
        exit(EXIT_FAILURE);
    }

    printf("%s\n", msg_recv); //接收的消息

    if ((send(sock_fd, "ACK", BUFF_SIZE, 0)) == -1) //发送数据
    {
        close(sock_fd);

        perror("Failed to send messages to the server");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        /* code */
    }

    //关闭套接字文件描述符
    close(sock_fd);

    return 0;
}

//函数定义————————————————————
