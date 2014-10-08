//
//  MessageManager.cpp
//  SocketClient
//
//  Created by 王 欢 on 14-4-12.
//  Copyright (c) 2014年 王 欢. All rights reserved.
//

#include "MessageManager.h"

namespace net{
    
    MessageManager *MessageManager::messageManager = NULL;

    void MessageManager::connect(){
        network.connect();
    }

    void MessageManager::disconnect(){
        network.disconnect();
        reader.reset();
        writer.reset();
    }

    void MessageManager::run(){
        network.run();
    }

    void MessageManager::sendMessageFromLua(lua_State *lua){
        const char *msgId = getMessageIdFromLua(lua);
        size_t msgLen = parser.luaToBinary(lua);
        sendMessage(msgId,parser.getSendBuffer(), msgLen);
    }

    void MessageManager::sendMessage(const char *msgId ,const void *data , size_t len){
        writer.write(msgId, data, len);
    }
    
    const char *MessageManager::getMessageIdFromLua(lua_State *lua){
        lua_pushstring(lua,"messageId");
        lua_gettable(lua, 1);
        const char *messageId = lua_tostring(lua, -1);
        lua_settop(lua,1);
        return messageId;
    }

    void MessageManager::setLuaState(lua_State *L){
        lua = L;
    }

    lua_State *MessageManager::getLuaState(){
        return lua;
    }

    MessageWriter& MessageManager::getWriter(){
        return writer;
    }

    MessageReader& MessageManager::getReader(){
        return reader;
    }

    MessageParser& MessageManager::getParser(){
        return parser;
    }
}