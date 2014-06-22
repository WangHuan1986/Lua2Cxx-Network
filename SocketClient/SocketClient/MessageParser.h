//
//  MessageParser.h
//  SocketClient
//
//  Created by 王 欢 on 14-5-26.
//  Copyright (c) 2014年 王 欢. All rights reserved.
//

#ifndef __SocketClient__MessageParser__
#define __SocketClient__MessageParser__

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include "tinyxml.h"
#include <stack>
class MessageManager;

enum DataType {
    STRING = 0,BOOLEAN,CHAR,SHORT,INT,LONG,DOUBLE,UNKNOWN
};

enum DataTypeLen{
    BOOLEAN_LEN = 1,CHAR_LEN = 1,SHORT_LEN = 2,INT_LEN = 4,
    LONG_LEN = 8,DOUBEL_LEN = 8
};

const int PARSER_BUFFER_SIZE = 1024;
//项目中不会直接这样使用一个文件路径，而是使用引擎自带的文件查找工具函数根据文件名称进行搜索而得到文件全路径
const std::string MESSAGE_PROTOCOL_PATH = "/Users/wanghuan/dev/SocketClient/SocketClient/messageProtocol/";

/**
 *  protocol tree的结点
 *  每一个结点对应消息协议xml中的一个field结点，根节点对应xml中的message结点
**/
struct Node{
    const char *data;
    bool isRoot;
    bool isObject;
    bool isList;
    int listSize;//当结点为list的时候，记录list中的数据数量
    DataType dataType;
    Node *leftLink;
    Node *rightLink;
    Node *parent;
    Node():leftLink(NULL),rightLink(NULL),parent(NULL),isRoot(false),isList(false),listSize(0),isObject(false){}
    Node(const char *d):leftLink(NULL),rightLink(NULL),parent(NULL),isRoot(false),isList(false),isObject(false),listSize(0),data(d){}
};

/**
 *  data tree的结点
 *  每一个结点对应所发送或接收数据中的一个数据字段。当发送数据的时候LuaTable上的数据会被转换为data tree，序列化成为二进制数据发送给服务器
 *  当接收数据的时候会将接收到的二进制数据转换为data tree，最终将data tree转换为Lua Table并将其返回给lua脚本的回调
**/
struct DataNode{
    int travelId;
    const char* nodeName;
    const void *data;
    bool isRoot;
    bool isList;
    bool isObject;
    int listSize;//当结点为list的时候，记录list中的长度
    DataType dataType;
    DataNode *leftLink;
    DataNode *rightLink;
    DataNode *parent;
    DataNode():leftLink(NULL),rightLink(NULL),isRoot(false),isList(false),travelId(0),listSize(0),isObject(false),nodeName(NULL){}
    DataNode(const char *d):leftLink(NULL),rightLink(NULL),isRoot(false),isList(false),travelId(0),isObject(false),listSize(0),data(d),nodeName(NULL){}
    ~DataNode(){
        switch (dataType) {
                
            case DataType::STRING:
            {
                free(const_cast<void *>(this->data));
            }
                break;
            case DataType::BOOLEAN:
            {
                delete static_cast<const int8_t*>(data);
            }
                break;
            case DataType::CHAR:
            {
                delete static_cast<const int8_t*>(data);
            }
                break;
            case DataType::SHORT:
            {
                delete static_cast<const int16_t*>(data);
            }
                break;
            case DataType::INT:
            {
                delete static_cast<const int32_t*>(data);
            }
                break;
            case DataType::LONG :
            {
                delete static_cast<const int64_t*>(data);
            }
                break;
            case DataType::DOUBLE:
            {
                delete static_cast<const int64_t*>(data);
            }
                break;
            case DataType::UNKNOWN:
            {}
                break;
        }
    }
};

//这个POD类记录当前正在被转换的lua table
struct TableStackObject {
    int travelId;
    int parentTravelId;
    bool isList;
    int listSize;
    int listIndex;
    TableStackObject():travelId(0),parentTravelId(0),isList(false),listSize(0),listIndex(0){}
};

