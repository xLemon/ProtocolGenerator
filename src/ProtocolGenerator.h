#ifndef __PROTOCOL_GENERATOR_H__
#define __PROTOCOL_GENERATOR_H__

#ifdef __cplusplus
#	define NS_PROTOCOL_GENERATOR_BEGIN     namespace protocol_generator {
#	define NS_PROTOCOL_GENERATOR_END       }
#	define NS_PROTOCOL_GENERATOR           protocol_generator
#	define USING_NS_PROTOCOL_GENERATOR     using namespace NS_PROTOCOL_GENERATOR
#else
#	define NS_PROTOCOL_GENERATOR_BEGIN
#	define NS_PROTOCOL_GENERATOR_END
#	define NS_PROTOCOL_GENERATOR
#	define USING_NS_PROTOCOL_GENERATOR
#endif

#define CC_IS_VALID_ANSI_STR(x) (nullptr != (x) && strlen((x)) > 0)

#include "CCLuaValue.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/compiler/importer.h>

#include <vector>
#include <string>

#include <stdint.h>

typedef float  float32_t;
typedef double float64_t;

NS_PROTOCOL_GENERATOR_BEGIN

class ProtocolGenerator
{
public:
	enum class PROTOCOL_DATA_TYPE
	{
		PROTOCOL_DATA_VALUE,
		PROTOCOL_DATA_MULTI,
	};

public:
	typedef struct _ProtocolData
	{
	public:
		_ProtocolData();

	public:
		void Clean();

	public:
		ProtocolGenerator::PROTOCOL_DATA_TYPE eDataType;

	public:
		std::string strField;
		std::string strValue;

	public:
		std::vector<ProtocolGenerator::_ProtocolData> vecValues;
	} ProtocolData;

public:
	static ProtocolGenerator * Create(const std::string & p_strProtocolFileName);

public:
	ProtocolGenerator();

public:
	~ProtocolGenerator();

public:
	bool Initialize(const std::string & p_strProtocolFileName);

public:
	bool ParseMessage(const char * p_pszMessageName, const unsigned char * p_pszDataBuffer, const int32_t p_nDataSize, lua_State * p_pLuaState);
	bool ParseMessage(google::protobuf::Message * p_pMessage, lua_State * p_pLuaState);

public:
	google::protobuf::Message * GenerateMessage(const char * p_pszMessageName, lua_State * p_pLuaState, int32_t p_nIndex);
	google::protobuf::Message * GenerateMessage(const char * p_pszMessageName, const std::vector<ProtocolGenerator::ProtocolData> & p_vecValues);
	google::protobuf::Message * GenerateMessage(const char * p_pszMessageName, const unsigned char * p_pszDataBuffer, const int32_t p_nDataSize);

private:
	bool _FillMessageDatas(google::protobuf::Message * p_pMessage, const google::protobuf::Descriptor * p_pDescriptor, const std::vector<ProtocolGenerator::ProtocolData> & p_vecValues);
	bool _FillMessageFileValue(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData);

private:
	bool _FillInt32Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData, bool p_bRepeated);
	bool _FillInt64Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData, bool p_bRepeated);
	bool _FillUInt32Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData, bool p_bRepeated);
	bool _FillUInt64Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData, bool p_bRepeated);
	bool _FillFloat32Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData, bool p_bRepeated);
	bool _FillFloat64Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData, bool p_bRepeated);
	bool _FillBoolValue(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData, bool p_bRepeated);
	bool _FillStringValue(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData, bool p_bRepeated);
	bool _FillEnumValue(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData, bool p_bRepeated);
	bool _FillMessageValue(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData, bool p_bRepeated);

private:
	bool _FillRepeatedInt32Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData);
	bool _FillRepeatedInt64Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData);
	bool _FillRepeatedUInt32Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData);
	bool _FillRepeatedUInt64Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData);
	bool _FillRepeatedFloat32Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData);
	bool _FillRepeatedFloat64Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData);
	bool _FillRepeatedBoolValue(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData);
	bool _FillRepeatedStringValue(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData);
	bool _FillRepeatedEnumValue(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData);
	bool _FillRepeatedMessageValue(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData);

private:
	bool _AnalysisTableData(std::vector<ProtocolGenerator::ProtocolData> & p_vecTableValues, lua_State * p_pLuaState, int32_t p_nIndex);

private:
	bool _ParseFieldData(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState);

private:
	bool _ParseInt32Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState);
	bool _ParseInt64Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState);
	bool _ParseUInt32Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState);
	bool _ParseUInt64Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState);
	bool _ParseFloat32Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState);
	bool _ParseFloat64Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState);
	bool _ParseBoolValue(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState);
	bool _ParseStringValue(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState);
	bool _ParseEnumValue(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState);
	bool _ParseMessageValue(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState);

private:
	bool _ParseRepeatedInt32Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState);
	bool _ParseRepeatedInt64Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState);
	bool _ParseRepeatedUInt32Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState);
	bool _ParseRepeatedUInt64Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState);
	bool _ParseRepeatedFloat32Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState);
	bool _ParseRepeatedFloat64Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState);
	bool _ParseRepeatedBoolValue(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState);
	bool _ParseRepeatedStringValue(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState);
	bool _ParseRepeatedEnumValue(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState);
	bool _ParseRepeatedMessageValue(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState);

private:
	google::protobuf::compiler::Importer * m_pImporter;

private:
	google::protobuf::DynamicMessageFactory m_cMessageFactory;
};

NS_PROTOCOL_GENERATOR_END

#endif // !defined(__PROTOCOL_GENERATOR_H__)
