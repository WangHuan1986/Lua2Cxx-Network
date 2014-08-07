//
//  MessageWriter.cpp
//  SocketClient
//
//  Created by 王 欢 on 14-4-12.
//  Copyright (c) 2014年 王 欢. All rights reserved.
//

#include "MessageManager.h"
#include "MessageWriter.h"
namespace net{
    extern const int MESSAGE_INDICATOR_LENTH;

    void MessageWriter::write(const void* data , size_t len){
        //如果当前正处于发送状态或是要求发送的数据为0
        if (!isAvaliable() || len == 0) {
            return;
        }
        
        int bufferIndex = 0;
        //预留了4个字节的位置用于存放标识消息整体长度的信息
        bufferIndex += MESSAGE_INDICATOR_LENTH;
        
        //拷贝message数据
        memcpy(buffer + bufferIndex, data, len);
        bufferIndex += len;
        //拷贝消息的整体长度
        //这里分配了5个字节大小是因为sprintf函数会在最后补\0
        char indicator[5];
        sprintf(indicator, "%04d",bufferIndex - MESSAGE_INDICATOR_LENTH);
        memcpy(buffer, indicator, MESSAGE_INDICATOR_LENTH);
        //消息的长度不包含indicator本身的长度，即len指的是消息体的长度
        setDataLength(bufferIndex - MESSAGE_INDICATOR_LENTH);

        setAvaliable(false);
    }

    char * MessageWriter::getBuffer(){
        return buffer;
    }

    void MessageWriter::setDataLength(size_t len){
        dataLength = len;
    }

    size_t MessageWriter::getDataLength(){
        return dataLength;
    }

    void MessageWriter::setHasSend(size_t thisTimeSend){
        size_t totalSend = getHasSend() + thisTimeSend;
        size_t needSend = dataLength + MESSAGE_INDICATOR_LENTH;
        
        if (totalSend < needSend && !isAvaliable()) {
            hasSend = totalSend;
        }
        if (totalSend == needSend) {
            reset();
        }
    }

    size_t MessageWriter::getHasSend(){
        return hasSend;
    }

    void MessageWriter::reset(){
        dataLength = 0;
        hasSend = 0;
        available = true;
    }

    bool MessageWriter::isAvaliable(){
        return available;
    }

    void MessageWriter::setAvaliable(bool b){
        available = b;
    }
}
