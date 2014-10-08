//
//  MessageReader.h
//  SocketClient
//
//  Created by 王 欢 on 14-4-13.
//  Copyright (c) 2014年 王 欢. All rights reserved.
//

#ifndef __SocketClient_v2__MessageReader__
#define __SocketClient_v2__MessageReader__

#include <iostream>
#include <map>
#include "MessageConfig.h"

namespace net{
    
    extern const int MESSAGE_READER_BUFFER_SIZE;
    
    class MessageManager;
    class MessageReader{
    public:
        MessageReader():
            available(true),
            hasRecv(0),
            dataLength(0){}
        void read(size_t);
    public:
        char * getBuffer();
        
        size_t getDataLength();
        void setDataLength(size_t len);
        void setHasRecv(size_t);
        size_t getHasRecv();
        bool isAvailable();
        void setAvailable(bool);
        void reset();//当数据发送完毕后重置数据成员以准备下一次接收
    private:
        char buffer[MESSAGE_READER_BUFFER_SIZE];
        bool available;
        size_t dataLength; //消息体长度，不包括indicator
        size_t hasRecv; //实际接收的字节数，包括indicator

    };
}
#endif /* defined(__SocketClient__MessageReader__) */
