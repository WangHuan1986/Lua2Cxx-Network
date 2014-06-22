//
//  MessageParser.cpp
//  SocketClient
//
//  Created by 王 欢 on 14-5-26.
//  Copyright (c) 2014年 王 欢. All rights reserved.
//

#include "MessageManager.h"
#include "MessageParser.h"

size_t MessageParser::luaToBinary(lua_State *lua){
    //将buffer中的index重置
    size_t msgLen = 0;
    resetTravelId();
    sendBufferIndex = 0;
    //获取消息类型
    const char *msgType = getMessageTypeFromLua(lua);
    //在定义消息协议文件的时候，第一个field一定得是“<field type = "short" name = "messageType"/>”
    //因为在通过二进制转换为lua table的时候首先要从二进制字节流中获取到messageType，然后才能进一步加载消息协议文件对二进制信息进行解析
    TiXmlDocument doc((MESSAGE_PROTOCOL_PATH + std::string(msgType) + ".xml").c_str());
    lua_settop(lua, 1);
    if(doc.LoadFile()){
        Node *root = createProtocolTree(&doc,NULL);
        DataNode *dataTreeRoot = createDataTreeFromLua(lua,root);
        //将datatree上的数据拷贝到parser的缓冲区上
        msgLen = serialDataTree(dataTreeRoot);
        //释放资源
        destoryProtocolTree(root);
        destoryDataTree(dataTreeRoot);
        lua_settop(lua, 0);
    }
    
    return msgLen;
}

void MessageParser::binaryToLua(){
    resetTravelId();
    //获取消息类型
    const char *msgType = getMessageTypeFromBinary();
    TiXmlDocument doc((MESSAGE_PROTOCOL_PATH + std::string(msgType) + ".xml").c_str());
    if(doc.LoadFile()){
        Node *root = createProtocolTree(&doc,NULL);
        DataNode *dataTreeRoot = createDataTreeFromBinary(root,NULL);
        lua_State *lua = MessageManager::getInstance()->getLuaState();
        //清空lua栈
        lua_settop(lua, 0);
        //清空table栈
        while (tableStack.size() > 0) {
            tableStack.pop();
        }
        //根据接收到的二进制消息创建lua table
        createLuaTable(dataTreeRoot);
        destoryProtocolTree(root);
        destoryDataTree(dataTreeRoot);
        //Lua Table创建完毕，回调lua函数
        lua_getglobal(lua, "luaCallBack");
        lua_insert(lua, 1);
        lua_pcall(lua, 1, 0, 0);
    }
}

Node * MessageParser::createProtocolTree(TiXmlNode* pParent,Node *parentNode){
    if ( !pParent ) return NULL;
    
    Node *node = NULL;
	TiXmlNode* pChild;
    int t = pParent->Type();
    /**
     *  使用tinyxml读取xml文件，例如
     *  xml:
     *    <?xml version="1.0" encoding="UTF-8"?>
     *    <message>
     *      <field type = "short" name = "messageType"/>
     *    </message>
     *
     *  返回的TiXmlDocument对象树:
     *    DOCUMENT
     *       /
     *   DECLARATION - message
     *                   /
     *               field
     *
     *  由于tinyxml读取xml文件后其根节点是DOCUMENT，而当前业务中真正想要的根节点是message，因此需要将DOCUMENT、DECLARATION等除了ELEMENT结点类型
     *  以外的结点都过滤掉，以message为根节点创建protocol tree，protocol tree代表了消息协议在内存中的存储结构
     **/
    if (t != TiXmlNode::TINYXML_ELEMENT) {
        for (pChild = pParent->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) {
            node = createProtocolTree(pChild,NULL);
        }
    }
    //这里是递归创建protocol tree的出口，判断如果是真则返回整个树的根节点
    if (node && node->isRoot) {
        return node;
    }
    //如果不是ELEMENT类型的结点将被过滤
    if (t != TiXmlNode::TINYXML_ELEMENT){
        return NULL;
    }
    //将根据消息协议中的结点信息存储到对应的结点中
    if (t == TiXmlNode::TINYXML_ELEMENT) {
        node = new Node();
        if (strcmp(pParent->Value(),"message") == 0) {
            node->isRoot = true;
        }
        
        TiXmlAttribute* pAttrib = pParent->ToElement()->FirstAttribute();
        while (pAttrib)
        {
            if (strcmp(pAttrib->Name(),"name") == 0) {
                node->data = pAttrib->Value();
            }
            if (strcmp(pAttrib->Name(),"type") == 0) {
                node->dataType = parseDataType(pAttrib->Value());
            }
            if (strcmp(pAttrib->Name(),"list") == 0) {
                if (strcmp(pAttrib->Value(),"true") == 0) {
                    node->isList = true;
                }
            }
            if (strcmp(pAttrib->Name(),"object") == 0) {
                if (strcmp(pAttrib->Value(),"true") == 0) {
                    node->isObject = true;
                }
            }
            pAttrib = pAttrib->Next();
        }
    }
    
    pChild = pParent->FirstChild();
    node->leftLink = createProtocolTree(pChild,node);
    node->rightLink = createProtocolTree(pParent->NextSibling(),parentNode);
    node->parent = parentNode;
    
    if (node->isRoot) {
        node->data = "root";
        node->parent = NULL;
    }
    
    return node;
}

