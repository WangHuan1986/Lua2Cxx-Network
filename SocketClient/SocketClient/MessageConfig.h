//
//  MessageConfig.h
//  SocketClient
//
//  Created by 王 欢 on 14-10-8.
//  Copyright (c) 2014年 王 欢. All rights reserved.
//

#ifndef SocketClient_MessageConfig_h
#define SocketClient_MessageConfig_h

namespace net{
    
    //消息头部大小
    const int MESSAGE_INDICATOR_LENGTH = 4;
    //消息ID所占用的字节数
    const int MESSAGE_ID_LENGTH = 2;
    const int MESSAGE_READER_BUFFER_SIZE = 1024;
    const int MESSAGE_WRITER_BUFFER_SIZE = 1024;
    const int MESSAGE_SOCKET_SEND_BUFFER_SIZE = 1024;
    const int MESSAGE_SOCKET_RECV_BUFFER_SIZE = 1024;
    const int MESSAGE_PARSER_SEND_BUFFER_SIZE = 1024;
    //消息协议路径
    const char * const MESSAGE_PROTOCOL_PATH = "/Users/wanghuan/mine/Lua2Cxx-Network/SocketClient/SocketClient/messageProtocol/messageProtocol.xml";
    //服务器IP地址
    const char * const SERVER_IP = "127.0.0.1";
    //服务器端口
    const int SERVER_PORT = 9877;
    
}

#endif
