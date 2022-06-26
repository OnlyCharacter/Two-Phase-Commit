#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#define N 1024

int login(struct sockaddr_in);
int showFunction();
int interactFunction(char *, int);
int whatToDoAfterRetn(int, int, int, char *);

int main(){
    struct sockaddr_in addr;        //目标服务器地址
    int len;
    int opt;
    int connect_socket = 0;         //通信套接字
    int i, j;
    int go;

    char commd[N];                  //发出的指令
    int retn;                       //接受的返回值
    char rest[1024];
    char buff[1024];                //接收返回值的buff

    bzero(&addr, sizeof(addr));     
    addr.sin_family = AF_INET;      //AF_INET代表TCP／IP协议
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(/* 服务器端口 */ 3333);
    len = sizeof(addr);

    LO:                             //尝试登录，2s周期
    connect_socket = login(addr);   //此处返回了通信套接字
    if(connect_socket <= 0){
        printf("连接失败，重新连接...\n");
        sleep(2);
        goto LO;
    }
    printf("已连接至服务器!\n");

    // ====== 功能区 ==========================================================================
    START:
    opt = showFunction();   //给出功能菜单，获取选项
    
    bzero(commd, 1024);
    go = interactFunction(commd, opt);
    if(go < 0){
        printf("选项错误!\n");
        goto START;
    }
    else if(go == 4){
        printf("退出!\n");
        goto END;
    }
    printf("指令: %s\n", commd);

    if(write(connect_socket, commd, strlen(commd)+1) < 0){    //发出指令
        printf("Write Error!\n");
    }

    bzero(rest, 1024);
    if(recv(connect_socket, rest, 1024, 0) < 0){       //等待一个返回字符串
        printf("Read Error!\n");
    }
    printf("结果: %s\n\n", rest);

    //whatToDoAfterRetn(connect_socket, opt, retn, buff);     //对于所接收的服务器返回值，作处理
    sleep(1);
    goto START;
    // ====== 功能区结束 =======================================================================
    END:
    printf("客户端终止\n");
    if(write(connect_socket, "QUIT", strlen("QUIT")) < 0){
        printf("Write Error!\n");
    }

    close(connect_socket);
    return 0;
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

int showFunction(){    //输出功能目录，并获取和返回选项
    int opt;

    RE:
    printf("=== 功能选择 ===\n");
    printf("1. GET\n");
    printf("2. PUT\n");
    printf("3. DEL\n");
    printf("4. 退出\n");
    printf("请选择方法:");
    scanf("%d", &opt);

    if(opt != 1 && opt != 2 && opt != 3 && opt != 4){
        printf("选项不存在，请重新输入！\n");
        goto RE;
    }

    return opt;
}

int interactFunction(char *c, int opt){    //接收具体指令 c: 要将指令写入的位置 opt: 之前所接收的选项
    switch (opt)
    {
    case 1:{
        char temp[20];
        bzero(temp, 20);
        printf("请输入需要GET的key值:");
        scanf("%s", temp);

        strcat(c, "GET ");
        strcat(c, temp);
        return 1;
        }
        break;
    
    case 2:{
        char tempkey[20];
        char tempvalue[20];
        bzero(tempkey, 20);
        bzero(tempvalue, 20);

        printf("请输入需要PUT的key值:");
        scanf("%s", tempkey);
        getchar();
        printf("请输入需要PUT的value值:");
        memset(tempvalue, 0, 20);
        fgets(tempvalue, 20, stdin);
        tempvalue[strlen(tempvalue) - 1] = '\0';
        printf("tempvalue:%s\n", tempvalue);
        // scanf("%s", tempvalue);

        strcat(c, "PUT ");
        strcat(c, tempkey);
        strcat(c, " ");
        strcat(c, tempvalue);
        return 2;
        }
        break;

    case 3:{
        char temp[20];
        bzero(temp, 20);
        printf("请输入需要DEL的key值:");
        scanf("%s", temp);

        strcat(c, "DEL ");
        strcat(c, temp);
        return 3;
        }
        break;

    case 4:{
        return 4;
    }
    
    default:{
        return -1;
        }
        break;
    }
}