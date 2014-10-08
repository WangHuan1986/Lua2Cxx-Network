//
//  MessageManager.h
//  SocketClient
//
//  Created by 王 欢 on 14-4-12.
//  Copyright (c) 2014年 王 欢. All rights reserved.
//

#ifndef __SocketClient_v2__MessageManager__
#define __SocketClient_v2__MessageManager__

#include <iostream>

extern "C" {
    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"
}
#include "MessageConfig.h"
#include "MessageWriter.h"
#include "MessageReader.h"
#include "NetworkService.h"
#include "MessageParser.h"

namespace net{

    extern const int MESSAGE_INDICATOR_LENGTH;
    extern const int MESSAGE_ID_LENGTH;

    class MessageManager{
    public:
        static MessageManager* getInstance(){
            if (messageManager == NULL) {
                messageManager = new MessageManager;
            }
            return messageManager;
        }
        void connect();
        void disconnect();
        void run();
        void sendMessage(const char *,const void* , size_t len);
        void sendMessageFromLua(lua_State *lua);
    public:
        MessageWriter& getWriter();
        MessageReader& getReader();
        MessageParser& getParser();
        void setLuaState(lua_State *);
        lua_State *getLuaState();
    private:
        MessageManager():lua(NULL){}
        const char *getMessageIdFromLua(lua_State *);
        static MessageManager *messageManager;
        NetworkService network;
        MessageWriter writer;
        MessageReader reader;
        MessageParser parser;
        lua_State *lua;
        std::string messageProtocalPath;
    };
}
#endif /* defined(__SocketClient__MessageManager__) */