DataType MessageParser::parseDataType(const char* dataType){
    
    if (strcmp(dataType,"string") == 0) {
        return DataType::STRING;
    }
    else if (strcmp(dataType,"boolean") == 0){
        return DataType::BOOLEAN;
    }
    else if (strcmp(dataType,"char") == 0){
        return DataType::CHAR;
    }
    else if (strcmp(dataType,"short") == 0){
        return DataType::SHORT;
    }
    else if (strcmp(dataType,"int") == 0){
        return DataType::INT;
    }
    else if (strcmp(dataType,"long") == 0){
        return DataType::LONG;
    }
    else if (strcmp(dataType,"double") == 0){
        return DataType::DOUBLE;
    }
    else{
        return DataType::UNKNOWN;
    }
}

DataNode *MessageParser::createDataTreeFromLua(lua_State *lua,Node *node){
    if (node == NULL) {
        return NULL;
    }
    DataNode *dataNode = new DataNode();
    dataNode->travelId = generateTravelId();
    //通过给定的node得到从Lua table上获取对应的值的访问路径
    std::stack<std::string> keyArray = getKeyArray(node);
    //如果node是一个list节点，则返回list的第一个孩子节点的指针
    //如果node是一个普通节点，则返回具体值的指针
    const void *val = getValueFromLua(lua,keyArray,node);
    if (node->isRoot) {
        dataNode->data = "root";
        dataNode->isRoot = true;
    }
    else if(node->isList){
        dataNode->data = NULL;
        dataNode->listSize = node->listSize;
        dataNode->isList = true;
    }
    else{
        dataNode->data = val;
    }
    
    dataNode->dataType = node->dataType;
    //如果是一个list则不需要再进行递归了，因为getValueFromLua已经递归创建了list，最后返回的是list的中的第一个结点
    if (node->isList) {
        dataNode->leftLink = (DataNode*)(val);
    }
    else{
        dataNode->leftLink = createDataTreeFromLua(lua, node->leftLink);
    }
    dataNode->rightLink = createDataTreeFromLua(lua, node->rightLink);
    
    return dataNode;
}

int MessageParser::generateTravelId(){
    return ++travelId;
}

std::stack<std::string> MessageParser::getKeyArray(Node *node){
    std::stack<std::string> ret;
    if (!node) {
        return ret;
    }
    //将当前节点本身的数据先压栈，根节点除外
    if (node->parent) {
        ret.push(node->data);
    }
    
    Node *parentNode = node->parent;
    while (parentNode) {
        //压入除根节点外的访问路径上的节点数据
        if (parentNode->parent) {
            ret.push(parentNode->data);
        }
        parentNode = parentNode->parent;
    }
    
    return ret;
}

