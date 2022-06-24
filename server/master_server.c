//头文件————————————————————
#include <sys/socket.h> //socket()、setsockopt()、bind()、listen()、accept()、recv()、send()
#include <stdio.h>      //perror()
#include <stdlib.h>     //exit()
#include <unistd.h>     //close()、fork()
#include <netinet/in.h> //sockaddr_in、htonl()、htons()
#include <string.h>     //bzero()、strstr()、strncpy()
#include <arpa/inet.h>
//宏————————————————————
#define SERV_PORT 3333     //主服务端端口号
#define LISTEN_MAX_COUNT 2 //所监听的客户端最大连接数
#define BUFF_SIZE 64       //传递消息缓冲区大小
#define REPL_SERV_COUNT 2  //从服务端数量

//全局变量
int g_repl_serv_fds[REPL_SERV_COUNT] = {-1}; //与从服务端的连接套接字文件描述符集

//函数声明————————————————————
void clie_handle(int connect_fd);                       //客户端处理
void two_phase_commit(int connect_fd, char recv_msg[]); //两阶段提交
void get(int connect_fd, char recv_msg[]);              //获取

void conn_reli_serv();        //连接从服务端
void rollback(int ack_count); //回滚

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

    int connect_fd = -1;          //连接套接字文件描述符
    struct sockaddr_in clie_addr; //客户端网络信息结构体
    socklen_t addr_size;                //网络信息结构体大小
    int pid = -1;                 //进程标识符

    bzero(&clie_addr, sizeof(clie_addr));
    addr_size = sizeof(struct sockaddr);

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
            close(listen_fd); //关闭不需要的 不关闭需要的

            clie_handle(connect_fd); //客户端处理
        }
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
        if (recv(connect_fd, recv_msg, BUFF_SIZE, 0) < 0)
        {
            perror("Failed to receive command\n");
            continue; //循环接收
        }
        printf("RECV: %s\n", recv_msg);
        //处理
        if ((strstr(recv_msg, "PUT")) != NULL) // PUT请求
        {
            printf("Enter PUT\n");
            two_phase_commit(connect_fd, recv_msg); //两阶段提交
        }
        else if ((strstr(recv_msg, "DEL")) != NULL) // DEL请求
        {
            printf("Enter DEL\n");
            two_phase_commit(connect_fd, recv_msg); //两阶段提交
        }
        else if ((strstr(recv_msg, "GET")) != NULL) // GET请求
        {   
            printf("Enter GET\n");
            get(connect_fd, recv_msg); //获取
        }
        else if ((strstr(recv_msg, "QUIT")) != NULL) // QUIT请求
        {
            printf("Enter QUIT\n");
            close(connect_fd);  //关闭套接字
            exit(EXIT_SUCCESS); //退出进程
        }
    }

    return;
}

//两阶段提交
void two_phase_commit(int connect_fd, char recv_msg[])
{
    //连接从服务端
    conn_reli_serv();
    printf("connect successfully\n");
    //对每个从服务端发送准备请求
    char send_msg[BUFF_SIZE]; //发送从服务端端消息缓冲区

    strncpy(send_msg, recv_msg, BUFF_SIZE); //发送从服务端消息为接收客户端消息
    bzero(recv_msg, BUFF_SIZE);     //清空接收从服务端消息

    for (int i = 0; i < REPL_SERV_COUNT; ++i)
    {
        if ((send(g_repl_serv_fds[i], send_msg, BUFF_SIZE, 0)) == -1) //发送消息
        {
            perror("Failed to send messages to the replica server");
            return; //调用返回  重新接收客户端消息
        }

        //同步阻塞  因为只要有一个没有准备成功都不能进行
        if ((recv(g_repl_serv_fds[i], recv_msg, BUFF_SIZE, 0)) < 0) //接收消息
        {
            perror("Failed to receive messages from the replica server");
            return;
        }

        //准备失败  因为只要有一个准备失败都不能进行
        if ((strstr(recv_msg, "NO")) != NULL)
        {
            //发送客户端操作失败信息
            strncpy(send_msg, "The operation failure", BUFF_SIZE);

            if ((send(connect_fd, send_msg, BUFF_SIZE, 0)) == -1)
            {
                perror("Failed to send messages to the client");
                return;
            }

            //关闭与从服务端连接套接字文件描述符
            for (int i = 0; i < REPL_SERV_COUNT; ++i)
            {
                close(g_repl_serv_fds[i]);
            }

            return;
        }
    }

    //没有准备失败调用返回就是准备成功
    int ack_count = 0; //从服务端响应提交成功的数量
    snprintf(send_msg, 4, "YES");
    bzero(recv_msg, BUFF_SIZE);
    //对每个从服务端发送提交请求
    for (int i = 0; i < REPL_SERV_COUNT; ++i)
    {
        if ((send(g_repl_serv_fds[i], send_msg, BUFF_SIZE, 0)) == -1) //发送消息
        {
            perror("Failed to send messages to the replica server");
            return;
        }

        //同步阻塞  因为只要有一个提交失败都不能进行
        if ((recv(g_repl_serv_fds[i], recv_msg, BUFF_SIZE, 0)) < 0) //接收消息
        {
            perror("Failed to receive messages from the replica server");
            return;
        }

        if ((strstr(recv_msg, "NACK")) != NULL) //提交失败  因为只要有一个提交失败都不能进行
        {
            //回滚
            rollback(ack_count); //按从服务端提交成功的数量顺序回滚

            //发送客户端操作失败信息
            strncpy(send_msg, "The operation failure", BUFF_SIZE);

            if ((send(connect_fd, send_msg, BUFF_SIZE, 0)) == -1)
            {
                perror("Failed to send messages to the client");
                exit(EXIT_FAILURE);
            }

            //关闭与从服务端连接套接字文件描述符
            for (int i = 0; i < REPL_SERV_COUNT; ++i)
            {
                close(g_repl_serv_fds[i]);
            }

            return;
        }
        else //一个从客户段提交成功，顺序记录从服务端提交成功的数量
        {
            ++ack_count;
        }
    }

    //没有提交失败调用返回就是提交成功
    //发送客户端操作成功信息
    strncpy(send_msg, "The operation success", BUFF_SIZE);

    if ((send(connect_fd, send_msg, BUFF_SIZE, 0)) == -1)
    {
        perror("Failed to send messages to the client");
        return;
    }

    //关闭套接字文件描述符
    for (int i = 0; i < REPL_SERV_COUNT; ++i)
    {
        close(g_repl_serv_fds[i]);
    }

    return;
}

