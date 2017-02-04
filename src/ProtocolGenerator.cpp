#include "ProtocolGenerator_New.h"

#include "CCFileUtils.h"
#include "CCLuaEngine.h"

#include <algorithm>

USING_NS_CC;

NS_PROTOCOL_GENERATOR_BEGIN

struct FindProtocolDataByField : public std::binary_function <ProtocolGenerator::ProtocolData, std::string, bool>
{
	bool operator () (const ProtocolGenerator::ProtocolData & p_cProtocolData, const std::string & p_strField) const
	{
		return p_cProtocolData.strField == p_strField;
	}
};

ProtocolGenerator::_ProtocolData::_ProtocolData()
{
	Clean();
}

void ProtocolGenerator::_ProtocolData::Clean()
{
	eDataType = ProtocolGenerator::PROTOCOL_DATA_TYPE::PROTOCOL_DATA_VALUE;

	strField.clear();
	strValue.clear();

	vecValues.clear();
}

ProtocolGenerator * ProtocolGenerator::Create(const std::string & p_strProtocolFileName)
{
	ProtocolGenerator * pGenerator = new (std::nothrow) ProtocolGenerator();

	if (nullptr == pGenerator || !pGenerator->Initialize(p_strProtocolFileName))
	{
		CC_SAFE_DELETE(pGenerator);
	}

	return pGenerator;
}

ProtocolGenerator::ProtocolGenerator()
{
	this->m_pImporter = nullptr;
}

ProtocolGenerator::~ProtocolGenerator()
{
	CC_SAFE_DELETE(this->m_pImporter);
}

bool ProtocolGenerator::Initialize(const std::string & p_strProtocolFileName)
{
	bool bSuccess = false;

	do 
	{
		CC_BREAK_IF(p_strProtocolFileName.empty());

		std::string strFullPath = CCFileUtils::getInstance()->fullPathForFilename(p_strProtocolFileName);

		if (!CCFileUtils::getInstance()->isFileExist(strFullPath))
		{
			CCLOGERROR("Protocol File \"%s\" Not Exist!", strFullPath.c_str()); break;
		}

		google::protobuf::compiler::DiskSourceTree cSourceTree;

		cSourceTree.MapPath("", strFullPath.c_str());

		this->m_pImporter = new (std::nothrow) google::protobuf::compiler::Importer(&cSourceTree, nullptr);

		CC_BREAK_IF(nullptr == this->m_pImporter);
		CC_BREAK_IF(nullptr == this->m_pImporter->Import(p_strProtocolFileName));

		bSuccess = true;
	}
	while (false);
	
	return bSuccess;
}

bool ProtocolGenerator::ParseMessage(const char * p_pszMessageName, const unsigned char * p_pszDataBuffer, const int32_t p_nDataSize, lua_State * p_pLuaState)
{
	if (nullptr == p_pLuaState)
	{
		return false;
	}

	google::protobuf::Message * pMessage = this->GenerateMessage(p_pszMessageName, p_pszDataBuffer, p_nDataSize);

	if (nullptr == pMessage)
	{
		return false;
	}

	lua_newtable(p_pLuaState);

	bool bSucces = true;

	if (!this->ParseMessage(pMessage, p_pLuaState))
	{
		bSucces = false;
	}

	pMessage->Clear();

	CC_SAFE_DELETE(pMessage);

	return bSucces;
}

bool ProtocolGenerator::ParseMessage(google::protobuf::Message * p_pMessage, lua_State * p_pLuaState)
{
	bool bSuccess = false;

	do 
	{
		CC_BREAK_IF(nullptr == p_pMessage);
		CC_BREAK_IF(nullptr == p_pLuaState);

		const google::protobuf::Descriptor * pDescriptor = p_pMessage->GetDescriptor();

		CC_BREAK_IF(nullptr == pDescriptor);

		const google::protobuf::Reflection * pReflection = p_pMessage->GetReflection();

		CC_BREAK_IF(nullptr == pReflection);

		bSuccess = true;

		int32_t nFieldCount = pDescriptor->field_count();

		CC_BREAK_IF(nFieldCount <= 0);

		const google::protobuf::FieldDescriptor * pField = nullptr;

		for (int32_t i = 0; i < nFieldCount; ++i)
		{
			bSuccess = false;

			pField = pDescriptor->field(i);

			CC_BREAK_IF(nullptr == pField);
			CC_BREAK_IF(!this->_ParseFieldData(p_pMessage, pField, p_pLuaState));

			bSuccess = true;
		}
	}
	while (false);

	return bSuccess;
}

google::protobuf::Message * ProtocolGenerator::GenerateMessage(const char * p_pszMessageName, lua_State * p_pLuaState, int32_t p_nIndex)
{
	google::protobuf::Message * pMessage = nullptr;

	do
	{
		CC_BREAK_IF(nullptr == p_pLuaState || p_nIndex < 0);
		CC_BREAK_IF(!CC_IS_VALID_ANSI_STR(p_pszMessageName));

		std::vector<ProtocolGenerator::ProtocolData> vecTableValues;

		CC_BREAK_IF(!this->_AnalysisTableData(vecTableValues, p_pLuaState, p_nIndex));

		pMessage = this->GenerateMessage(p_pszMessageName, vecTableValues);
	}
	while (false);

	return pMessage;
}

google::protobuf::Message * ProtocolGenerator::GenerateMessage(const char * p_pszMessageName, const std::vector<ProtocolGenerator::ProtocolData> & p_vecValues)
{
	google::protobuf::Message * pMessage = nullptr;

	do
	{
		CC_BREAK_IF(!CC_IS_VALID_ANSI_STR(p_pszMessageName));
		CC_BREAK_IF(nullptr == this->m_pImporter);

		const google::protobuf::Descriptor * pDescriptor = this->m_pImporter->pool()->FindMessageTypeByName(p_pszMessageName);

		CC_BREAK_IF(nullptr == pDescriptor);

		const google::protobuf::Message * pPrototype = this->m_cMessageFactory.GetPrototype(pDescriptor);

		CC_BREAK_IF(nullptr == pPrototype);

		pMessage = pPrototype->New();

		CC_BREAK_IF(nullptr == pMessage);
		CC_BREAK_IF(this->_FillMessageDatas(pMessage, pDescriptor, p_vecValues));

		pMessage->Clear();

		CC_SAFE_DELETE(pMessage);
	}
	while (false);

	return pMessage;
}

google::protobuf::Message * ProtocolGenerator::GenerateMessage(const char * p_pszMessageName, const unsigned char * p_pszDataBuffer, const int32_t p_nDataSize)
{
	google::protobuf::Message * pMessage = nullptr;

	do
	{
		CC_BREAK_IF(!CC_IS_VALID_ANSI_STR(p_pszMessageName));
		CC_BREAK_IF(nullptr == p_pszDataBuffer || p_nDataSize <= 0);
		CC_BREAK_IF(nullptr == this->m_pImporter);

		const google::protobuf::Descriptor * pDescriptor = this->m_pImporter->pool()->FindMessageTypeByName(p_pszMessageName);

		CC_BREAK_IF(nullptr == pDescriptor);

		const google::protobuf::Message * pPrototype = this->m_cMessageFactory.GetPrototype(pDescriptor);

		CC_BREAK_IF(nullptr == pPrototype);

		pMessage = pPrototype->New();

		CC_BREAK_IF(nullptr == pMessage);
		CC_BREAK_IF(pMessage->ParseFromArray(p_pszDataBuffer, p_nDataSize));

		pMessage->Clear();

		CC_SAFE_DELETE(pMessage);
	}
	while (false);

	return pMessage;
}

