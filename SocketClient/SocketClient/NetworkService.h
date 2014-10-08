//
//  NetworkService.h
//  SocketClient
//
//  Created by 王 欢 on 14-4-12.
//  Copyright (c) 2014年 王 欢. All rights reserved.
//

#ifndef __SocketClient_v2__NetworkService__
#define __SocketClient_v2__NetworkService__

extern "C" {
#include "syslib.h"
}
#include "MessageConfig.h"

namespace net{

    extern const int MESSAGE_SOCKET_SEND_BUFFER_SIZE;
    extern const int MESSAGE_SOCKET_RECV_BUFFER_SIZE;
    extern const char * const SERVER_IP;
    extern const int SERVER_PORT;
    
    class MessageManager;
    class NetworkService{
    public:
        NetworkService():
            sockSendBufferFull(false){}
        void connect();
        void disconnect();
        void run();
        void update();
    private:
        int getMessageLen(int connfd);
        int noblockConnect(int, const SA*, socklen_t, int);
        ssize_t	writen(int fd, const void *vptr, size_t n);
        ssize_t readn(int fd, void *vptr, size_t n);
        size_t send();
        size_t recv();
        bool isSockSendBufferFull();
        void setSockSendBufferFull(bool);
        bool isSockRecvBufferFull();
        void setSockRecvBufferFull(bool);
    private:
        char sendBuffer[MESSAGE_SOCKET_SEND_BUFFER_SIZE];
        char recvBuffer[MESSAGE_SOCKET_RECV_BUFFER_SIZE];
        int sockfd;
        bool sockSendBufferFull;
        bool sockRecvBufferFull;
    };
}
#endif /* defined(__SocketClient__NetworkService__) */