const void * MessageParser::getValueFromLua(lua_State *lua,std::stack<std::string> &keyArray,Node *node){
    //记录当前lua栈的高度
    int iniIndex = lua_gettop(lua);
    int currIndex = iniIndex;
    const void *ret = NULL;
    //根据路径stack读取当前node所对应的lua table上的数据
    while (!keyArray.empty()){
        lua_pushstring(lua, keyArray.top().c_str());
        lua_gettable(lua, currIndex++);
        keyArray.pop();
    }
    //如果读取到了数据
    if (lua_gettop(lua) > currIndex - 1) {
        //如果当前要解析的node对应的是一个list类型，那么读取出来的将是一个table，例如{[1] = "x",[2] = "x"}
        if (node->isList) {
            //获取table数组的长度，即list的长度
            int listLen = (int)lua_objlen(lua,-1);
            node->listSize = listLen;
            DataNode *listFirstChild = NULL;
            DataNode *listLastChild = listFirstChild;
            for (int i = 1; i <= listLen; i++) {
                lua_rawgeti(lua, -i, i);
                //递归创建list中的各个元素，list中的元素以单链表的形式进行存储，最后返回链表的第一个结点
                DataNode *newChild = createDataTreeFromLua(lua,copySubTree(node,NULL));
                if (listFirstChild == NULL) {
                    listFirstChild = newChild;
                    listLastChild = newChild;
                }
                else{
                    listLastChild->rightLink = newChild;
                    listLastChild = newChild;
                }
            }
            
            ret = listFirstChild;
        }
        //如果是普通结点，则之间读取对应的数据
        else{
            ret = readTableData(lua,node);
        }
        //在Lua stack上读取某个数据后需要清除掉由于读取数据而产生的中间结果，以备下次进行其它读取操作
        lua_settop(lua, iniIndex);
    }
    else{
        ret = NULL;
    }
    return ret;
    
}

/**
 *  将protocol tree转向data tree的时候，如果结点是一个list则需要进一步处理。
 *  Lua Table:
 *    local data = {
 *       ["kids"] = {
 *            {["name"] = "k1",["age"] = 1},
 *            {["name"] = "k2",["age"] = 2}
 *        }
 *    }
 *
 *  protocol xml:
 *    <?xml version="1.0" encoding="UTF-8"?>
 *    <message>
 *      <field type = "java.obj" name = "kids" list = "true">
 *          <field type = "string" name = "name"/>
 *          <field type = "char" name = "age"/>
 *      </field>
 *    </message>
 *
 *  protocol tree:                         data tree:
 *      message                                 root
 *      /               ------->                /
 *    kids              ------->              kids
 *    /                                       /
 *  name - age                              kid1 - kid2
 *                                          /       /
 *                                        k1 - 1  k2 - 2
 *
 *  当遍历protocol tree的时候，当前结点是一个List结点(例如kids)，想要生成data tree中的kids子树就需要复制protocol tree的kids子树：
 *
 *     kids
 *     /
 *  name - age
 *
 *  复制后得到的子树的根节点对应于data tree中,kids1或kids2中间结点，中间结点并不携带真正要发送的数据，实际上在转换为data tree后中间结点
 *  的data为null
 *  copySubTree用于复制子树，利用生成的子树以及当前在lua stack上的list table递归调用createDataTreeFromLua以形成data tree中的list子树
**/
Node *MessageParser::copySubTree(Node *origNode,Node *parentNode){
    if (origNode == NULL) {
        return NULL;
    }
    
    Node *node = new Node();
    node->leftLink = copySubTree(origNode->leftLink,node);
    node->rightLink = copySubTree(origNode->rightLink,parentNode);
    node->parent = parentNode;
    node->data = origNode->data;
    node->dataType = origNode->dataType;
    if (parentNode) {
        node->isList = origNode->isList;
        node->isObject = origNode->isObject;
    }
    
    return node;
}