bool ProtocolGenerator::_FillMessageDatas(google::protobuf::Message * p_pMessage, const google::protobuf::Descriptor * p_pDescriptor, const std::vector<ProtocolGenerator::ProtocolData> & p_vecValues)
{
	if (nullptr == p_pMessage || nullptr == p_pDescriptor)
	{
		return false;
	}

	const google::protobuf::Reflection * pReflection = p_pMessage->GetReflection();

	if (nullptr == pReflection)
	{
		return CCLOGERROR("Message Type \"%s\"'s Reflection Is NULL!", p_pMessage->GetTypeName().c_str()), false;
	}

	int32_t nFieldCount = p_pDescriptor->field_count();

	if (nFieldCount <= 0)
	{
		return true;
	}

	const google::protobuf::FieldDescriptor * pField = nullptr;

	const ProtocolGenerator::ProtocolData * pProtocolData = nullptr;

	bool bSuccess = true;

	for (int32_t i = 0; i < nFieldCount; ++i)
	{
		bSuccess = false;

		pProtocolData = nullptr;

		pField = p_pDescriptor->field(i);

		CC_BREAK_IF(nullptr == pField);

		auto pIterFind = std::find_if(p_vecValues.begin(), p_vecValues.end(), std::bind2nd(FindProtocolDataByField(), pField->name()));

		if (pIterFind != p_vecValues.end())
		{
			pProtocolData = &(*pIterFind);
		}

		CC_BREAK_IF(!this->_FillMessageFileValue(p_pMessage, pField, pReflection, pProtocolData));

		bSuccess = true;
	}

	return bSuccess;
}

bool ProtocolGenerator::_FillMessageFileValue(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData)
{
	if (nullptr == p_pMessage || nullptr == p_pField || nullptr == p_pReflection)
	{
		return false;
	}

	if (p_pField->is_required())
	{
		if (nullptr == p_pProtocolData)
		{
			return CCLOGERROR("Field \"%s\"'s Value Is Required! Message Type : \"%s\".", p_pField->name().c_str(), p_pMessage->GetTypeName().c_str()), false;
		}
	}

	google::protobuf::FieldDescriptor::CppType eType = p_pField->cpp_type();

	if (eType == google::protobuf::FieldDescriptor::CPPTYPE_INT32)
	{
		if (p_pField->is_repeated())
		{
			return this->_FillRepeatedInt32Value(p_pMessage, p_pField, p_pReflection, p_pProtocolData);
		}
		return this->_FillInt32Value(p_pMessage, p_pField, p_pReflection, p_pProtocolData, false);
	}
	else if (eType == google::protobuf::FieldDescriptor::CPPTYPE_INT64)
	{
		if (p_pField->is_repeated())
		{
			return this->_FillRepeatedInt64Value(p_pMessage, p_pField, p_pReflection, p_pProtocolData);
		}
		return this->_FillInt64Value(p_pMessage, p_pField, p_pReflection, p_pProtocolData, false);
	}
	else if (eType == google::protobuf::FieldDescriptor::CPPTYPE_UINT32)
	{
		if (p_pField->is_repeated())
		{
			return this->_FillRepeatedUInt32Value(p_pMessage, p_pField, p_pReflection, p_pProtocolData);
		}
		return this->_FillUInt32Value(p_pMessage, p_pField, p_pReflection, p_pProtocolData, false);
	}
	else if (eType == google::protobuf::FieldDescriptor::CPPTYPE_UINT64)
	{
		if (p_pField->is_repeated())
		{
			return this->_FillRepeatedUInt64Value(p_pMessage, p_pField, p_pReflection, p_pProtocolData);
		}
		return this->_FillUInt64Value(p_pMessage, p_pField, p_pReflection, p_pProtocolData, false);
	}
	else if (eType == google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE)
	{
		if (p_pField->is_repeated())
		{
			return this->_FillRepeatedFloat64Value(p_pMessage, p_pField, p_pReflection, p_pProtocolData);
		}
		return this->_FillFloat64Value(p_pMessage, p_pField, p_pReflection, p_pProtocolData, false);
	}
	else if (eType == google::protobuf::FieldDescriptor::CPPTYPE_FLOAT)
	{
		if (p_pField->is_repeated())
		{
			return this->_FillRepeatedFloat32Value(p_pMessage, p_pField, p_pReflection, p_pProtocolData);
		}
		return this->_FillFloat32Value(p_pMessage, p_pField, p_pReflection, p_pProtocolData, false);
	}
	else if (eType == google::protobuf::FieldDescriptor::CPPTYPE_BOOL)
	{
		if (p_pField->is_repeated())
		{
			return this->_FillRepeatedBoolValue(p_pMessage, p_pField, p_pReflection, p_pProtocolData);
		}
		return this->_FillBoolValue(p_pMessage, p_pField, p_pReflection, p_pProtocolData, false);
	}
	else if (eType == google::protobuf::FieldDescriptor::CPPTYPE_ENUM)
	{
		if (p_pField->is_repeated())
		{
			return this->_FillRepeatedEnumValue(p_pMessage, p_pField, p_pReflection, p_pProtocolData);
		}
		return this->_FillEnumValue(p_pMessage, p_pField, p_pReflection, p_pProtocolData, false);
	}
	else if (eType == google::protobuf::FieldDescriptor::CPPTYPE_STRING)
	{
		if (p_pField->is_repeated())
		{
			return this->_FillRepeatedStringValue(p_pMessage, p_pField, p_pReflection, p_pProtocolData);
		}
		return this->_FillStringValue(p_pMessage, p_pField, p_pReflection, p_pProtocolData, false);
	}
	else if (eType == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
	{
		if (p_pField->is_repeated())
		{
			return this->_FillRepeatedMessageValue(p_pMessage, p_pField, p_pReflection, p_pProtocolData);
		}
		return this->_FillMessageValue(p_pMessage, p_pField, p_pReflection, p_pProtocolData, false);
	}
	else
	{
		return CCLOGERROR("Field \"%s\"'s Type(%d) Is Unsupported!", p_pField->name().c_str(), static_cast<int32_t>(eType)), false;
	}
}

bool ProtocolGenerator::_FillInt32Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData, bool p_bRepeated)
{
	if (nullptr != p_pProtocolData && p_pProtocolData->eDataType == ProtocolGenerator::PROTOCOL_DATA_TYPE::PROTOCOL_DATA_VALUE && !p_pProtocolData->strValue.empty())
	{
		int32_t nValue = 0;

		sscanf(p_pProtocolData->strValue.c_str(), "%d", &nValue);

		if (p_bRepeated)
		{
			p_pReflection->AddInt32(p_pMessage, p_pField, nValue);
		}
		else
		{
			p_pReflection->SetInt32(p_pMessage, p_pField, nValue);
		}

		return p_pReflection->SetInt32(p_pMessage, p_pField, nValue), true;
	}

	int32_t nDefaultValue = p_pField->default_value_int32();

	if (p_bRepeated)
	{
		p_pReflection->AddInt32(p_pMessage, p_pField, nDefaultValue);
	}
	else
	{
		p_pReflection->SetInt32(p_pMessage, p_pField, nDefaultValue);
	}

	if (p_pField->is_required())
	{
		CCLOGWARN("Field \"%s\"'s Value Is Required, Used Default%s Value \"%d\". Message Type : \"%s\".", p_pField->name().c_str(), p_pField->has_default_value() ? " Specified" : "", nDefaultValue, p_pMessage->GetTypeName().c_str());
	}
#if defined(_DEBUG)
	else
	{
		CCLOGINFO("Field \"%s\"'s Value Is Optional Or Repeated, Used Default%s Value \"%d\". Message Type : \"%s\".", p_pField->name().c_str(), p_pField->has_default_value() ? " Specified" : "", nDefaultValue, p_pMessage->GetTypeName().c_str());
	}
#endif

	return true;
}

