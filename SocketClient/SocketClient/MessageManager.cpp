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
//        messageProtocalPath = cocos2d::FileUtils::getInstance()->fullPathForFilename("messageProtocol/TC_0.xml");
        network.run();
    }

    void MessageManager::sendMessageFromLua(lua_State *lua){
        size_t msgLen = parser.luaToBinary(lua);
        sendMessage(parser.getSendBuffer(), msgLen);
    }

    void MessageManager::sendMessage(const void *data , size_t len){
        writer.write(data,len);
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