class MessageParser{
public:
    MessageParser():sendBufferIndex(0),readBufferIndex(0),travelId(0){}
    //将lua table序列化为二进制数据发送出去
    size_t luaToBinary(lua_State *);
    //将二进制数据转换为lua table，在接收完一条消息后进行调用
    void binaryToLua();
public:
    char *getSendBuffer();
private:
    //根据消息协议xml创建protocol tree
    Node *createProtocolTree(TiXmlNode* pParent,Node *parentNode);
    
    //给定一个protocol tree的根节点，将整个protocol tree转换为data tree
    DataNode *createDataTreeFromLua(lua_State *lua,Node *node);
    
    DataType parseDataType(const char* dataType);
    
    //复制list节点及其孩子节点
    Node *copySubTree(Node *origNode,Node *parentNode);
    
    //通过给定的node得到从Lua table上获取对应的值的访问路径。所谓访问路径是指从lua table上读取对应值所需要的key，这些key会存放到stack返回
    //例如：luatable = {["wife"] = {"[name]" = "yj"}}，则返回的stack为: 栈顶 -> |"wife"|"name"|
    std::stack<std::string> getKeyArray(Node *node);
    
    //给定一个protocol tree上的node，根据node从lua table上读取对应的信息
    const void *readTableData(lua_State *lua,Node *node);
    
    //根据getKeyArray返回的stack从lua table上读取数据
    const void * getValueFromLua(lua_State *lua,std::stack<std::string> &keyArray,Node *node);
    
    //将一个给定的data tree node序列化后存入sendbuffer
    size_t serialDataNode(DataNode *node);
    
    //将整个datatree进行序列化
    size_t serialDataTree(DataNode *node);
    
    //根据接收到的二进制信息创建data tree
    DataNode *createDataTreeFromBinary(Node *node,DataNode *parentNode);
    
    const void * getValueFromBinary(Node *node,int nodeTravelId,DataNode *parentNode);
    
    //给定一个消息协议结点从接收到的二进制信息中读取数据
    const void *readBinaryData(Node *node);
    
    int generateTravelId();
    void resetTravelId();
    
    //将data tree转换为lua table
    void createLuaTable(DataNode *);
    
    //根据当前datanode将二进制数据转换为Lua table以及table上的数据
    void setTableVal(lua_State *lua,DataNode *node);
    void pushTableVal(lua_State *lua,DataNode *node);
    
    void pushTableStack(DataNode *);
    //判断当前被转换的结点的父节点是否为tableStack当前最后一个被压栈的
    bool isParentOnStackTop(DataNode *);
    void setNestedTable(lua_State *lua,DataNode *node);
    
    const char *getMessageTypeFromLua(lua_State *lua);
    const char *getMessageTypeFromBinary();
    const char *getMessageById(short);
    
    //发送或接收完一条数据后释放用到的资源
    void destoryProtocolTree(Node *node);
    void destoryDataTree(DataNode *node);
    
    //下面方法用于测试
    void travelProtocolTree(Node *node);
    void travelDataTree(DataNode *node);
    void printData(DataNode *node);
    void printStack(std::stack<std::string> &stack);
private:
    char sendbuffer[PARSER_BUFFER_SIZE];
    int sendBufferIndex;
    //将二进制数据从MessageReader的buffer上读取出来时使用此index来标记当前读取的位置
    //这里没有再为MessageParser创建一个recvBuffer是因为当binaryToLua被调用的时候刚好是一整条数据接收完毕的时候，所以直接从
    //MessageReader的buffer读取即可
    int readBufferIndex;
    //travelId为data tree的每个结点的遍历顺序，开始发送或开始接收的时候将travelId置为0
    //根节点的travelId是1
    int travelId;
    //接收数据时将datatree转换为lua table的时候用于记录当前正在被转化的table，以实现table之间的嵌套关系
    std::stack<TableStackObject> tableStack;
};

#endif /* defined(__SocketClient__MessageParser__) */