bool ProtocolGenerator::_FillInt64Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData, bool p_bRepeated)
{
	if (nullptr != p_pProtocolData && p_pProtocolData->eDataType == ProtocolGenerator::PROTOCOL_DATA_TYPE::PROTOCOL_DATA_VALUE && !p_pProtocolData->strValue.empty())
	{
		int64_t nValue = 0;

		sscanf(p_pProtocolData->strValue.c_str(), "%lld", &nValue);

		if (p_bRepeated)
		{
			p_pReflection->AddInt64(p_pMessage, p_pField, nValue);
		}
		else
		{
			p_pReflection->SetInt64(p_pMessage, p_pField, nValue);
		}

		return true;
	}

	int64_t nDefaultValue = p_pField->default_value_int64();

	if (p_bRepeated)
	{
		p_pReflection->AddInt64(p_pMessage, p_pField, nDefaultValue);
	}
	else
	{
		p_pReflection->SetInt64(p_pMessage, p_pField, nDefaultValue);
	}

	if (p_pField->is_required())
	{
		CCLOGWARN("Field \"%s\"'s Value Is Required, Used Default%s Value \"%lld\". Message Type : \"%s\".", p_pField->name().c_str(), p_pField->has_default_value() ? " Specified" : "", nDefaultValue, p_pMessage->GetTypeName().c_str());
	}
#if defined(_DEBUG)
	else
	{
		CCLOGINFO("Field \"%s\"'s Value Is Optional Or Repeated, Used Default%s Value \"%lld\". Message Type : \"%s\".", p_pField->name().c_str(), p_pField->has_default_value() ? " Specified" : "", nDefaultValue, p_pMessage->GetTypeName().c_str());
	}
#endif

	return true;
}

bool ProtocolGenerator::_FillUInt32Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData, bool p_bRepeated)
{
	if (nullptr != p_pProtocolData && p_pProtocolData->eDataType == ProtocolGenerator::PROTOCOL_DATA_TYPE::PROTOCOL_DATA_VALUE && !p_pProtocolData->strValue.empty())
	{
		uint32_t uValue = 0;

		sscanf(p_pProtocolData->strValue.c_str(), "%u", &uValue);

		if (p_bRepeated)
		{
			p_pReflection->AddUInt32(p_pMessage, p_pField, uValue);
		}
		else
		{
			p_pReflection->SetUInt32(p_pMessage, p_pField, uValue);
		}

		return true;
	}

	uint32_t uDefaultValue = p_pField->default_value_uint32();

	if (p_bRepeated)
	{
		p_pReflection->AddUInt32(p_pMessage, p_pField, uDefaultValue);
	}
	else
	{
		p_pReflection->SetUInt32(p_pMessage, p_pField, uDefaultValue);
	}

	if (p_pField->is_required())
	{
		CCLOGWARN("Field \"%s\"'s Value Is Required, Used Default%s Value \"%u\". Message Type : \"%s\".", p_pField->name().c_str(), p_pField->has_default_value() ? " Specified" : "", uDefaultValue, p_pMessage->GetTypeName().c_str());
	}
#if defined(_DEBUG)
	else
	{
		CCLOGINFO("Field \"%s\"'s Value Is Optional Or Repeated, Used Default%s Value \"%u\". Message Type : \"%s\".", p_pField->name().c_str(), p_pField->has_default_value() ? " Specified" : "", uDefaultValue, p_pMessage->GetTypeName().c_str());
	}
#endif

	return true;
}

bool ProtocolGenerator::_FillUInt64Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData, bool p_bRepeated)
{
	if (nullptr != p_pProtocolData && p_pProtocolData->eDataType == ProtocolGenerator::PROTOCOL_DATA_TYPE::PROTOCOL_DATA_VALUE && !p_pProtocolData->strValue.empty())
	{
		uint64_t uValue = 0;

		sscanf(p_pProtocolData->strValue.c_str(), "%llu", &uValue);

		if (p_bRepeated)
		{
			p_pReflection->AddUInt64(p_pMessage, p_pField, uValue);
		}
		else
		{
			p_pReflection->SetUInt64(p_pMessage, p_pField, uValue);
		}

		return true;
	}

	uint64_t uDefaultValue = p_pField->default_value_uint64();

	if (p_bRepeated)
	{
		p_pReflection->AddUInt64(p_pMessage, p_pField, uDefaultValue);
	}
	else
	{
		p_pReflection->SetUInt64(p_pMessage, p_pField, uDefaultValue);
	}

	if (p_pField->is_required())
	{
		CCLOGWARN("Field \"%s\"'s Value Is Required, Used Default%s Value \"%llu\". Message Type : \"%s\".", p_pField->name().c_str(), p_pField->has_default_value() ? " Specified" : "", uDefaultValue, p_pMessage->GetTypeName().c_str());
	}
#if defined(_DEBUG)
	else
	{
		CCLOGINFO("Field \"%s\"'s Value Is Optional Or Repeated, Used Default%s Value \"%llu\". Message Type : \"%s\".", p_pField->name().c_str(), p_pField->has_default_value() ? " Specified" : "", uDefaultValue, p_pMessage->GetTypeName().c_str());
	}
#endif

	return true;
}

bool ProtocolGenerator::_FillFloat32Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData, bool p_bRepeated)
{
	if (nullptr != p_pProtocolData && p_pProtocolData->eDataType == ProtocolGenerator::PROTOCOL_DATA_TYPE::PROTOCOL_DATA_VALUE && !p_pProtocolData->strValue.empty())
	{
		float32_t fValue = 0.f;

		sscanf(p_pProtocolData->strValue.c_str(), "%f", &fValue);

		if (p_bRepeated)
		{
			p_pReflection->AddFloat(p_pMessage, p_pField, fValue);
		}
		else
		{
			p_pReflection->SetFloat(p_pMessage, p_pField, fValue);
		}

		return true;
	}

	float32_t fDefaultValue = p_pField->default_value_float();

	if (p_bRepeated)
	{
		p_pReflection->AddFloat(p_pMessage, p_pField, fDefaultValue);
	}
	else
	{
		p_pReflection->SetFloat(p_pMessage, p_pField, fDefaultValue);
	}

	if (p_pField->is_required())
	{
		CCLOGWARN("Field \"%s\"'s Value Is Required, Used Default%s Value \"%f\". Message Type : \"%s\".", p_pField->name().c_str(), p_pField->has_default_value() ? " Specified" : "", fDefaultValue, p_pMessage->GetTypeName().c_str());
	}
#if defined(_DEBUG)
	else
	{
		CCLOGINFO("Field \"%s\"'s Value Is Optional Or Repeated, Used Default%s Value \"%f\". Message Type : \"%s\".", p_pField->name().c_str(), p_pField->has_default_value() ? " Specified" : "", fDefaultValue, p_pMessage->GetTypeName().c_str());
	}
#endif

	return true;
}

bool ProtocolGenerator::_FillFloat64Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData, bool p_bRepeated)
{
	if (nullptr != p_pProtocolData && p_pProtocolData->eDataType == ProtocolGenerator::PROTOCOL_DATA_TYPE::PROTOCOL_DATA_VALUE && !p_pProtocolData->strValue.empty())
	{
		float64_t fValue = 0;

		sscanf(p_pProtocolData->strValue.c_str(), "%lf", &fValue);

		if (p_bRepeated)
		{
			p_pReflection->AddDouble(p_pMessage, p_pField, fValue);
		}
		else
		{
			p_pReflection->SetDouble(p_pMessage, p_pField, fValue);
		}

		return true;
	}

	float64_t fDefaultValue = p_pField->default_value_double();

	if (p_bRepeated)
	{
		p_pReflection->AddDouble(p_pMessage, p_pField, fDefaultValue);
	}
	else
	{
		p_pReflection->SetDouble(p_pMessage, p_pField, fDefaultValue);
	}

	if (p_pField->is_required())
	{
		CCLOGWARN("Field \"%s\"'s Value Is Required, Used Default%s Value \"%lf\". Message Type : \"%s\".", p_pField->name().c_str(), p_pField->has_default_value() ? " Specified" : "", fDefaultValue, p_pMessage->GetTypeName().c_str());
	}
#if defined(_DEBUG)
	else
	{
		CCLOGINFO("Field \"%s\"'s Value Is Optional Or Repeated, Used Default%s Value \"%lf\". Message Type : \"%s\".", p_pField->name().c_str(), p_pField->has_default_value() ? " Specified" : "", fDefaultValue, p_pMessage->GetTypeName().c_str());
	}
#endif

	return true;
}

