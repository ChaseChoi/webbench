/* $Id: socket.c 1.1 1995/01/01 07:11:14 cthuang Exp $
 *
 * This module has been modified by Radim Kolar for OS/2 emx
 */

/***********************************************************************
  module:       socket.c
  program:      popclient
  SCCS ID:      @(#)socket.c    1.5  4/1/94
  programmer:   Virginia Tech Computing Center
  compiler:     DEC RISC C compiler (Ultrix 4.1)
  environment:  DEC Ultrix 4.3 
  description:  UNIX sockets code.
 ***********************************************************************/
 
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

//功能:获取socket descriptor
int Socket(const char *host, int clientPort)
{
    int sock; //socket descriptor获取“描述符”是关键
    unsigned long inaddr;
    struct sockaddr_in ad; //存储目标主机的相关信息
    struct hostent *hp;  //gethostbyname返回值类型, h_addr存储了DNS服务获取的IP地址
    
    memset(&ad, 0, sizeof(ad));//sin_zero[8] 设置为全0
    ad.sin_family = AF_INET; //IPv4

    inaddr = inet_addr(host); //返回Network Byte Order 无需htonl()做转换;使用inet_aton()更简洁,但个别平台没有该函数
    
    //对*host IP地址/域名 进行区分处理
    if (inaddr != INADDR_NONE)  //差错检测很重要,出错了(return -1)直接用变成广播地址 (unsigned)(-1)=>255.255.255.255
        memcpy(&ad.sin_addr, &inaddr, sizeof(inaddr));
    else
    {
        hp = gethostbyname(host);  //DNS获取IP address
        if (hp == NULL) 
            return -1; 
        memcpy(&ad.sin_addr, hp->h_addr, hp->h_length); //Network Byte Order, 无需类型转换
    }
    ad.sin_port = htons(clientPort); //host to network short 类型转换
    
    sock = socket(AF_INET, SOCK_STREAM, 0); //获取 socket descriptor; AF_INET 使用IPv4
    if (sock < 0)   //socket() return -1 on error
        return sock;
    if (connect(sock, (struct sockaddr *)&ad, sizeof(ad)) < 0)  //sockaddr_in 记得转换类型, error-> -1 success->0
        return -1;  //连接出错 
    return sock;
}

