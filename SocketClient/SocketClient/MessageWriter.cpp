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
    
    extern const int MESSAGE_INDICATOR_LENGTH;
    extern const int MESSAGE_ID_LENGTH;
    
    //发送的消息格式为 xxxx|xx ... xxx ...
    //4个字节的头部代表整个消息体的长度(不包含头部本身)，头部后边是消息体，消息体的前2个字节为消息id
    void MessageWriter::write(const char *msgId,const void* data,size_t len){
        //如果当前正处于发送状态或是要求发送的数据为0
        if (!isAvaliable() || len == 0) {
            return;
        }
        
        int bufferIndex = 0;
        //这里分配了5个字节大小是因为sprintf函数会在最后补\0
        char indicator[5];
        sprintf(indicator, "%04d",(int)len + MESSAGE_ID_LENGTH);
        //4个字节的信息用于存放标识消息整体长度的字符串
        memcpy(buffer, indicator, MESSAGE_INDICATOR_LENGTH);
        bufferIndex += MESSAGE_INDICATOR_LENGTH;
        //2个字节的messageId，获取此数据的时候将其转换为short类型
        short messageId = atoi(msgId);
        memcpy(buffer + bufferIndex,&messageId,MESSAGE_ID_LENGTH);
        bufferIndex += MESSAGE_ID_LENGTH;
        //拷贝消息体数据
        memcpy(buffer + bufferIndex, data, len);
        
        //消息的长度不包含indicator本身的长度，即len指的是消息体的长度
        setDataLength(len + MESSAGE_ID_LENGTH);
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
        size_t needSend = dataLength + MESSAGE_INDICATOR_LENGTH;
        
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