bool ProtocolGenerator::_FillBoolValue(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData, bool p_bRepeated)
{
	if (nullptr != p_pProtocolData && p_pProtocolData->eDataType == ProtocolGenerator::PROTOCOL_DATA_TYPE::PROTOCOL_DATA_VALUE && !p_pProtocolData->strValue.empty())
	{
		if (0 == p_pProtocolData->strValue.compare("true"))
		{
			if (p_bRepeated)
			{
				p_pReflection->AddBool(p_pMessage, p_pField, true);
			}
			else
			{
				p_pReflection->SetBool(p_pMessage, p_pField, true);
			}
			return true;
		}
		else if (0 == p_pProtocolData->strValue.compare("false"))
		{
			if (p_bRepeated)
			{
				p_pReflection->AddBool(p_pMessage, p_pField, false);
			}
			else
			{
				p_pReflection->SetBool(p_pMessage, p_pField, false);
			}
			return true;
		}
		else
		{
			return CCLOGERROR("Boolean Field \"%s\"'s Value(%s) Is Invalid! It Must Be \"true\" Or \"false\".", p_pField->name().c_str(), p_pProtocolData->strValue.c_str()), false;
		}
	}

	bool bDefaultValue = p_pField->default_value_bool();

	if (p_bRepeated)
	{
		p_pReflection->AddBool(p_pMessage, p_pField, bDefaultValue);
	}
	else
	{
		p_pReflection->SetBool(p_pMessage, p_pField, bDefaultValue);
	}

	if (p_pField->is_required())
	{
		CCLOGWARN("Field \"%s\"'s Value Is Required, Used Default%s Value \"%s\". Message Type : \"%s\".", p_pField->name().c_str(), p_pField->has_default_value() ? " Specified" : "", bDefaultValue ? "true" : "false", p_pMessage->GetTypeName().c_str());
	}
#if defined(_DEBUG)
	else
	{
		CCLOGINFO("Field \"%s\"'s Value Is Optional Or Repeated, Used Default%s Value \"%s\". Message Type : \"%s\".", p_pField->name().c_str(), p_pField->has_default_value() ? " Specified" : "", bDefaultValue ? "true" : "false", p_pMessage->GetTypeName().c_str());
	}
#endif

	return true;
}

bool ProtocolGenerator::_FillStringValue(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData, bool p_bRepeated)
{
	if (nullptr != p_pProtocolData && p_pProtocolData->eDataType == ProtocolGenerator::PROTOCOL_DATA_TYPE::PROTOCOL_DATA_VALUE)
	{
		if (p_bRepeated)
		{
			p_pReflection->AddString(p_pMessage, p_pField, p_pProtocolData->strValue);
		}
		else
		{
			p_pReflection->SetString(p_pMessage, p_pField, p_pProtocolData->strValue);
		}
		return true;
	}

	const std::string & strDefaultValue = p_pField->default_value_string();

	if (p_bRepeated)
	{
		p_pReflection->AddString(p_pMessage, p_pField, strDefaultValue);
	}
	else
	{
		p_pReflection->SetString(p_pMessage, p_pField, strDefaultValue);
	}

	if (p_pField->is_required())
	{
		CCLOGWARN("Field \"%s\"'s Value Is Required, Used Default%s Value \"%s\". Message Type : \"%s\".", p_pField->name().c_str(), p_pField->has_default_value() ? " Specified" : "", strDefaultValue.c_str(), p_pMessage->GetTypeName().c_str());
	}
#if defined(_DEBUG)
	else
	{
		CCLOGINFO("Field \"%s\"'s Value Is Optional Or Repeated, Used Default%s Value \"%s\". Message Type : \"%s\".", p_pField->name().c_str(), p_pField->has_default_value() ? " Specified" : "", strDefaultValue.c_str(), p_pMessage->GetTypeName().c_str());
	}
#endif

	return true;
}

bool ProtocolGenerator::_FillEnumValue(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData, bool p_bRepeated)
{
	if (nullptr != p_pProtocolData && p_pProtocolData->eDataType == ProtocolGenerator::PROTOCOL_DATA_TYPE::PROTOCOL_DATA_VALUE && !p_pProtocolData->strValue.empty())
	{
		const google::protobuf::EnumDescriptor * pEnumDescriptor = p_pField->enum_type();

		if (nullptr == pEnumDescriptor)
		{
			return CCLOGERROR("Field \"%s\"'s Descriptor Is NULL! Message Type : \"%s\".", p_pField->name().c_str(), p_pMessage->GetTypeName().c_str()), false;
		}

		int32_t nValue = 0;

		sscanf(p_pProtocolData->strValue.c_str(), "%d", &nValue);

		const google::protobuf::EnumValueDescriptor * pEnumValueDescriptor = pEnumDescriptor->FindValueByNumber(nValue);

		if (nullptr == pEnumValueDescriptor)
		{
			return CCLOGERROR("Field \"%s\"'s EnumValueDescriptor(%d) Is NULL! Message Type : \"%s\".", p_pField->name().c_str(), nValue, p_pMessage->GetTypeName().c_str()), false;
		}

		if (p_bRepeated)
		{
			p_pReflection->AddEnum(p_pMessage, p_pField, pEnumValueDescriptor);
		}
		else
		{
			p_pReflection->SetEnum(p_pMessage, p_pField, pEnumValueDescriptor);
		}

		return true;
	}

	const google::protobuf::EnumValueDescriptor * pDefaultEnumValueDescriptor = p_pField->default_value_enum();

	if (nullptr == pDefaultEnumValueDescriptor)
	{
		return CCLOGERROR("Field \"%s\"'s Value Is Required! Message Type : \"%s\".", p_pField->name().c_str(), p_pMessage->GetTypeName().c_str()), false;
	}

	if (p_bRepeated)
	{
		p_pReflection->AddEnum(p_pMessage, p_pField, pDefaultEnumValueDescriptor);
	}
	else
	{
		p_pReflection->SetEnum(p_pMessage, p_pField, pDefaultEnumValueDescriptor);
	}

	if (p_pField->is_required())
	{
		CCLOGWARN("Field \"%s\"'s Value Is Required, Used Default%s Value \"%s\" (%d). Message Type : \"%s\".", p_pField->name().c_str(), p_pField->has_default_value() ? " Specified" : "", pDefaultEnumValueDescriptor->full_name().c_str(), pDefaultEnumValueDescriptor->number(), p_pMessage->GetTypeName().c_str());
	}
#if defined(_DEBUG)
	else
	{
		CCLOGINFO("Field \"%s\"'s Value Is Optional Or Repeated, Used Default%s Value \"%s\" (%d). Message Type : \"%s\".", p_pField->name().c_str(), p_pField->has_default_value() ? " Specified" : "", pDefaultEnumValueDescriptor->full_name().c_str(), pDefaultEnumValueDescriptor->number(), p_pMessage->GetTypeName().c_str());
	}
#endif

	return true;
}