const void *MessageParser::readTableData(lua_State *lua,Node *node){
    const void *ret = NULL;
    switch (node->dataType) {
        case DataType::STRING:
        {
            const char *str = lua_tostring(lua, -1);
            if (str == NULL) {
                return NULL;
            }
            size_t len = strlen(str);
            char *strCopied = (char *)malloc(len + 1);
            memcpy(strCopied, str, len);
            strCopied[len] = '\0';
            ret = strCopied;
        }
            break;
        case DataType::BOOLEAN:
        {
            int8_t b = lua_toboolean(lua, -1);
            ret = new int8_t(b);
        }
            break;
        case DataType::CHAR:
        {
            int8_t i8 = lua_tonumber(lua, -1);
            ret = new int8_t(i8);
        }
            break;
            
        case DataType::SHORT:
        {
            int16_t i16 = lua_tonumber(lua, -1);
            ret = new int16_t(i16);
        }
            break;
        case DataType::INT:
        {
            int32_t i32 = lua_tonumber(lua, -1);
            ret = new int32_t(i32);
        }
            break;
        case DataType::LONG :
        {
            int64_t i64 = lua_tonumber(lua, -1);
            ret = new int64_t(i64);
        }
            break;
        case DataType::DOUBLE:
        {
            double d = lua_tonumber(lua, -1);
            ret = new double(d);
        }
            break;
        case DataType::UNKNOWN:
        {
            ret = NULL;
        }
            break;
    }
    
    return ret;
}

//将存有lua数据的树序列化为二进制数据存放在parser buffer上
size_t MessageParser::serialDataTree(DataNode *node){
    size_t msgLen = 0;
    if (node == NULL) {
        return 0;
    }
    if (!node->isRoot) {
        msgLen += serialDataNode(node);
    }
    
    msgLen += serialDataTree(node->leftLink);
    msgLen += serialDataTree(node->rightLink);
    
    return msgLen;
}

size_t MessageParser::serialDataNode(DataNode *node){
    size_t dataLen = 0;
    if ((node == NULL || node->data == NULL) && !node->isList) {
        return 0;
    }
    switch (node->dataType) {
        case DataType::STRING:
        {
            const char *ret = (const char*)(node->data);
            size_t strLen = strlen(ret);
            //如果当前序列化的节点是string类型，则在string数据的前面加上2个字节的长度数据以表示这个字符串的长度
            //被拷贝到parser buffer上的string不带有结束标示符'\0'
            int16_t strLenIndicator = (int16_t)strLen;
            memcpy(sendbuffer + sendBufferIndex, &strLenIndicator, DataTypeLen::SHORT_LEN);
            sendBufferIndex += DataTypeLen::SHORT_LEN;
            memcpy(sendbuffer + sendBufferIndex, ret, strLen);
            sendBufferIndex += strLen;
            dataLen = strLen + DataTypeLen::SHORT_LEN;
        }
            break;
        case DataType::BOOLEAN:
        {
            const int8_t *b = (const int8_t *)(node->data);
            memcpy(sendbuffer + sendBufferIndex, b, DataTypeLen::BOOLEAN_LEN);
            dataLen = DataTypeLen::BOOLEAN_LEN;
            sendBufferIndex += dataLen;
        }
            break;
        case DataType::CHAR:
        {
            const int8_t *i8 = (const int8_t *)(node->data);
            memcpy(sendbuffer + sendBufferIndex, i8, DataTypeLen::CHAR_LEN);
            dataLen = DataTypeLen::CHAR_LEN;
            sendBufferIndex += dataLen;
        }
            break;
        case DataType::SHORT:
        {
            const int16_t *i16 = (const int16_t *)(node->data);
            memcpy(sendbuffer + sendBufferIndex, i16, DataTypeLen::SHORT_LEN);
            dataLen = DataTypeLen::SHORT_LEN;
            sendBufferIndex += dataLen;
        }
            break;
        case DataType::INT:
        {
            const int32_t *i32 = (const int32_t *)(node->data);
            memcpy(sendbuffer + sendBufferIndex, i32, DataTypeLen::INT_LEN);
            dataLen = DataTypeLen::INT_LEN;
            sendBufferIndex += dataLen;
        }
            break;
        case DataType::LONG :
        {
            const int64_t *i64 = (const int64_t *)(node->data);
            memcpy(sendbuffer + sendBufferIndex, i64, DataTypeLen::LONG_LEN);
            dataLen = DataTypeLen::LONG_LEN;
            sendBufferIndex += dataLen;
        }
            break;
        case DataType::DOUBLE:
        {
            const double *d = (const double *)(node->data);
            memcpy(sendbuffer + sendBufferIndex, d, DataTypeLen::DOUBEL_LEN);
            dataLen = DataTypeLen::DOUBEL_LEN;
            sendBufferIndex += dataLen;
        }
            break;
        case DataType::UNKNOWN:
        {
            if (node->isList) {
                //如果当前序列化的节点是list类型，则在list数据的前面加上2个字节的长度数据以表示list中所含元素的个数
                int16_t listLenIndicator = (int16_t)node->listSize;
                memcpy(sendbuffer + sendBufferIndex, &listLenIndicator, DataTypeLen::SHORT_LEN);
                sendBufferIndex += DataTypeLen::SHORT_LEN;
                dataLen = DataTypeLen::SHORT_LEN;
            }
        }
            break;
    }
    
    return dataLen;
}

