#ifndef _DEF_JSON_H
#define _DEF_JSON_H

#include <string>
#include <vector>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/rapidjson.h"

/************************************************************************/
/* 

struct MyStruct
{
	int a;
	int b;
	int c;

	//序列化
	BEGIN_SERIALIZE_TO_JSON
		ADD_DOC_JSON(a)
		ADD_DOC_JSON(b)
		ADD_DOC_JSON(c)

		ADD_OBJ_JSON_INIT(myobj)
		ADD_OBJ_JSON_MEMBER(myobj, a)
		ADD_OBJ_JSON_MEMBER(myobj, b)
		ADD_OBJ_JSON_MEMBER(myobj, c)
		ADD_DOC_JSON(myobj)

		ADD_ARRAY_JSON_INIT(myarr)
		ADD_ARRAY_JSON_MEMBER(myarr, a)
		ADD_ARRAY_JSON_MEMBER(myarr, b)
		ADD_ARRAY_JSON_MEMBER(myarr, c)
		ADD_DOC_JSON(myarr)
	END_SERIALIZE_TO_JSON


	//反序列化
	BEGIN_DESERIALIZE_FROM_JSON
		a = GET_DOC_JSON_MEMBER("a", int32);
		b = GET_DOC_JSON_MEMBER("b", int32);
		c = GET_DOC_JSON_MEMBER("c", int32);

		a = 3;
		b = 1;
		c = 2;

		if (json_check_is_obj(doc, "myobj"))
		{
			auto& obj = doc["myobj"];

			a = GET_OBJ_JSON_MEMBER(obj, "a", int32);
			b = GET_OBJ_JSON_MEMBER(obj, "b", int32);
			c = GET_OBJ_JSON_MEMBER(obj, "c", int32);
		}

		a = 3;
		b = 1;
		c = 2;

		if (json_check_is_array(doc, "myarr"))
		{
			auto& arr = doc["myarr"];

			for (int i = 0; i < arr.Size(); i++)
			{
				switch (i)
				{
					case 0:
						a = GET_JSON_VALUE(arr[i], int32);
						break;
					case 1:
						b = GET_JSON_VALUE(arr[i], int32);
						break;
					case 2:
						c = GET_JSON_VALUE(arr[i], int32);
						break;
					default:
						break;
				}
			}
		}

	END_DESERIALIZE_FROM_JSON

}
*/
/************************************************************************/

// 基础变量的校验  
#define json_check_is_bool(value, strKey) (value.HasMember(strKey) && value[strKey].IsBool())  
#define json_check_is_string(value, strKey) (value.HasMember(strKey) && value[strKey].IsString())  
#define json_check_is_int32(value, strKey) (value.HasMember(strKey) && value[strKey].IsInt())  
#define json_check_is_uint32(value, strKey) (value.HasMember(strKey) && value[strKey].IsUint())  
#define json_check_is_int64(value, strKey) (value.HasMember(strKey) && value[strKey].IsInt64())  
#define json_check_is_uint64(value, strKey) (value.HasMember(strKey) && value[strKey].IsUint64())  
#define json_check_is_float(value, strKey) (value.HasMember(strKey) && value[strKey].IsFloat())  
#define json_check_is_double(value, strKey) (value.HasMember(strKey) && value[strKey].IsDouble())  

#define json_check_is_number(value, strKey) (value.HasMember(strKey) && value[strKey].IsNumber())  
#define json_check_is_array(value, strKey) (value.HasMember(strKey) && value[strKey].IsArray())  
#define json_check_is_obj(value, strKey) (value.HasMember(strKey) && value[strKey].IsObject())  

// 得到对应类型的数据，如果数据不存在则得到一个默认值  
#define json_check_bool(value, strKey) (json_check_is_bool(value, strKey) && value[strKey].GetBool())  
#define json_check_string(value, strKey) (json_check_is_string(value, strKey) ? value[strKey].GetString() : "")  
#define json_check_int32(value, strKey) (json_check_is_int32(value, strKey) ? value[strKey].GetInt() : 0)  
#define json_check_uint32(value, strKey) (json_check_is_uint32(value, strKey) ? value[strKey].GetUint() : 0)  
#define json_check_int64(value, strKey) (json_check_is_int64(value, strKey) ? ((value)[strKey]).GetInt64() : 0)  
#define json_check_uint64(value, strKey) (json_check_is_uint64(value, strKey) ? ((value)[strKey]).GetUint64() : 0)  
#define json_check_float(value, strKey) (json_check_is_float(value, strKey) ? ((value)[strKey]).GetFloat() : 0)  
#define json_check_double(value, strKey) (json_check_is_double(value, strKey) ? ((value)[strKey]).GetDouble() : 0)  


#define json_bool(value) (value.IsBool() ? value.GetBool() : false)  
#define json_string(value) (value.IsString() ? value.GetString() : "")  
#define json_int32(value) (value.IsInt() ? value.GetInt() : 0)  
#define json_uint32(value) (value.IsUint() ? value.GetUint() : 0)  
#define json_int64(value) (value.IsInt64() ? value.GetInt64() : 0)  
#define json_uint64(value) (value.IsUint64() ? value.GetUint64() : 0)  
#define json_float(value) (value.IsFloat() ? value.GetFloat() : 0)  
#define json_double(value) (value.IsDouble() ? value.GetDouble() : 0)  


// 得到Value指针  
#define json_check_value_ptr(value, strKey) (((value).HasMember(strKey)) ? &((value)[strKey]) : nullptr)  


//json 序列化
#define  BEGIN_SERIALIZE_TO_JSON  \
	     void OnInitSerializeToJson(std::string& out){\
              rapidjson::Document document;\
              document.SetObject();\
		      rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
              
#define  ADD_DOC_JSON(member) \
              document.AddMember(#member, member, allocator);

#define  ADD_OBJ_JSON_INIT(objname)\
         rapidjson::Value objname(rapidjson::kObjectType);


#define  ADD_OBJ_JSON_MEMBER(objname,member) \
        objname.AddMember(#member, member, allocator);

#define  ADD_ARRAY_JSON_INIT(arryname)\
         rapidjson::Value arryname(rapidjson::kArrayType);

#define  ADD_ARRAY_JSON_MEMBER(arryname,value) \
        arryname.PushBack(value,allocator);

#define  END_SERIALIZE_TO_JSON  \
          rapidjson::StringBuffer buffer;\
          rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);\
          document.Accept(writer);\
          out = buffer.GetString();\
		}

//json 反序列化
#define  BEGIN_DESERIALIZE_FROM_JSON \
        void OnJsonStringDeSerialize(std::string& jsonStr)\
		{ \
			rapidjson::Document doc;\
			doc.Parse<0>(jsonStr.c_str());
        
#define  GET_DOC_JSON_MEMBER(member,type)  json_check_##type(doc,member);

#define  GET_OBJ_JSON_MEMBER(obj,member,type)  json_check_##type(obj,member);

#define  GET_JSON_VALUE(value,type) json_##type(value);

#define  END_DESERIALIZE_FROM_JSON \
        }

#endif