bool ProtocolGenerator::_FillMessageValue(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData, bool p_bRepeated)
{
	if (nullptr != p_pProtocolData && p_pProtocolData->eDataType == ProtocolGenerator::PROTOCOL_DATA_TYPE::PROTOCOL_DATA_MULTI && !p_pProtocolData->vecValues.empty())
	{
		const google::protobuf::Descriptor * pDescriptor = p_pField->message_type();

		if (nullptr == pDescriptor)
		{
			return CCLOGERROR("Field \"%s\"'s Descriptor Is NULL! Message Type : \"%s\".", p_pField->name().c_str(), p_pMessage->GetTypeName().c_str()), false;
		}

		bool bSuccess = true;

		google::protobuf::Message * pSubMessage = nullptr;

		if (p_pField->is_repeated())
		{
//			int32_t nFieldCountBefore = p_pReflection->FieldSize(*p_pMessage, p_pField);

			pSubMessage = p_pReflection->AddMessage(p_pMessage, p_pField, &(this->m_cMessageFactory));

			if (nullptr == pSubMessage)
			{
				bSuccess = false;
			}

//			int32_t nFieldCountAfter = p_pReflection->FieldSize(*p_pMessage, p_pField);

			if (bSuccess)
			{
				bSuccess = this->_FillMessageDatas(pSubMessage, pSubMessage->GetDescriptor(), p_pProtocolData->vecValues);
			}
		}
		else
		{
			pSubMessage = this->GenerateMessage(pDescriptor->name().c_str(), p_pProtocolData->vecValues);

			if (nullptr == pSubMessage)
			{
				bSuccess = false;
			}

			if (bSuccess)
			{
				p_pReflection->SetAllocatedMessage(p_pMessage, pSubMessage, p_pField);
			}
		}

		if (!bSuccess)
		{
			CC_SAFE_DELETE(pSubMessage);
		}

		return bSuccess;
	}

	// 嵌套的message不会有默认值，因此这里不处理默认值的情况

	if (p_pField->is_required())
	{
		return CCLOGERROR("Field \"%s\"'s Value Is Required! Message Type : \"%s\".", p_pField->name().c_str(), p_pMessage->GetTypeName().c_str()), false;
	}

	if (p_pField->is_optional())
	{
		return true;
	}

	if (p_pField->is_repeated())
	{
		return true;
	}

	return CCLOGERROR("Field \"%s\" Fill Message Value With Error! Message Type : \"%s\".", p_pField->name().c_str(), p_pMessage->GetTypeName().c_str()), false;
}

bool ProtocolGenerator::_FillRepeatedInt32Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData)
{
	if (nullptr == p_pProtocolData)
	{
		return true;
	}

	if (p_pProtocolData->eDataType != ProtocolGenerator::PROTOCOL_DATA_TYPE::PROTOCOL_DATA_MULTI)
	{
		return false;
	}

	bool bSuccess = false;

	for (auto pIter = p_pProtocolData->vecValues.begin(), pIterEnd = p_pProtocolData->vecValues.end(); pIter != pIterEnd; ++pIter)
	{
		bSuccess = false;

		CC_BREAK_IF(!this->_FillInt32Value(p_pMessage, p_pField, p_pReflection, &(*pIter), true));

		bSuccess = true;
	}

	return bSuccess;
}

bool ProtocolGenerator::_FillRepeatedInt64Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData)
{
	if (nullptr == p_pProtocolData)
	{
		return true;
	}

	if (p_pProtocolData->eDataType != ProtocolGenerator::PROTOCOL_DATA_TYPE::PROTOCOL_DATA_MULTI)
	{
		return false;
	}

	bool bSuccess = false;

	uint32_t nIndex = 0;

	for (auto pIter = p_pProtocolData->vecValues.begin(), pIterEnd = p_pProtocolData->vecValues.end(); pIter != pIterEnd; ++pIter)
	{
		bSuccess = false;

		CC_BREAK_IF(!this->_FillInt64Value(p_pMessage, p_pField, p_pReflection, &(*pIter), true));

		bSuccess = true;
	}

	return bSuccess;
}

bool ProtocolGenerator::_FillRepeatedUInt32Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData)
{
	if (nullptr == p_pProtocolData)
	{
		return true;
	}

	if (p_pProtocolData->eDataType != ProtocolGenerator::PROTOCOL_DATA_TYPE::PROTOCOL_DATA_MULTI)
	{
		return false;
	}

	bool bSuccess = false;

	uint32_t nIndex = 0;

	for (auto pIter = p_pProtocolData->vecValues.begin(), pIterEnd = p_pProtocolData->vecValues.end(); pIter != pIterEnd; ++pIter)
	{
		bSuccess = false;

		CC_BREAK_IF(!this->_FillUInt32Value(p_pMessage, p_pField, p_pReflection, &(*pIter), true));

		bSuccess = true;
	}

	return bSuccess;
}

bool ProtocolGenerator::_FillRepeatedUInt64Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData)
{
	if (nullptr == p_pProtocolData)
	{
		return true;
	}

	if (p_pProtocolData->eDataType != ProtocolGenerator::PROTOCOL_DATA_TYPE::PROTOCOL_DATA_MULTI)
	{
		return false;
	}

	bool bSuccess = false;

	uint32_t nIndex = 0;

	for (auto pIter = p_pProtocolData->vecValues.begin(), pIterEnd = p_pProtocolData->vecValues.end(); pIter != pIterEnd; ++pIter)
	{
		bSuccess = false;

		CC_BREAK_IF(!this->_FillUInt64Value(p_pMessage, p_pField, p_pReflection, &(*pIter), true));

		bSuccess = true;
	}

	return bSuccess;
}

bool ProtocolGenerator::_FillRepeatedFloat32Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData)
{
	if (nullptr == p_pProtocolData)
	{
		return true;
	}

	if (p_pProtocolData->eDataType != ProtocolGenerator::PROTOCOL_DATA_TYPE::PROTOCOL_DATA_MULTI)
	{
		return false;
	}

	bool bSuccess = false;

	uint32_t nIndex = 0;

	for (auto pIter = p_pProtocolData->vecValues.begin(), pIterEnd = p_pProtocolData->vecValues.end(); pIter != pIterEnd; ++pIter)
	{
		bSuccess = false;

		CC_BREAK_IF(!this->_FillFloat32Value(p_pMessage, p_pField, p_pReflection, &(*pIter), true));

		bSuccess = true;
	}

	return bSuccess;
}

bool ProtocolGenerator::_FillRepeatedFloat64Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData)
{
	if (nullptr == p_pProtocolData)
	{
		return true;
	}

	if (p_pProtocolData->eDataType != ProtocolGenerator::PROTOCOL_DATA_TYPE::PROTOCOL_DATA_MULTI)
	{
		return false;
	}

	bool bSuccess = false;

	uint32_t nIndex = 0;

	for (auto pIter = p_pProtocolData->vecValues.begin(), pIterEnd = p_pProtocolData->vecValues.end(); pIter != pIterEnd; ++pIter)
	{
		bSuccess = false;

		CC_BREAK_IF(!this->_FillFloat64Value(p_pMessage, p_pField, p_pReflection, &(*pIter), true));

		bSuccess = true;
	}

	return bSuccess;
}

bool ProtocolGenerator::_FillRepeatedBoolValue(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData)
{
	if (nullptr == p_pProtocolData)
	{
		return true;
	}

	if (p_pProtocolData->eDataType != ProtocolGenerator::PROTOCOL_DATA_TYPE::PROTOCOL_DATA_MULTI)
	{
		return false;
	}

	bool bSuccess = false;

	uint32_t nIndex = 0;

	for (auto pIter = p_pProtocolData->vecValues.begin(), pIterEnd = p_pProtocolData->vecValues.end(); pIter != pIterEnd; ++pIter)
	{
		bSuccess = false;

		CC_BREAK_IF(!this->_FillBoolValue(p_pMessage, p_pField, p_pReflection, &(*pIter), true));

		bSuccess = true;
	}

	return bSuccess;
}

