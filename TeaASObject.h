//
//  TeaASObject.h
//  tea
//
//  Created by yaowei li on 11-7-13.
//  Copyright 2011年 touchage. All rights reserved.
//
#ifndef __TEA_AS_OBJECT_H__
#define __TEA_AS_OBJECT_H__

#include "cocos2d.h"

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include "tea.h"

#include "cppUtils.h"

using namespace cocos2d;

namespace tea {
	enum DataType
	{
		DT_UNDEFINED = 0x00,	//unsupport
		DT_NULL = 0x01,
		DT_FALSE = 0x02,
		DT_TRUE = 0x03,
		DT_INTEGER = 0x04,
		DT_DOUBLE = 0x05,
		DT_STRING = 0x06,
		DT_XMLDOC = 0x07,		//unsupport
		DT_DATE = 0x08,
		DT_ARRAY = 0x09,
		DT_OBJECT = 0x0A,
		DT_XML = 0x0B,			//unsupport
		DT_BYTEARRAY = 0x0C		
	};
	class TeaAMFCodec;
	class TeaASObject : public CCObject {
		friend class TeaAMFCodec;
	private:
		char* m_pClassname;
		
		char*	m_pValue;
		size_t	m_sValueLen;
		
		CCArray	*m_aChildren;
		

	
	public:
        bool m_dynamic;
        bool m_externalizable;
		char* m_pName;
		DataType	m_eType;
		TeaASObject();
		~TeaASObject();
		void clear();
		
//		virtual void release(bool isWeakRefrence=false){};
//		virtual void retain(bool isWeakRefrence=false){};
		
		
		template <typename T>
		void setValue(const T& value,DataType type=DT_UNDEFINED) {
			size_t s = sizeof(value);
			CC_SAFE_FREE(m_pValue);
			m_pValue = (char*)malloc(s);
			memcpy(m_pValue,&value,s);
			if ( type == DT_UNDEFINED ) {
				switch (s) {
					case 1:
						m_eType = value?DT_TRUE:DT_FALSE;
						break;
					case 2:
						m_eType = DT_INTEGER;
						break;
					case 4:
						m_eType = ( (int)value <= 0x0FFFFFFF )?DT_INTEGER:DT_DOUBLE;
						break;
					case 8:
						m_eType = DT_DOUBLE;
						break;
				}
				
			} else {
				m_eType = type;
			}
			m_sValueLen = s;
		}
		template <typename T>
		inline T& getValue() {
			// 简单数据类型
			assert(m_eType < DT_STRING);
			return *((T*)m_pValue);
		}
		
		//add by Haichao Fan
		inline int getIntValue(){
            
            if(m_eType==DT_NULL){
                return 0;
            }
            if(m_eType == DT_FALSE)return 0;
            if(m_eType == DT_TRUE)return 1;
            
			assert(m_eType == DT_INTEGER || m_eType == DT_DOUBLE || m_eType == DT_STRING);
			
			if(m_eType == DT_DOUBLE){
				return (int)this->getValue<double>();
			}
			
			if(m_eType == DT_STRING){
				std::string tmpStr((char*)m_pValue);
				return StringToNumber<int>(tmpStr);
			}
			
			return this->getValue<int>();
			
		}
		
		inline float getFloatValue(){
            if(m_eType==DT_NULL){
                return 0;
            }
			assert(m_eType == DT_INTEGER || m_eType == DT_DOUBLE || m_eType == DT_STRING);
			
			if(m_eType == DT_DOUBLE){
				return (float)this->getValue<double>();
			}
			
			if(m_eType == DT_STRING){
				std::string tmpStr((char*)m_pValue);
				return StringToNumber<float>(tmpStr);
			}
			
			return (float)this->getValue<int>();
		}
        
        inline double getDoubleValue(){
            if(m_eType==DT_NULL){
                return 0;
            }
            assert(m_eType == DT_INTEGER || m_eType == DT_DOUBLE || m_eType == DT_STRING);
			
			if(m_eType == DT_DOUBLE){
				return this->getValue<double>();
			}
			
			if(m_eType == DT_STRING){
				std::string tmpStr((char*)m_pValue);
				return StringToNumber<float>(tmpStr);
			}
			
			return (float)this->getValue<int>();
        }
		
		inline void* getValue() {
			if(m_eType == DT_ARRAY || m_eType == DT_OBJECT){
				return getChildren();
			}
			return m_pValue;
		}
		
		inline DataType getValueType() {
			return m_eType;
		}
        inline bool isNullValue(){
            return m_eType == DT_NULL;
        }
		
		void addChild(TeaASObject* child) {
			assert(m_eType == DT_ARRAY || m_eType == DT_OBJECT);
			if ( m_aChildren == NULL ) {
				m_aChildren = CCArray::createWithCapacity(4);
				m_aChildren->retain();
			}
			m_aChildren->addObject(child);
		}
		
		// 由樊海潮实现
		void addChildren(TeaASObject* child,...) {
			assert(false);
		}
		template<typename T>
		static TeaASObject* objectWithValue(T& value,DataType type=DT_UNDEFINED) {
			TeaASObject* object = new TeaASObject();
			object->autorelease();
			object->setValue(value,type);
			return object;
		}
		
		static TeaASObject* objectArrayWithCapacity(size_t capacity=4);
		static TeaASObject* objectWithClassname(const char* classname,size_t capacity=4);
		static TeaASObject* objectWithoutClassName(size_t capacity);
		
		static TeaASObject* objectWithString(const char* value,unsigned int len=-1);
		//处理map<std::string,std::string>构造TeaASObject
		static TeaASObject* objectWithKeyAndValue(const char* key, const char* value);
        
        static TeaASObject* objectWithInt(int value);
		
		
		TeaASObject* valueForKey(std::string key){
			if(m_eType == DT_NULL){
				return NULL;
			}

//			assert(m_eType == DT_ARRAY || m_eType == DT_OBJECT);
//			assert(m_aChildren != NULL);
			
			CCObject* obj;
			CCARRAY_FOREACH(m_aChildren, obj) {
				TeaASObject* object = (TeaASObject*)obj;
				if(object->m_pName != NULL){
					std::string tmpstr(object->m_pName);
					if(tmpstr == key){
						return object;
					}
				}
			}
			
			return NULL;
		}
		
		inline CCArray* getChildren(){
			if(m_eType == DT_ARRAY || m_eType == DT_OBJECT){
				return m_aChildren;
			}else{
				return NULL;
			}
		}
		/*
		inline char* getClassname(){
			assert(m_eType == DT_OBJECT);
			return m_pClassname;
		}*/
		
		inline std::string getClassname(){
//			assert(m_eType == DT_OBJECT);
            if(m_pClassname==NULL)return "";
			return std::string(m_pClassname);
		}
        
        inline std::string getStringValue(){
            if(m_eType == DT_NULL){
				return "";
			}
            return std::string(m_pValue);
        }
        
	};
}


#endif