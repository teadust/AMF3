//
//  TeaASObject.cpp
//  tea
//
//  Created by yaowei li on 11-7-13.
//  Copyright 2011年 touchage. All rights reserved.
//

#include "TeaASObject.h"

using namespace tea;
using namespace cocos2d;

TeaASObject::TeaASObject():
m_pClassname(NULL),
m_pName(NULL),
m_eType(DT_UNDEFINED),
m_pValue(NULL),
m_sValueLen(0),
m_aChildren(NULL)
{
    m_dynamic = false;
    m_externalizable = false;
}
TeaASObject::~TeaASObject() {
	//CCLOG("TeaASObject ----- remove");
	CC_SAFE_DELETE(m_pClassname);
	CC_SAFE_DELETE(m_pName);
	CC_SAFE_FREE(m_pValue);
	CC_SAFE_RELEASE(m_aChildren);
}
void TeaASObject::clear() {
	//CCLOG("TeaASObject ----- clear");
	CC_SAFE_DELETE(m_pClassname);
	CC_SAFE_DELETE(m_pName);
	CC_SAFE_FREE(m_pValue);
	m_sValueLen = 0;
	
	m_aChildren->removeAllObjects();
}

#pragma -
#pragma static create function

TeaASObject* TeaASObject::objectArrayWithCapacity(size_t capacity) {
	TeaASObject* object = new TeaASObject();
	object->autorelease();
	object->m_eType = DT_ARRAY;
	object->m_aChildren = CCArray::createWithCapacity(capacity);
	object->m_aChildren->retain();
	
	return object;
}
TeaASObject* TeaASObject::objectWithClassname(const char* classname,size_t capacity) {
	TeaASObject* object = new TeaASObject();
	object->autorelease();
	object->m_eType = DT_OBJECT;
	object->m_aChildren = CCArray::createWithCapacity(capacity);
	object->m_aChildren->retain();
	TEA_COPY_STRING(object->m_pClassname, classname);
	return object;
}

TeaASObject* TeaASObject::objectWithString(const char* value,unsigned int len) {
	TeaASObject* object = new TeaASObject();
	object->autorelease();
	if ( len == -1 ) {
		len = strlen(value) + 1;
		object->m_sValueLen = len-1;
		object->m_eType = DT_STRING;
	} else {
		object->m_sValueLen = len;
		object->m_eType = DT_BYTEARRAY;
	}
	object->m_pValue = (char*)malloc(len);
	memcpy(object->m_pValue,value,len);
	return object;
}

TeaASObject* TeaASObject::objectWithInt(int value)
{
    TeaASObject* object = new TeaASObject();
    object->m_eType = DT_INTEGER;
    object->setValue(value);
    object->autorelease();
    return object;
}

TeaASObject* TeaASObject::objectWithKeyAndValue(const char* key, const char* value){
	TeaASObject* object = new TeaASObject();
    object->autorelease();
	object->m_eType = DT_OBJECT;
	unsigned int len = strlen(key) + 1;
	object->m_sValueLen = len-1;
	object->m_pName = (char*)malloc(len);
	memcpy(object->m_pName, key, len);
	
	TEA_COPY_STRING(object->m_pValue, value);
	return object;
}

TeaASObject* TeaASObject::objectWithoutClassName(size_t capacity)
{
	TeaASObject* object = new TeaASObject();
    object->autorelease();
	object->m_eType = DT_OBJECT;
	object->m_aChildren = CCArray::createWithCapacity(capacity);
	object->m_aChildren->retain();
	return object;
}