bool ProtocolGenerator::_FillRepeatedStringValue(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData)
{
	if (nullptr == p_pProtocolData)
	{
		return true;
	}

	if (p_pProtocolData->eDataType != ProtocolGenerator::PROTOCOL_DATA_TYPE::PROTOCOL_DATA_MULTI)
	{
		return false;
	}

	bool bSuccess = false;

	uint32_t nIndex = 0;

	for (auto pIter = p_pProtocolData->vecValues.begin(), pIterEnd = p_pProtocolData->vecValues.end(); pIter != pIterEnd; ++pIter)
	{
		bSuccess = false;

		CC_BREAK_IF(!this->_FillStringValue(p_pMessage, p_pField, p_pReflection, &(*pIter), true));

		bSuccess = true;
	}

	return bSuccess;
}

bool ProtocolGenerator::_FillRepeatedEnumValue(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData)
{
	if (nullptr == p_pProtocolData)
	{
		return true;
	}

	if (p_pProtocolData->eDataType != ProtocolGenerator::PROTOCOL_DATA_TYPE::PROTOCOL_DATA_MULTI)
	{
		return false;
	}

	bool bSuccess = false;

	uint32_t nIndex = 0;

	for (auto pIter = p_pProtocolData->vecValues.begin(), pIterEnd = p_pProtocolData->vecValues.end(); pIter != pIterEnd; ++pIter)
	{
		bSuccess = false;

		CC_BREAK_IF(!this->_FillEnumValue(p_pMessage, p_pField, p_pReflection, &(*pIter), true));

		bSuccess = true;
	}

	return bSuccess;
}

bool ProtocolGenerator::_FillRepeatedMessageValue(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, const google::protobuf::Reflection * p_pReflection, const ProtocolGenerator::ProtocolData * p_pProtocolData)
{
	if (nullptr == p_pProtocolData)
	{
		return true;
	}

	if (p_pProtocolData->eDataType != ProtocolGenerator::PROTOCOL_DATA_TYPE::PROTOCOL_DATA_MULTI)
	{
		return false;
	}

	bool bSuccess = false;

	uint32_t nIndex = 0;

	for (auto pIter = p_pProtocolData->vecValues.begin(), pIterEnd = p_pProtocolData->vecValues.end(); pIter != pIterEnd; ++pIter)
	{
		bSuccess = false;

		CC_BREAK_IF(!this->_FillMessageValue(p_pMessage, p_pField, p_pReflection, &(*pIter), true));

		bSuccess = true;
	}

	return bSuccess;
}

bool ProtocolGenerator::_AnalysisTableData(std::vector<ProtocolGenerator::ProtocolData> & p_vecTableValues, lua_State * p_pLuaState, int32_t p_nIndex)
{
	if (!lua_istable(p_pLuaState, p_nIndex))
	{
		return false;
	}

	bool bSuccess = true;

	// Push another reference to the table on top of the stack (so we know  
	// where it is, and this function can work for negative, positive and  
	// pseudo indices

	lua_pushvalue(p_pLuaState, p_nIndex);
	
	// stack now contains: -1 => table

	lua_pushnil(p_pLuaState);

	// stack now contains: -1 => nil; -2 => table

	while (lua_next(p_pLuaState, -2))
	{
		ProtocolGenerator::ProtocolData cProtocolData;

		// stack now contains: -1 => value; -2 => key; -3 => table
		// copy the key so that lua_tostring does not modify the original

		lua_pushvalue(p_pLuaState, -2);

		// stack now contains: -1 => key; -2 => value; -3 => key; -4 => table

		do 
		{
			bSuccess = false;

			const char * pszKey = lua_tostring(p_pLuaState, -1);

			CC_BREAK_IF(!CC_IS_VALID_ANSI_STR(pszKey));

			if (lua_istable(p_pLuaState, -2))
			{
				CC_BREAK_IF(!this->_AnalysisTableData(cProtocolData.vecValues, p_pLuaState, -2));

				cProtocolData.eDataType = ProtocolGenerator::PROTOCOL_DATA_TYPE::PROTOCOL_DATA_MULTI;
			}
			else if (lua_isboolean(p_pLuaState, -2))
			{
				bool bValue = lua_toboolean(p_pLuaState, -2) == 1;

				cProtocolData.eDataType = ProtocolGenerator::PROTOCOL_DATA_TYPE::PROTOCOL_DATA_VALUE;
				cProtocolData.strValue  = bValue ? "true" : "false";
			}
			else
			{
				const char * pszValue = lua_tostring(p_pLuaState, -2);

				CC_BREAK_IF(nullptr == pszValue);

				cProtocolData.eDataType = ProtocolGenerator::PROTOCOL_DATA_TYPE::PROTOCOL_DATA_VALUE;
				cProtocolData.strValue  = pszValue;
			}

			cProtocolData.strField = pszKey;

			p_vecTableValues.push_back(cProtocolData);

			bSuccess = true;
		}
		while (false);
		
		lua_pop(p_pLuaState, 2); // pop value + copy of key, leaving original key

		// stack now contains: -1 => key; -2 => table

		CC_BREAK_IF(!bSuccess);
	}

	// stack now contains: -1 => table (when lua_next returns 0 it pops the key
	// but does not push anything.)
	
	lua_pop(p_pLuaState, 1); // Pop table

	// Stack is now the same as it was on entry to this function

	return bSuccess;
}

bool ProtocolGenerator::_ParseFieldData(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState)
{
	if (nullptr == p_pMessage || nullptr == p_pField)
	{
		return false;
	}

// 	if (p_pField->is_optional() && p_pMessage->GetReflection()->getu)
// 	{
// 		if (nullptr == p_pProtocolData)
// 		{
// 			return false;
// 		}
// 	}

	google::protobuf::FieldDescriptor::CppType eType = p_pField->cpp_type();

	if (eType == google::protobuf::FieldDescriptor::CPPTYPE_INT32)
	{
		if (p_pField->is_repeated())
		{
			return this->_ParseRepeatedInt32Value(p_pMessage, p_pField, p_pLuaState);
		}
		return this->_ParseInt32Value(p_pMessage, p_pField, p_pLuaState);
	}
	else if (eType == google::protobuf::FieldDescriptor::CPPTYPE_INT64)
	{
		if (p_pField->is_repeated())
		{
			return this->_ParseRepeatedInt64Value(p_pMessage, p_pField, p_pLuaState);
		}
		return this->_ParseInt64Value(p_pMessage, p_pField, p_pLuaState);
	}
	else if (eType == google::protobuf::FieldDescriptor::CPPTYPE_UINT32)
	{
		if (p_pField->is_repeated())
		{
			return this->_ParseRepeatedUInt32Value(p_pMessage, p_pField, p_pLuaState);
		}
		return this->_ParseUInt32Value(p_pMessage, p_pField, p_pLuaState);
	}
	else if (eType == google::protobuf::FieldDescriptor::CPPTYPE_UINT64)
	{
		if (p_pField->is_repeated())
		{
			return this->_ParseRepeatedUInt64Value(p_pMessage, p_pField, p_pLuaState);
		}
		return this->_ParseUInt64Value(p_pMessage, p_pField, p_pLuaState);
	}
	else if (eType == google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE)
	{
		if (p_pField->is_repeated())
		{
			return this->_ParseRepeatedFloat64Value(p_pMessage, p_pField, p_pLuaState);
		}
		return this->_ParseFloat64Value(p_pMessage, p_pField, p_pLuaState);
	}
	else if (eType == google::protobuf::FieldDescriptor::CPPTYPE_FLOAT)
	{
		if (p_pField->is_repeated())
		{
			return this->_ParseRepeatedFloat32Value(p_pMessage, p_pField, p_pLuaState);
		}
		return this->_ParseFloat32Value(p_pMessage, p_pField, p_pLuaState);
	}
	else if (eType == google::protobuf::FieldDescriptor::CPPTYPE_BOOL)
	{
		if (p_pField->is_repeated())
		{
			return this->_ParseRepeatedBoolValue(p_pMessage, p_pField, p_pLuaState);
		}
		return this->_ParseBoolValue(p_pMessage, p_pField, p_pLuaState);
	}
	else if (eType == google::protobuf::FieldDescriptor::CPPTYPE_ENUM)
	{
		if (p_pField->is_repeated())
		{
			return this->_ParseRepeatedEnumValue(p_pMessage, p_pField, p_pLuaState);
		}
		return this->_ParseEnumValue(p_pMessage, p_pField, p_pLuaState);
	}
	else if (eType == google::protobuf::FieldDescriptor::CPPTYPE_STRING)
	{
		if (p_pField->is_repeated())
		{
			return this->_ParseRepeatedStringValue(p_pMessage, p_pField, p_pLuaState);
		}
		return this->_ParseStringValue(p_pMessage, p_pField, p_pLuaState);
	}
	else if (eType == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
	{
		if (p_pField->is_repeated())
		{
			return this->_ParseRepeatedMessageValue(p_pMessage, p_pField, p_pLuaState);
		}
		return this->_ParseMessageValue(p_pMessage, p_pField, p_pLuaState);
	}
	else
	{
		return CCLOGERROR("Field \"%s\"'s Type(%d) Is Unsupported! Message Type : \"%s\".", p_pField->name().c_str(), static_cast<int32_t>(eType), p_pMessage->GetTypeName().c_str()), false;
	}
}

bool ProtocolGenerator::_ParseInt32Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState)
{
	int32_t nValue = p_pMessage->GetReflection()->GetInt32(*p_pMessage, p_pField);

	lua_pushstring(p_pLuaState, p_pField->name().c_str());
	lua_pushnumber(p_pLuaState, nValue);

	lua_rawset(p_pLuaState, -3);

	return true;
}