DataNode *MessageParser::createDataTreeFromBinary(Node *node,DataNode *parentNode){
    if (node == NULL) {
        return NULL;
    }
    DataNode *dataNode = new DataNode();
    int nodeTravelId = generateTravelId();
    dataNode->travelId = nodeTravelId;
    //将二进制信息转换为data tree结点上的数据
    const void *val = getValueFromBinary(node,nodeTravelId,dataNode);
    if (node->isRoot) {
        dataNode->data = "root";
        dataNode->isRoot = true;
        dataNode->parent = NULL;
    }
    else if(node->isList){
        dataNode->data = NULL;
        dataNode->isList = true;
    }
    else{
        dataNode->data = val;
    }
    
    if (node->isObject) {
        dataNode->isObject = true;
    }
    
    dataNode->dataType = node->dataType;
    dataNode->nodeName = node->data;
    if (node->isList) {
        dataNode->leftLink = (DataNode*)(val);
    }
    else{
        dataNode->leftLink = createDataTreeFromBinary(node->leftLink,dataNode);
    }
    dataNode->rightLink = createDataTreeFromBinary(node->rightLink,parentNode);
    dataNode->parent = parentNode;
    
    return dataNode;
}

const void * MessageParser::getValueFromBinary(Node *node,int nodeTravelId,DataNode *parentNode){
    const void *ret = NULL;
    if (node->isList) {
        //如果当前正在解析的数据是一个list，则二进制信息中的前两个自己代表了List中所含元素的数量
        int16_t listLen = 0;
        char *readBuffer = MessageManager::getInstance()->getReader().getBuffer();
        memcpy(&listLen,readBuffer + readBufferIndex,DataTypeLen::SHORT_LEN);
        readBufferIndex += DataTypeLen::SHORT_LEN;
        parentNode->listSize = listLen;
        
        DataNode *listFirstChild = NULL;
        DataNode *listLastChild = listFirstChild;
        for (int i = 1; i <= listLen; i++) {
            DataNode *newChild = createDataTreeFromBinary(copySubTree(node,NULL),parentNode);
            newChild->isObject = true;
            if (listFirstChild == NULL) {
                listFirstChild = newChild;
                listLastChild = newChild;
            }
            else{
                listLastChild->rightLink = newChild;
                listLastChild = newChild;
            }
        }
        
        ret = listFirstChild;
    }
    else{
        ret = readBinaryData(node);
    }
    
    return ret;
}