//获取
void get(int connect_fd, char recv_msg[])
{
    conn_reli_serv(); //连接从服务端
    printf("connect successfully\n");
    char send_msg[BUFF_SIZE]; //发送从服务端端消息缓冲区

    strncpy(send_msg, recv_msg, BUFF_SIZE); //发送从服务端消息为接收客户端消息
    bzero(recv_msg, BUFF_SIZE);     //清空接收从服务端消息

    //默认在第一个从服务端获取
    //发送从服务端消息
    printf("get send: %s\n", send_msg);
    if ((send(g_repl_serv_fds[0], send_msg, BUFF_SIZE, 0)) == -1)
    {
        perror("Failed to send messages to the replica server");
        return;
    }

    //接收从服务端消息  暂不考虑错误情况
    if (recv(g_repl_serv_fds[0], recv_msg, BUFF_SIZE, 0) < 0)
    {
        perror("Failed to receive messages from the replica server");
        return;
    }

    //发送客户端消息
    strncpy(send_msg, recv_msg, BUFF_SIZE); //发送客户端消息为接收从服务端消息

    if ((send(connect_fd, recv_msg, BUFF_SIZE, 0)) == -1)
    {
        perror("Failed to send messages to the client");
        return;
    }

    //关闭套接字文件描述符
    for (int i = 0; i < REPL_SERV_COUNT; ++i)
    {
        close(g_repl_serv_fds[i]);
    }

    return;
}

//连接从服务端
void conn_reli_serv()
{
    //循环连接从服务端
    for (int i = 0; i < REPL_SERV_COUNT; ++i)
    {
        //创建套接字并获取套接字文件描述符
        if ((g_repl_serv_fds[i] = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        {
            perror("Failed to create the server's socket");
            return;
        }

        struct sockaddr_in repl_serv_addr; //从服务端网络信息结构体
        int addr_size;                     //网络信息结构体大小

        bzero(&repl_serv_addr, sizeof(repl_serv_addr));
        addr_size = sizeof(struct sockaddr);

        //初始化从服务端网络信息结构体
        repl_serv_addr.sin_family = AF_INET;
        repl_serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        repl_serv_addr.sin_port = htons(SERV_PORT + 1 + i); //动态更新端口号

        //与从服务端建立连接
        if ((connect(g_repl_serv_fds[i], (struct sockaddr *)(&repl_serv_addr), addr_size)) == -1)
        {
            close(g_repl_serv_fds[i]);

            perror("Failed to establish connection");
            return;
        }
    }

    return;
}

//回滚
void rollback(int ack_count)
{
    char send_msg[BUFF_SIZE]; //发送从服务端端消息缓冲区

    strncpy(send_msg, "ROLLBACK", BUFF_SIZE); //发送从服务端回滚请求

    //按从服务端提交成功的数量顺序回滚
    for (int i = 0; i < ack_count; ++i)
    {
        if ((send(g_repl_serv_fds[i], send_msg, BUFF_SIZE, 0)) == -1) //发送消息
        {
            perror("Failed to send messages to the replica server");
            return; //调用返回  重新接收客户端消息
        }
    }

    //暂不考虑接收回滚成功、失败消息

    return;
}