bool ProtocolGenerator::_ParseInt64Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState)
{
	int64_t nValue = p_pMessage->GetReflection()->GetInt64(*p_pMessage, p_pField);

	lua_pushstring(p_pLuaState, p_pField->name().c_str());

#if defined __LUA_SET_INT64_AS_STRING__
	char szValue[30] = {0};

	sprintf(szValue, "%lld", nValue);

	lua_pushstring(p_pLuaState, szValue);
#else
	lua_pushnumber(p_pLuaState, nValue);
#endif

	lua_rawset(p_pLuaState, -3);

	return true;
}

bool ProtocolGenerator::_ParseUInt32Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState)
{
	uint32_t uValue = p_pMessage->GetReflection()->GetUInt32(*p_pMessage, p_pField);

	lua_pushstring(p_pLuaState, p_pField->name().c_str());
	lua_pushnumber(p_pLuaState, uValue);

	lua_rawset(p_pLuaState, -3);

	return true;
}

bool ProtocolGenerator::_ParseUInt64Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState)
{
	uint64_t uValue = p_pMessage->GetReflection()->GetUInt64(*p_pMessage, p_pField);

	lua_pushstring(p_pLuaState, p_pField->name().c_str());
	
#if defined __LUA_SET_INT64_AS_STRING__
	char szValue[30] = {0};

	sprintf(szValue, "%llu", uValue);

	lua_pushstring(p_pLuaState, szValue);
#else
	lua_pushnumber(p_pLuaState, uValue);
#endif

	lua_rawset(p_pLuaState, -3);

	return true;
}

bool ProtocolGenerator::_ParseFloat32Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState)
{
	float32_t fValue = p_pMessage->GetReflection()->GetFloat(*p_pMessage, p_pField);

	lua_pushstring(p_pLuaState, p_pField->name().c_str());
	lua_pushnumber(p_pLuaState, fValue);

	lua_rawset(p_pLuaState, -3);

	return true;
}

bool ProtocolGenerator::_ParseFloat64Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState)
{
	float64_t fValue = p_pMessage->GetReflection()->GetDouble(*p_pMessage, p_pField);

	lua_pushstring(p_pLuaState, p_pField->name().c_str());
	lua_pushnumber(p_pLuaState, fValue);

	lua_rawset(p_pLuaState, -3);

	return true;
}

bool ProtocolGenerator::_ParseBoolValue(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState)
{
	bool bValue = p_pMessage->GetReflection()->GetBool(*p_pMessage, p_pField);

	lua_pushstring(p_pLuaState, p_pField->name().c_str());
	lua_pushboolean(p_pLuaState, bValue);

	lua_rawset(p_pLuaState, -3);

	return true;
}

bool ProtocolGenerator::_ParseStringValue(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState)
{
	std::string strValue = p_pMessage->GetReflection()->GetString(*p_pMessage, p_pField);

	lua_pushstring(p_pLuaState, p_pField->name().c_str());
	lua_pushstring(p_pLuaState, strValue.c_str());

	lua_rawset(p_pLuaState, -3);

	return true;
}

bool ProtocolGenerator::_ParseEnumValue(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState)
{
	const google::protobuf::EnumValueDescriptor * pEnumValueDescriptor = p_pMessage->GetReflection()->GetEnum(*p_pMessage, p_pField);

	if (nullptr == pEnumValueDescriptor)
	{
		return CCLOGERROR("Field \"%s\"'s EnumValueDescriptor Is NULL! Message Type : \"%s\".", p_pField->name().c_str(), p_pMessage->GetTypeName().c_str()), false;
	}

	lua_pushstring(p_pLuaState, p_pField->name().c_str());
	lua_pushnumber(p_pLuaState, pEnumValueDescriptor->number());

	lua_rawset(p_pLuaState, -3);

	return true;
}

bool ProtocolGenerator::_ParseMessageValue(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState)
{
	const google::protobuf::Message & cSubMessage = p_pMessage->GetReflection()->GetMessage(*p_pMessage, p_pField);

	google::protobuf::Message * pSubMessage = const_cast<google::protobuf::Message *>(&cSubMessage);

	lua_pushstring(p_pLuaState, p_pField->name().c_str());

	lua_newtable(p_pLuaState);

	bool bSuccess = this->ParseMessage(pSubMessage, p_pLuaState);

	lua_rawset(p_pLuaState, -3);

	return bSuccess;
}

bool ProtocolGenerator::_ParseRepeatedInt32Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState)
{
	int32_t nCount = p_pMessage->GetReflection()->FieldSize(*p_pMessage, p_pField);

	if (nCount <= 0)
	{
		return true;
	}

	lua_pushstring(p_pLuaState, p_pField->name().c_str());

	lua_newtable(p_pLuaState);

	for (int32_t i = 0; i < nCount; ++i)
	{
		int32_t nValue = p_pMessage->GetReflection()->GetRepeatedInt64(*p_pMessage, p_pField, i);

		lua_pushnumber(p_pLuaState, i + 1);
		lua_pushnumber(p_pLuaState, nValue);

		lua_rawset(p_pLuaState, -3);
	}

	lua_rawset(p_pLuaState, -3);

	return true;
}