const void *MessageParser::readBinaryData(Node *node){
    const void *ret = NULL;
    MessageReader *reader = &MessageManager::getInstance()->getReader();
    char *readBuffer = reader->getBuffer();
    switch (node->dataType) {
            
        case DataType::STRING:
        {
            //string数据前面加上2个字节代表string类型的数据长度
            int16_t len = 0;
            memcpy(&len,readBuffer + readBufferIndex,DataTypeLen::SHORT_LEN);
            readBufferIndex += DataTypeLen::SHORT_LEN;
            
            char *strCopied = (char *)malloc(len + 1);
            memcpy(strCopied, readBuffer + readBufferIndex, len);
            strCopied[len] = '\0';
            ret = strCopied;
            readBufferIndex += len;
        }
            break;
        case DataType::BOOLEAN:
        {
            int8_t *b = new int8_t();
            memcpy(b,readBuffer + readBufferIndex,DataTypeLen::BOOLEAN_LEN);
            ret = b;
            readBufferIndex += DataTypeLen::BOOLEAN_LEN;
        }
            break;
        case DataType::CHAR:
        {
            int8_t *c = new int8_t();
            memcpy(c,readBuffer + readBufferIndex,DataTypeLen::CHAR_LEN);
            ret = c;
            readBufferIndex += DataTypeLen::CHAR_LEN;
        }
            break;
            
        case DataType::SHORT:
        {
            int16_t *i16 = new int16_t();
            memcpy(i16,readBuffer + readBufferIndex,DataTypeLen::SHORT_LEN);
            ret = i16;
            readBufferIndex += DataTypeLen::SHORT_LEN;
        }
            break;
        case DataType::INT:
        {
            int32_t *i32 = new int32_t();
            memcpy(i32,readBuffer + readBufferIndex,DataTypeLen::INT_LEN);
            ret = i32;
            readBufferIndex += DataTypeLen::INT_LEN;
        }
            break;
        case DataType::LONG :
        {
            int64_t *i64 = new int64_t();
            memcpy(i64,readBuffer + readBufferIndex,DataTypeLen::LONG_LEN);
            ret = i64;
            readBufferIndex += DataTypeLen::LONG_LEN;
        }
            break;
        case DataType::DOUBLE:
        {
            double *d = new double();
            memcpy(d,readBuffer + readBufferIndex,DataTypeLen::DOUBEL_LEN);
            ret = d;
            readBufferIndex += DataTypeLen::DOUBEL_LEN;
        }
            break;
        case DataType::UNKNOWN:
        {
            ret = NULL;
        }
            break;
    }
    
    return ret;
}

void MessageParser::createLuaTable(DataNode *node){
    
    if (node == NULL) {
        return;
    }
    
    lua_State *lua = MessageManager::getInstance()->getLuaState();
    setTableVal(lua, node);
    
    createLuaTable(node->leftLink);
    createLuaTable(node->rightLink);
    //如果遍历已经完毕，但lua栈上依然还有没有被嵌套设置的table则还要继续进行设置
    if (node->isRoot) {
        while (lua_gettop(lua) > 1) {
            setNestedTable(lua,node);
        }
    }
}

void MessageParser::setTableVal(lua_State *lua,DataNode *node){
    if (node->isRoot) {
        lua_newtable(lua);
        pushTableStack(node);
    }
    else if (node->isObject || node->isList) {
        if (!isParentOnStackTop(node)) {
            setNestedTable(lua,node);
        }
        //如果当前节点是集合元素，则不会push table key，而是直接调用lua_rawseti进行设置
        if (!node->parent->isList) {
            lua_pushstring(lua, node->nodeName);
        }
        //创建新的lua table到lua stack上
        lua_newtable(lua);
        pushTableStack(node);
    }
    //如果是叶子节点
    else{
        if (!isParentOnStackTop(node)) {
            setNestedTable(lua,node);
        }
        //将叶子节点携带的数据设置为table的值
        lua_pushstring(lua, node->nodeName);
        pushTableVal(lua, node);
        lua_settable(lua, -3);
    }
}

void MessageParser::pushTableStack(DataNode *node){
    TableStackObject stackObject;
    stackObject.travelId = node->travelId;
    if (node->isRoot) {
        stackObject.parentTravelId = 0;
    }
    else{
        stackObject.parentTravelId = node->parent->travelId;
    }
    stackObject.isList = node->isList;
    stackObject.listSize = node->listSize;
    //当前已经被添加到table的list的元素的个数
    stackObject.listIndex = 0;
    tableStack.push(stackObject);
}

