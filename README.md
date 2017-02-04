# ProtocolGenerator
A utility class, which is use for convert between Google protobuf message and Lua table for cocos2dx

假设protobuf消息结构定义在一个叫test.proto的文件里，那么使用这个文件初始化ProtocolGenerator:

```proto
// test.proto

message ST_ITEM_BUY
{
	required uint32 item_id = 1; // 物品ID
	required uint32 buy_num = 2; // 购买数量
}
```

```C++
ProtocolGenerator * pProtocolGenerator = ProtocolGenerator::Create("test.proto");
```

初始化之后，就可以使用ProtocolGenerator将Lua的table和protobuf的Message进行相互的转换。

#发送数据（将Lua table转换为protobuf的Message）

我们已经在test.proto中定义了一个message ST_ITEM_BUY，那么要发送这个message，只需要在Lua中定义一个table，并通过绑定的函数将message的名字和这个table传进去:

```Lua
local datas = {}

datas.item_id = 1000
datas.buy_num = 3

NetworkManager:SendMessage("ST_ITEM_BUY", datas)
```

之后，在C++层进行处理

```C++
// lua binding

static int tolua_NetworkManager_SendMessage(lua_State * p_pLuaState)
{
	if (nullptr == p_pLuaState)
	{
		return 0;
	}

#if COCOS2D_DEBUG >= 1
	tolua_Error cToLuaError;

	if (!tolua_isusertype(p_pLuaState, 1, "NetworkManager", 0, &cToLuaError))
	{
		goto tolua_lerror;
	}
#endif

	NetworkManager * pNetworkManager = static_cast<NetworkManager *>(tolua_tousertype(p_pLuaState, 1, 0));

	_x_int nArguments = lua_gettop(p_pLuaState) - 1;

	if (2 == nArguments)
	{
#if COCOS2D_DEBUG >= 1
		if (!tolua_isstring(p_pLuaState, 2, 0, &cToLuaError))
		{
			goto tolua_lerror;
		}

		if (!tolua_istable(p_pLuaState, 3, 0, &cToLuaError))
		{
			goto tolua_lerror;
		}
#endif
		const char * pszMessageName = tolua_tostring(p_pLuaState, 2, "");

		bool bSuccess = pNetworkProcessor->SendMessage(pszMessageName, p_pLuaState, 3);

		tolua_pushboolean(p_pLuaState, bSuccess);

		return 1;
	}

	luaL_error(p_pLuaState, "%s has wrong number of arguments : %d, was expecting %d\n", "NetworkManager:SendMessage", nArguments, 2);

	return 0;

#if COCOS2D_DEBUG >= 1
tolua_lerror:
	tolua_error(p_pLuaState, "#ferror in function 'tolua_NetworkManager_SendMessage'.", &cToLuaError);

	return 0;
#endif
}
```

```C++
// NetworkManager.cpp

bool NetworkManager::SendMessage(const char * p_pszMessageName, lua_State * p_pLuaState, _x_int p_nIndex)
{
    ProtocolGenerator * pProtocolGenerator = ProtocolGenerator::Create("test.proto"); // 简单起见，这里直接初始化，使用后直接删除

    // 第一个参数表示proto文件里定义的message的名字，这里p_pszMessageName的值为从Lua传过来的"ST_ITEM_BUY"
    // 第二个参数表示Lua虚拟机的对象指针
    // 第三个参数表示Lua中入栈的消息结构table所在的位置
    google::protobuf::Message * pMessage = pProtocolGenerator->GenerateMessage(p_pszMessageName, p_pLuaState, p_nIndex);

  // 若解析成功，那么就会返回一个google::protobuf::Message的对象指针，若返回nullptr，那么说明解析失败了
  
    if (nullptr == pMessage)
	{
		return CCLOGERROR("Protocol Message \"%s\" Parse Fail!", p_pszMessageName), false;
	}

	bool bSuccess = this->SendMessage(pMessage); // 将这个Message发送出去	
  
    pMessage->Clear();

	SAFE_DELETE(pMessage);
	SAFE_DELETE(pProtocolGenerator);

	return bSuccess;
}

bool NetworkManager::SendMessage(xMessage * p_pMessage)
{
	// serialize and send message...
}
```

#接收数据（将protobuf的Message转换为Lua table）

```C++
void NetworkManager::_ProcessData(const uint32_t p_uMessageType, const unshgned char * p_pszDataBuffer, const uint32_t p_uDataSize)
{
	if (nullptr == p_pszPacketBuffer || p_uDataSize == 0)
	{
	    return;
	}
	
	LuaStack * pLuaStack = LuaEngine::getInstance()->getLuaStack();

	if (nullptr == pLuaStack)
	{
	    return;
	}
	
	ProtocolGenerator * pProtocolGenerator = ProtocolGenerator::Create("test.proto"); // 简单起见，这里直接初始化，使用后直接删除
	
	const char * pszMessageName = this->_GetMessageName(p_uMessageType);

	pLuaStack->pushInt(p_uMessageType);

	if (pProtocolGenerator->ParseMessage(pszMessageName, p_pszDataBuffer, p_uDataSize, pLuaStack->getLuaState()))
	{
		pLuaStack->executeFunctionByHandler(this->m_mapHandlers[p_uMessageType], 2);
	}
	
 	pLuaStack->clean();
 	
 	SAFE_DELETE(pProtocolGenerator);
}
```
