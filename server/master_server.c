//头文件————————————————————
#include <sys/socket.h> //socket()、setsockopt()、bind()、listen()、accept()、recv()、send()
#include <stdio.h>      //perror()
#include <stdlib.h>     //exit()
#include <unistd.h>     //close()、fork()
#include <netinet/in.h> //sockaddr_in、htonl()、htons()
#include <string.h>     //bzero()、strstr()

//宏————————————————————
#define SERV_PORT 3333     //服务端端口号
#define LISTEN_MAX_COUNT 4 //所监听的最大连接数
#define REPL_SERV_COUNT 2  //从服务端数量
#define BUFF_SIZE 64       //传递消息缓冲区大小

//全局变量
int g_repl_serv_fds[REPL_SERV_COUNT] = {-1}; //与从服务端的连接套接字文件描述符集

//函数声明————————————————————
void clie_handle(int connect_fd);                       //客户端处理
void two_phase_commit(int connect_fd, char recv_msg[]); //两阶段提交
void get(int connect_fd, char recv_msg[]);              //获取

//主函数————————————————————
int main(int argc, char *argv[])
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

    struct sockaddr_in mast_serv_addr; //主服务端网络信息结构体

    //初始化主服务端网络信息结构体
    bzero(&mast_serv_addr, sizeof(mast_serv_addr));
    mast_serv_addr.sin_family = AF_INET;
    mast_serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    mast_serv_addr.sin_port = htons(SERV_PORT);

    //绑定套接字与网络信息
    if ((bind(listen_fd, (struct sockaddr *)(&mast_serv_addr), sizeof(mast_serv_addr))) == -1)
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

    struct sockaddr_in repl_serv_addr; //从服务端网络信息结构体
    int addr_size;                     //网络信息结构体大小

    bzero(&repl_serv_addr, sizeof(repl_serv_addr));
    addr_size = sizeof(struct sockaddr);

    //循环监听从服务端请求
    for (int i = 0; i < REPL_SERV_COUNT; ++i)
    {
        //与从服务端建立连接
        if ((g_repl_serv_fds[i] = accept(listen_fd, (struct sockaddr *)(&repl_serv_addr), &addr_size)) == 0)
        {
            perror("Failed to accept connection");
            exit(EXIT_FAILURE);
        }
    }

    int connect_fd = -1;          //连接套接字文件描述符
    struct sockaddr_in clie_addr; //客户端网络信息结构体
    int pid = -1;                 //进程标识符

    bzero(&clie_addr, sizeof(clie_addr));

    //循环监听客户端请求
    while (1)
    {
        //与客户端建立连接
        if ((connect_fd = accept(listen_fd, (struct sockaddr *)(&clie_addr), &addr_size)) == 0)
        {
            perror("Failed to accept connection");
            exit(EXIT_FAILURE);
        }

        //使用子进程处理
        if ((pid = fork()) == 0)
        {
            close(listen_fd); //关闭不需要的

            clie_handle(connect_fd); //客户端处理
        }
    }

    //关闭套接字文件描述符
    for (int i = 0; i < REPL_SERV_COUNT; ++i)
    {
        close(g_repl_serv_fds[i]);
    }
    close(listen_fd);

    return 0;
}

//函数定义————————————————————
//客户端处理
void clie_handle(int connect_fd)
{
    char recv_msg[BUFF_SIZE]; //接收客户端消息缓冲区

    //循环接收客户端消息并处理
    while (1)
    {
        bzero(recv_msg, sizeof(*recv_msg));

        //接收客户端消息
        if (recv(connect_fd, recv_msg, BUFF_SIZE, 0) < 0) //可用read()
        {
            perror("Failed to receive command\n");
            exit(EXIT_FAILURE);
        }

        //处理
        if ((strstr(recv_msg, "PUT")) != NULL) // PUT请求
        {
            two_phase_commit(connect_fd, recv_msg); //两阶段提交
        }
        else if ((strstr(recv_msg, "DEL")) != NULL) // DEL请求
        {
            two_phase_commit(connect_fd, recv_msg); //两阶段提交
        }
        else if ((strstr(recv_msg, "GET")) != NULL) // GET请求
        {
            get(connect_fd, recv_msg); //获取
        }
    }

    return;
}

//两阶段提交
void two_phase_commit(int connect_fd, char recv_msg[])
{
    if ((strstr(recv_msg, "PUT")) != NULL) // PUT请求
    {
        //对每个从服务端发送PUT请求
        for (int i = 0; i < REPL_SERV_COUNT; ++i)
        {
            if ((send(g_repl_serv_fds[i], recv_msg, BUFF_SIZE, 0)) == -1) //发送数据
            {
                perror("Failed to send messages to the replica server");
                exit(EXIT_FAILURE);
            }
        }
    }

    return;
}

//获取
void get(int connect_fd, char recv_msg[])
{
    if ((send(connect_fd, recv_msg, BUFF_SIZE, 0)) == -1) //发送数据
    {
        perror("Failed to send messages to the client");
        exit(EXIT_FAILURE);
    }

    return;
}

// //使用子进程处理
// if ((pid = fork()) == 0)
// {
//     close(listen_fd); //关闭不需要的

//     repl_serv_handle(g_repl_serv_fds[i]); //从服务端处理
// }

// //从服务端处理
// void repl_serv_handle(int connect_fd)
// {
//     return;
// }

// void repl_serv_handle(int connect_fd); //从服务端处理