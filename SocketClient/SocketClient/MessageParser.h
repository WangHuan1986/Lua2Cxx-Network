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
    size_t luaToBinary(lua_State *);
    //将二进制数据转换为lua table，在接收完一条消息后进行调用
    void binaryToLua();
public:
    char *getSendBuffer();
private:
    Node *createProtocolTree(TiXmlNode* pParent,Node *parentNode);
    DataType parseDataType(const char* dataType);
    //复制list节点及其孩子节点
    Node *copySubTree(Node *origNode,Node *parentNode);
    //通过给定的node得到从Lua table上获取对应的值的访问路径
    //例如访问存有wife的name的node，则返回["wife","name"]
    std::stack<std::string> getKeyArray(Node *node);
    const void *readTableData(lua_State *lua,Node *node);
    const void * getValueFromLua(lua_State *lua,std::stack<std::string> &keyArray,Node *node);
    DataNode *createDataTreeFromLua(lua_State *lua,Node *node);
    size_t serialDataNode(DataNode *node);
    size_t serialDataTree(DataNode *node);
    const void *readBinaryData(Node *node,int nodeTravelId);
    const void * getValueFromBinary(Node *node,int nodeTravelId,DataNode *parentNode);
    DataNode *createDataTreeFromBinary(Node *node,DataNode *parentNode);
    int generateTravelId();
    void resetTravelId();
    void setTableVal(lua_State *lua,DataNode *node);
    void createLuaTable(DataNode *);
    void pushTableStack(DataNode *);
    void pushTableVal(lua_State *lua,DataNode *node);
    bool isParentOnStackTop(DataNode *);
    void setNestedTable(lua_State *lua,DataNode *node);
    const char *getMessageTypeFromLua(lua_State *lua);
    const char *getMessageTypeFromBinary();
    const char *getMessageById(short);
    void destoryProtocolTree(Node *node);
    void destoryDataTree(DataNode *node);
    
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
    //接收数据时将datatree转换为lua table的时候用于存储当前正在被转化的table，以实现table之间的嵌套关系
    std::stack<TableStackObject> tableStack;
};

#endif /* defined(__SocketClient__MessageParser__) */
