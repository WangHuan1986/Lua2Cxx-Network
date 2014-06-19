//
//  main.cpp
//  SocketServer
//
//  Created by 王 欢 on 14-3-20.
//  Copyright (c) 2014年 王 欢. All rights reserved.
//

#include <iostream>
extern "C" {
#include "unp.h"
}

const int MESSAGE_LENTH = 4;

int getMessageLen(int connfd){
    char msgLenBuffer[MESSAGE_LENTH];
    readn(connfd, msgLenBuffer, MESSAGE_LENTH);
    int len = atoi(msgLenBuffer);
    return len;
}

int main(int argc, const char * argv[])
{
    
    int listenedfd , connedfd;
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;
    char buffer[1024];
    char sendBuffer[1024];
    
    
    listenedfd = Socket(AF_INET,SOCK_STREAM,0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    Bind(listenedfd, (SA*)&servaddr, sizeof(servaddr));
    Listen(listenedfd, LISTENQ);
    clilen = sizeof(cliaddr);
again:
    connedfd = Accept(listenedfd, (SA*)&cliaddr, &clilen);
    for (;;) {
        int msgLen = getMessageLen(connedfd);
        if (msgLen == 0) {
            close(connedfd);
            goto again;
        }
        size_t readCont = readn(connedfd, buffer, msgLen);
        char msgLenStr[5];
        sprintf(msgLenStr, "%04d",msgLen);
        memcpy(sendBuffer,msgLenStr,MESSAGE_LENTH);
        memcpy(sendBuffer + MESSAGE_LENTH, buffer, msgLen);
        writen(connedfd, sendBuffer, MESSAGE_LENTH + readCont);
        
        printf("server will send %d byte in sendBuffer\n",(int)(MESSAGE_LENTH + readCont));
    }
    
    return 0;
}

