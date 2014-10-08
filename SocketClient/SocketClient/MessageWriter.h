//
//  MessageWriter.h
//  SocketClient
//
//  Created by 王 欢 on 14-4-12.
//  Copyright (c) 2014年 王 欢. All rights reserved.
//

#ifndef __SocketClient_v2__MessageWriter__
#define __SocketClient_v2__MessageWriter__

#include <iostream>
#include "MessageConfig.h"

namespace net{
    
    extern const int MESSAGE_WRITER_BUFFER_SIZE;
    
    class MessageManager;
    class MessageWriter{
    public:
        MessageWriter():
            available(true),
            hasSend(0),
            dataLength(0){}
        void write(const char *,const void* data , size_t len);
        char * getBuffer();
        size_t getDataLength();
        void setDataLength(size_t len);
        void setHasSend(size_t);
        size_t getHasSend();
        bool isAvaliable();
        void setAvaliable(bool b);
        void reset();//当数据发送完毕后重置数据成员以准备下一次发送
    private:
        char buffer[MESSAGE_WRITER_BUFFER_SIZE];
        bool available;
        size_t dataLength; //消息体长度，不包括indicator
        size_t hasSend; //实际发出去的字节数，包括indicator
    };
}
#endif /* defined(__SocketClient__MessageWriter__) */