void MessageParser::pushTableVal(lua_State *lua,DataNode *node){
    if (node->data == NULL) {
        lua_pushnil(lua);
        return;
    }
    switch (node->dataType) {
        case DataType::STRING:
        {
            const char *str = (const char *)node->data;
            lua_pushstring(lua,str);
        }
            break;
        case DataType::BOOLEAN:
        {
            int8_t *b = (int8_t *)node->data;
            lua_pushboolean(lua, *b);
        }
            break;
        case DataType::CHAR:
        {
            int8_t *i8 = (int8_t *)node->data;
            lua_pushnumber(lua, *i8);
        }
            break;
            
        case DataType::SHORT:
        {
            int16_t *i16 = (int16_t *)node->data;
            lua_pushnumber(lua, *i16);
        }
            break;
        case DataType::INT:
        {
            int32_t *i32 = (int32_t *)node->data;
            lua_pushnumber(lua, *i32);
        }
            break;
        case DataType::LONG :
        {
            int64_t *i64 = (int64_t *)node->data;
            lua_pushnumber(lua,*i64);
        }
            break;
        case DataType::DOUBLE:
        {
            double *d = (double *)node->data;
            lua_pushnumber(lua, *d);
        }
            break;
        case DataType::UNKNOWN:
        {
            lua_pushnil(lua);
        }
            break;
    }
    
}

/**
 *  将二级制数据转换为lua table，首先需要将二进制数据转换为data tree，然后再将data tree转换为lua table，也就是要将整个树上的信息按照其层次关系合并
 *  成为一个Lua table，最后再返回给Lua脚本
 *  data tree:
 *  
 *         root
 *         /
 *       dogs(list)
 *       /               
 *     dog1   -   dog2
 *     /           /
 *   name - age  name -age
 *
 *  按照先序遍历，遍历到dog2的时候:
 *
 *  tableStack此时为:
 *  |root|dogs|dog1| <-- top
 *  
 *  lua stack此时为:
 *  |root|dogs|dog1| <-- top 当遍历到dog2的时候由isParentOnStackTop判断得出，dog2的父节点不是当前table stack栈顶的元素
 *                           即当前已经开始遍历父节点的另外一个子节点了，这个时候就需要将lua stack上的dog1合并到dogs才能继续进行
 *
**/
bool MessageParser::isParentOnStackTop(DataNode *node){
    if (!tableStack.empty() && tableStack.top().travelId == node->parent->travelId){
        return true;
    }
    else{
        return false;
    }
}

/**
 *  当当前遍历到的节点的父节点不是tablestack当前的栈顶元素时，就需要settable或是rawset进行table合并了，因为这个时候标识着
 *  已经遍历到了栈顶元素所代表的节点的的兄弟节点了
 *  将tableStack栈顶的table设置为其父table的子元素。父table可能是一个数组，也可能是一个对象需要进行判断而区别对待
**/
void MessageParser::setNestedTable(lua_State *lua,DataNode *node){
    //弹出要被设为嵌套table对应的信息对象,弹出后当前的栈顶对象则为其父table的信息对象
    tableStack.pop();
    //获取到父table的信息
    TableStackObject &parentObj = tableStack.top();
    if (parentObj.isList) {
        lua_rawseti(lua, -2, ++parentObj.listIndex);
    }
    //如果是普通对象
    else{
        lua_settable(lua, -3);
    }
    //在合并table的时候需要一直向上合并到当前遍历到的节点的根节点为止，例如：
    /*
     *          root
     *           /
     *          A
     *         /
     *        B - F
     *       /   /
     *      C   G
     *     /
     *    D
     *   /
     *  E
     *
     *  当遍历完E，开始遍历F的时候，就需要按E->D->C->B->A的顺序依次进行table合并，然后再继续进行
     *
    */
    if (!node->isRoot && parentObj.travelId != node->parent->travelId) {
        setNestedTable(lua, node);
    }
}

void MessageParser::resetTravelId(){
    travelId = 0;
}


char * MessageParser::getSendBuffer(){
    return sendbuffer;
}

const char *MessageParser::getMessageTypeFromLua(lua_State *lua){
    //获取messageTypeId
    lua_pushstring(lua,"messageType");
    lua_gettable(lua, 1);
    short msgTypeId = lua_tonumber(lua, -1);
    //根据messageTypeId获取对应的消息协议路径
    const char *ret = getMessageById(msgTypeId);
    return ret;
}

