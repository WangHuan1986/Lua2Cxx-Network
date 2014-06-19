//
//  main.cpp
//  SelectClient
//
//  Created by 王 欢 on 14-4-8.
//  Copyright (c) 2014年 王 欢. All rights reserved.
//

#include <iostream>

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include "MessageManager.h"

static int sendData(lua_State *lua){
    MessageManager::getInstance()->sendMessageFromLua(lua);
    return 0;
}

static const struct luaL_reg mylib [] = {
    {"sendData", sendData},
    {NULL, NULL}  /* sentinel */
};


int registerAll(lua_State* lua){
    
    luaL_register(lua, "c_myLib", mylib);
    return 0;
}

int main(int argc, const char * argv[])
{
    MessageManager::getInstance()->connect();    
    int iErr = 0;
    lua_State *lua = lua_open();  //创建一个lua运行环境
    luaL_openlibs(lua); //将lua中要用到的库函数暴露出去
    
    //将c++的接口暴露给lua
    registerAll(lua);
    MessageManager::getInstance()->setLuaState(lua);
    
    //加载编译Lua文件,并发送数据
    if ((iErr = luaL_loadfile (lua, "/Users/wanghuan/dev/SocketClient/SocketClient/luaTest.lua")) == 0){
        //执行刚刚读进来的lua文件。LUA_MULTRET指的是可能会有多个返回值。
        if ((iErr = lua_pcall (lua, 0, LUA_MULTRET, 0)) == 0){
            MessageManager::getInstance()->run();
        }
    }
    
    lua_close (lua);
    
    return 0;
    
    
}

