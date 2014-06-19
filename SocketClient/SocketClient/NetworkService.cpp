//
//  NetworkService.cpp
//  SocketClient
//
//  Created by 王 欢 on 14-4-12.
//  Copyright (c) 2014年 王 欢. All rights reserved.
//

#include "NetworkService.h"
#include "MessageManager.h"

int NetworkService::noblockConnect(int sockfd, const SA *saptr, socklen_t salen, int nsec){
	int				flags, n, error;
	socklen_t		len;
	fd_set			rset, wset;
	struct timeval	tval;
    
    //使用无阻塞connect，用于控制连接超时
	flags = Fcntl(sockfd, F_GETFL, 0);
	Fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    
	error = 0;
    //使用系统的connect
    using ::connect;
	if ( (n = connect(sockfd, saptr, salen)) < 0)
		if (errno != EINPROGRESS)
			return(-1);
    
    //当客户端和服务器端在同一个host上会立刻返回成功(0)，而不会返回EINPROGRESS系统错误
	if (n != 0){
        FD_ZERO(&rset);
        FD_SET(sockfd, &rset);
        wset = rset;
        tval.tv_sec = nsec;
        tval.tv_usec = 0;
        
        //连接超时
        if ( (n = Select(sockfd+1, &rset, &wset, NULL,
                         nsec ? &tval : NULL)) == 0) {
            close(sockfd);
            errno = ETIMEDOUT;
            printf("connect time out for %d seconds set\n",nsec);
            return(-1);
        }
        
        if (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset)) {
            len = sizeof(error);
            if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
                return(-1);	
        } else
            printf("select error: sockfd not set\n");
    }
    
 	if (error) {
		close(sockfd);
		errno = error;
        switch (error) {
            case ECONNREFUSED:
                printf("server is not listening\n");
                break;
        }
		return(-1);
	}
	return(0);
}

void NetworkService::connect(){
    sockfd = Socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
        
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    Inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);
    //connect超时的功能需要通过无阻塞connet来实现
    if(noblockConnect(sockfd, (SA*)&servaddr, sizeof(servaddr),5) < 0){
        printf("connect fail!\n");
    }
}

void NetworkService::disconnect(){
    close(sockfd);
}

void NetworkService::run(){
    int maxfd = 0;
    fd_set rset , wset;
    
    FD_ZERO(&rset);
    FD_ZERO(&wset);
    //当没有socket不可读写则select立刻返回不阻塞。因为select在主线程中会阻塞主循环
    struct timeval time = {0,0};
    //实际项目中此处不会使用for，而是每一帧会调用run
    for (; ;) {
        
        FD_SET(sockfd, &rset);
        FD_SET(sockfd, &wset);
        maxfd = sockfd + 1;
        select(maxfd, &rset, &wset, NULL, &time);
        
        if (FD_ISSET(sockfd, &rset)) {
            recv();
        }
        
        if (FD_ISSET(sockfd, &wset)) {
            send();
        }
        
    }
}

size_t NetworkService::send(){
    MessageWriter &writer = MessageManager::getInstance()->getWriter();
    //此时用户没有需要发送的数据
    if (writer.isAvaliable()) {
        return 0;
    }
    
    char *buffer = writer.getBuffer();
    size_t dataSize = writer.getDataLength();
    //writer的buffer上没有发送完的数据
    size_t nleft = MESSAGE_INDICATOR_LENTH + dataSize - writer.getHasSend();
    //如果当前发送的数据量大于发送缓冲区则需要递归调用自己，尽量将所有要发送的数据在一帧之内发送出去
    bool needContinueSend = false;
    if (nleft > SEND_BUFFER_SIZE) {
        nleft = SEND_BUFFER_SIZE;
        needContinueSend = true;
    }
    memcpy(sendBuffer, buffer + writer.getHasSend(), nleft);
    size_t totalSend = 0;
    size_t actuallySend = writen(sockfd, sendBuffer,nleft);
    totalSend += actuallySend;
    if ((int)actuallySend >= 0) {
        writer.setHasSend(actuallySend);
    }

    //当发送缓冲区已满由于不能将数据在这样的状态下发送出去，那么在此时要发送的数据会始终大于SEND_BUFFER_SIZE
    //这样needContinueSend始终为true则会出现无限循环，所以当发送缓冲区已满的时候就退出函数，不会进入递归
    if (needContinueSend && !isSockSendBufferFull()) {
        totalSend += send();
    }
    //返回可能通过多次递归发送后总的发送数据量
    std::cout << "totalSend send :" << totalSend << std::endl;
    return totalSend;
}