const char *MessageParser::getMessageTypeFromBinary(){
    char *readBuffer = MessageManager::getInstance()->getReader().getBuffer();
    int16_t msgTypeId;
    memcpy(&msgTypeId,readBuffer + readBufferIndex,DataTypeLen::SHORT_LEN);
    const char *ret = getMessageById(msgTypeId);
    return ret;
}

const char *MessageParser::getMessageById(short msgTypeId){
    lua_State *lua = MessageManager::getInstance()->getLuaState();
    lua_getglobal(lua, "g");
    lua_pushstring(lua, "MESSAGE_TYPE");
    lua_gettable(lua,-2);
    lua_rawgeti(lua, -1, msgTypeId);
    const char *msgType = lua_tostring(lua, -1);
    
    
    return msgType;
}

void MessageParser::destoryProtocolTree(Node *node){
    if (node == NULL) {
        return;
    }
    delete node;
    destoryProtocolTree(node->leftLink);
    destoryProtocolTree(node->rightLink);
}

void MessageParser::destoryDataTree(DataNode *node){
    if (node == NULL) {
        return;
    }
    delete node;
    destoryDataTree(node->leftLink);
    destoryDataTree(node->rightLink);
}

void MessageParser::travelProtocolTree(Node *node){
    if (node == NULL) {
        return;
    }
    if (node->isRoot) {
        printf("root\n");
    }
    else{
        printf("node %s,isList=%s\n",node->data,node->isList ? "true" : "false");
    }
    
    travelProtocolTree(node->leftLink);
    travelProtocolTree(node->rightLink);
}

void MessageParser::printData(DataNode *node){
    if (node == NULL || node->data == NULL) {
        if (node->nodeName) {
            printf("nodeName_%s | ",node->nodeName);
        }
        if (node->isList) {
            printf("listSize_%d | ",node->listSize);
        }
        if (node->isObject) {
            printf("isObject_%d | ",node->isObject);
        }
        printf("nodeData_*\n");
        return;
    }
    switch (node->dataType) {
        case DataType::STRING:
        {
            const char *ret = (const char*)(node->data);
            if (node->nodeName) {
                printf("nodeName_%s | ",node->nodeName);
            }
            else{
                printf("root");
            }
            printf("nodeData_%s\n",ret);
            
        }
            break;
        case DataType::BOOLEAN:
        {
            const int8_t *b = (const int8_t *)(node->data);
            printf("%d\n",*b);
        }
            break;
        case DataType::CHAR:
        {
            const int8_t *i8 = (const int8_t *)(node->data);
            if (node->nodeName) {
                printf("nodeName_%s | ",node->nodeName);
            }
            
            printf("nodeData_%d\n",*i8);
        }
            break;
            
        case DataType::SHORT:
        {
            const int16_t *i16 = (const int16_t *)(node->data);
            printf("%d\n",*i16);
        }
            break;
        case DataType::INT:
        {
            const int32_t *i32 = (const int32_t *)(node->data);
            std::cout << *i32 << std::endl;
        }
            break;
        case DataType::LONG :
        {
            const int64_t *i64 = (const int64_t *)(node->data);
            std::cout << *i64 << std::endl;
        }
            break;
        case DataType::DOUBLE:
        {
            const double *d = (const double *)(node->data);
            printf("%f\n",*d);
        }
            break;
        case DataType::UNKNOWN:
        {
            printf("-unknow-\n");
            return;
        }
            break;
            
    }
}

void MessageParser::travelDataTree(DataNode *node){
    if (node == NULL) {
        return;
    }
    printf("travelId:%d : ",node->travelId);
    if (node->isRoot) {
        printf(" dataNode root\n");
    }
    else{
        printData(node);
    }
    
    travelDataTree(node->leftLink);
    travelDataTree(node->rightLink);
}

void MessageParser::printStack(std::stack<std::string> &stack){
    while (!stack.empty()) {
        printf("%s ",stack.top().c_str());
        stack.pop();
    }
    printf("\n");
}


