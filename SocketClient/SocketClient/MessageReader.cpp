//
//  MessageReader.cpp
//  SocketClient
//
//  Created by 王 欢 on 14-4-13.
//  Copyright (c) 2014年 王 欢. All rights reserved.
//

#include "MessageReader.h"
#include "MessageManager.h"

namespace net{
    //当一条完整数据接收完毕后此方法会被调用
    void MessageReader::read(size_t n){
        printf("===== Message Body %d Byte =====\n",(int)n);
        std::cout << "" << std::endl;
        MessageManager::getInstance()->getParser().binaryToLua();
    }

    char * MessageReader::getBuffer(){
        return buffer;
    }

    void MessageReader::setDataLength(size_t len){
        dataLength = len;
    }

    size_t MessageReader::getDataLength(){
        return dataLength;
    }

    void MessageReader::setHasRecv(size_t thisTimeRecv){
        size_t totalRecv = getHasRecv() + thisTimeRecv;
        size_t needRecv = dataLength;
        hasRecv = totalRecv;
        if (totalRecv > dataLength) {
            err_sys("wrong length of message from server!");
        }
        //一条完整消息读取完毕
        if (totalRecv == needRecv) {
            read(getHasRecv());
            reset();
        }
    }

    size_t MessageReader::getHasRecv(){
        return hasRecv;
    }

    bool MessageReader::isAvailable(){
        return available;
    }

    void MessageReader::setAvailable(bool b){
        available = b;
    }

    void MessageReader::reset(){
        dataLength = 0;
        hasRecv = 0;
        available = true;
    }
}