bool ProtocolGenerator::_ParseRepeatedInt64Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState)
{
	int32_t nCount = p_pMessage->GetReflection()->FieldSize(*p_pMessage, p_pField);

	if (nCount <= 0)
	{
		return true;
	}

	lua_pushstring(p_pLuaState, p_pField->name().c_str());

	lua_newtable(p_pLuaState);

	for (int32_t i = 0; i < nCount; ++i)
	{
		int64_t nValue = p_pMessage->GetReflection()->GetRepeatedInt64(*p_pMessage, p_pField, i);

		lua_pushnumber(p_pLuaState, i + 1);

#if defined __LUA_SET_INT64_AS_STRING__
		char szValue[30] = {0};

		sprintf(szValue, "%lld", nValue);

		lua_pushstring(p_pLuaState, szValue);
#else
		lua_pushnumber(p_pLuaState, nValue);
#endif

		lua_rawset(p_pLuaState, -3);
	}

	lua_rawset(p_pLuaState, -3);

	return true;
}

bool ProtocolGenerator::_ParseRepeatedUInt32Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState)
{
	int32_t nCount = p_pMessage->GetReflection()->FieldSize(*p_pMessage, p_pField);

	if (nCount <= 0)
	{
		return true;
	}

	lua_pushstring(p_pLuaState, p_pField->name().c_str());

	lua_newtable(p_pLuaState);

	for (int32_t i = 0; i < nCount; ++i)
	{
		uint32_t uValue = p_pMessage->GetReflection()->GetRepeatedUInt32(*p_pMessage, p_pField, i);

		lua_pushnumber(p_pLuaState, i + 1);
		lua_pushnumber(p_pLuaState, uValue);

		lua_rawset(p_pLuaState, -3);
	}

	lua_rawset(p_pLuaState, -3);

	return true;
}

bool ProtocolGenerator::_ParseRepeatedUInt64Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState)
{
	int32_t nCount = p_pMessage->GetReflection()->FieldSize(*p_pMessage, p_pField);

	if (nCount <= 0)
	{
		return true;
	}

	lua_pushstring(p_pLuaState, p_pField->name().c_str());

	lua_newtable(p_pLuaState);

	for (int32_t i = 0; i < nCount; ++i)
	{
		uint64_t uValue = p_pMessage->GetReflection()->GetRepeatedUInt64(*p_pMessage, p_pField, i);

		lua_pushnumber(p_pLuaState, i + 1);

#if defined __LUA_SET_INT64_AS_STRING__
		char szValue[30] = {0};

		sprintf(szValue, "%llu", uValue);

		lua_pushstring(p_pLuaState, szValue);
#else
		lua_pushnumber(p_pLuaState, uValue);
#endif

		lua_rawset(p_pLuaState, -3);
	}

	lua_rawset(p_pLuaState, -3);

	return true;
}

bool ProtocolGenerator::_ParseRepeatedFloat32Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState)
{
	int32_t nCount = p_pMessage->GetReflection()->FieldSize(*p_pMessage, p_pField);

	if (nCount <= 0)
	{
		return true;
	}

	lua_pushstring(p_pLuaState, p_pField->name().c_str());

	lua_newtable(p_pLuaState);

	for (int32_t i = 0; i < nCount; ++i)
	{
		float32_t fValue = p_pMessage->GetReflection()->GetRepeatedFloat(*p_pMessage, p_pField, i);

		lua_pushnumber(p_pLuaState, i + 1);
		lua_pushnumber(p_pLuaState, fValue);

		lua_rawset(p_pLuaState, -3);
	}

	lua_rawset(p_pLuaState, -3);

	return true;
}

bool ProtocolGenerator::_ParseRepeatedFloat64Value(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState)
{
	int32_t nCount = p_pMessage->GetReflection()->FieldSize(*p_pMessage, p_pField);

	if (nCount <= 0)
	{
		return true;
	}

	lua_pushstring(p_pLuaState, p_pField->name().c_str());

	lua_newtable(p_pLuaState);

	for (int32_t i = 0; i < nCount; ++i)
	{
		float64_t fValue = p_pMessage->GetReflection()->GetRepeatedDouble(*p_pMessage, p_pField, i);

		lua_pushnumber(p_pLuaState, i + 1);
		lua_pushnumber(p_pLuaState, fValue);

		lua_rawset(p_pLuaState, -3);
	}

	lua_rawset(p_pLuaState, -3);

	return true;
}

bool ProtocolGenerator::_ParseRepeatedBoolValue(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState)
{
	int32_t nCount = p_pMessage->GetReflection()->FieldSize(*p_pMessage, p_pField);

	if (nCount <= 0)
	{
		return true;
	}

	lua_pushstring(p_pLuaState, p_pField->name().c_str());

	lua_newtable(p_pLuaState);

	for (int32_t i = 0; i < nCount; ++i)
	{
		bool bValue = p_pMessage->GetReflection()->GetRepeatedBool(*p_pMessage, p_pField, i);

		lua_pushnumber(p_pLuaState, i + 1);
		lua_pushboolean(p_pLuaState, bValue);

		lua_rawset(p_pLuaState, -3);
	}

	lua_rawset(p_pLuaState, -3);

	return true;
}

bool ProtocolGenerator::_ParseRepeatedStringValue(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState)
{
	int32_t nCount = p_pMessage->GetReflection()->FieldSize(*p_pMessage, p_pField);

	if (nCount <= 0)
	{
		return true;
	}

	lua_pushstring(p_pLuaState, p_pField->name().c_str());

	lua_newtable(p_pLuaState);

	for (int32_t i = 0; i < nCount; ++i)
	{
		std::string strValue = p_pMessage->GetReflection()->GetRepeatedString(*p_pMessage, p_pField, i);

		lua_pushnumber(p_pLuaState, i + 1);
		lua_pushstring(p_pLuaState, strValue.c_str());

		lua_rawset(p_pLuaState, -3);
	}

	lua_rawset(p_pLuaState, -3);

	return true;
}

bool ProtocolGenerator::_ParseRepeatedEnumValue(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState)
{
	int32_t nCount = p_pMessage->GetReflection()->FieldSize(*p_pMessage, p_pField);

	if (nCount <= 0)
	{
		return true;
	}

	bool bSuccess = true;

	lua_pushstring(p_pLuaState, p_pField->name().c_str());

	lua_newtable(p_pLuaState);

	for (int32_t i = 0; i < nCount; ++i)
	{
		bSuccess = false;

		const google::protobuf::EnumValueDescriptor * pEnumValueDescriptor = p_pMessage->GetReflection()->GetRepeatedEnum(*p_pMessage, p_pField, i);

		CC_BREAK_IF(nullptr == pEnumValueDescriptor);

		lua_pushnumber(p_pLuaState, i + 1);
		lua_pushnumber(p_pLuaState, pEnumValueDescriptor->number());

		lua_rawset(p_pLuaState, -3);

		bSuccess = true;
	}

	lua_rawset(p_pLuaState, -3);

	return bSuccess;
}

bool ProtocolGenerator::_ParseRepeatedMessageValue(google::protobuf::Message * p_pMessage, const google::protobuf::FieldDescriptor * p_pField, lua_State * p_pLuaState)
{
	int32_t nCount = p_pMessage->GetReflection()->FieldSize(*p_pMessage, p_pField);

	if (nCount <= 0)
	{
		return true;
	}

	bool bSuccess = true;

	lua_pushstring(p_pLuaState, p_pField->name().c_str());

	lua_newtable(p_pLuaState);

	for (int32_t i = 0; i < nCount; ++i)
	{
		bSuccess = false;

		const google::protobuf::Message & cSubMessage = p_pMessage->GetReflection()->GetRepeatedMessage(*p_pMessage, p_pField, i);

		google::protobuf::Message * pSubMessage = const_cast<google::protobuf::Message *>(&cSubMessage);

		lua_pushnumber(p_pLuaState, i + 1);

		lua_newtable(p_pLuaState);

		bSuccess = this->ParseMessage(pSubMessage, p_pLuaState);

		lua_rawset(p_pLuaState, -3);

		CC_BREAK_IF(!bSuccess);
	}

	lua_rawset(p_pLuaState, -3);

	return bSuccess;
}

NS_PROTOCOL_GENERATOR_END