size_t NetworkService::recv(){
    MessageReader &reader = MessageManager::getInstance()->getReader();
    char *buffer = reader.getBuffer();
    
    //开始进行一个完整数据的读取
    if (reader.getDataLength() == 0 && reader.isAvailable()) {
        int msgLen = getMessageLen(sockfd);
        if (msgLen > 0) {
            reader.setDataLength(msgLen);
            reader.setAvailable(false);
        }
    }
    //由于配置了无阻塞socket，有可能出现虽然select认为socket可读，但是实际无数据的情况
    //这个时候会返回EWOULDBLOCK错误。
    if (reader.isAvailable()) {
        return 0;
    }
    
    size_t dataSize = reader.getDataLength();
    //注意这个地方由于在51行处获取消息的长度已经将indicator读取出来了
    bool needContinueRecv = false;
    size_t nleft = dataSize - reader.getHasRecv();
    if (nleft > RECV_BUFFER_SIZE) {
        nleft = RECV_BUFFER_SIZE;
        needContinueRecv = true;
    }
    size_t totoalRecv = 0;
    size_t readCont = readn(sockfd, recvBuffer, nleft);
    totoalRecv += readCont;
    if ((int)readCont < 0) {
        printf("readn error\n");
    }
    if ((int)readCont > 0) {
        memcpy(buffer + reader.getHasRecv(), recvBuffer , readCont);
        reader.setHasRecv(readCont);
    }
    
    if (needContinueRecv && !isSockRecvBufferFull()) {
        totoalRecv += recv();
    }

    return totoalRecv;
}

int NetworkService::getMessageLen(int connfd){
    char msgLenBuffer[MESSAGE_INDICATOR_LENTH];
    int ret = (int)readn(connfd, msgLenBuffer, MESSAGE_INDICATOR_LENTH);
    if (ret > 0) {
        return atoi(msgLenBuffer);
    }
    else{
        return 0;
    }
}

ssize_t	NetworkService::writen(int fd, const void *vptr, size_t nleft){
	ssize_t	nwritten;
    if ( (nwritten = write(fd, vptr, nleft)) <= 0) {
        //此时socket发送缓冲区已满
        if (errno == EWOULDBLOCK) {
            setSockSendBufferFull(true);
        }
        else{
            setSockSendBufferFull(false);
        }
        
        if (nwritten < 0 && (errno == EINTR || errno == EWOULDBLOCK)){
                nwritten = 0;		
        }
        else{
            return(-1);			
        }
    }
    
	return(nwritten);
}
						
ssize_t NetworkService::readn(int fd, void *vptr, size_t nleft){
	ssize_t	nread;
    char *ptr = (char*)vptr;
    
    if ( (nread = read(fd, ptr, nleft)) < 0) {
        //此时socket接收缓冲区已满
        if (errno == EWOULDBLOCK) {
            setSockRecvBufferFull(true);
        }
        else{
            setSockRecvBufferFull(false);
        }

        if (errno == EINTR || errno == EWOULDBLOCK)
            nread = 0;		
        else
            return(-1);
    }
    //EOF,服务器端关闭了socket
    else if (nread == 0)
        return 0;				
	
	return(nread);
}

bool NetworkService::isSockSendBufferFull(){
    return sockSendBufferFull;
}

void NetworkService::setSockSendBufferFull(bool b){
    sockSendBufferFull = b;
}

bool NetworkService::isSockRecvBufferFull(){
    return sockRecvBufferFull;
}

void NetworkService::setSockRecvBufferFull(bool b){
    sockRecvBufferFull = b;
}
