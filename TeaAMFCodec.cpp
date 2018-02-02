//
//  TeaAMFCodec.cpp
//  tea
//
//  Created by yaowei li on 11-7-14.
//  Copyright 2011年 touchage. All rights reserved.
//

#include "TeaAMFCodec.h"

using namespace tea;
using namespace cocos2d;


#define TEA_REFERENCE_BIT	0x01

#pragma -
#pragma construction dis
TeaAMFCodec::TeaAMFCodec() {
	m_aObjectRefTab = CCArray::createWithCapacity(4);
	m_aObjectRefTab->retain();
	m_aStringRefTab	= CCArray::createWithCapacity(4);
	m_aStringRefTab->retain();
    
    m_aTraitRefTab = CCArray::createWithCapacity(4);
	m_aTraitRefTab->retain();
}
TeaAMFCodec::~TeaAMFCodec() {
	CC_SAFE_RELEASE(m_aObjectRefTab);
	CC_SAFE_RELEASE(m_aStringRefTab);
    CC_SAFE_RELEASE(m_aTraitRefTab);
}

#pragma -
#pragma output AMFMessage

TeaAMFMessage* TeaAMFCodec::outputMessage() {
	assert(wptr-rptr > 0 );
	TeaAMFMessage*	message = new TeaAMFMessage();
	// clear ref
	m_aObjectRefTab->removeAllObjects();
	m_aStringRefTab->removeAllObjects();
    m_aTraitRefTab->removeAllObjects();
    
	message->autorelease();
	//版本号
	output(message->m_sVersion,true);
	
	// headers
	unsigned short headCount = getData<unsigned short>(true);
	//message->m_pHeaders->initWithCapacity(headCount);
	for (unsigned short i=0; i<headCount; ++i) {
		char	headName[TEA_MAX_STRING_LENGTH+1];
		bool	mustUnderstand;
		char	headValue[TEA_MAX_STRING_LENGTH+1];
		outputUTFString(headName);
		output(mustUnderstand);
		/**
		 *	空出 5 byte
		 **/
		(*this) += 4;
		
		char v;
		output(v);
		TeaASObject *object;
		if ( v == 0x11 )	// AMF3
		{
			object = getASObject();
		} else {
			outputUTFString(headValue);
			object = TeaASObject::objectWithString(headValue);
		}
		message->m_pHeaders->addObject(
									  TeaAMFMessageHeader::messageHeaderWithName(headName,
																				 object,
																				 mustUnderstand));
	}
	
	////change by bobo 索引head和body是分开的
	// clear ref
	m_aObjectRefTab->removeAllObjects();
	m_aStringRefTab->removeAllObjects();
    m_aTraitRefTab->removeAllObjects();
	////change by bobo 索引head和body是分开的
	
	//bodies
	unsigned short bodyCount = getData<unsigned short>(true);
	// body 只有一个
	assert(bodyCount == 1);
	
	char target[TEA_MAX_STRING_LENGTH+1];
	char response[TEA_MAX_STRING_LENGTH+1];
	
	outputUTFString(target);
	outputUTFString(response);
	/**
	 *	空出 5 byte
	 **/
	(*this) += 5;
	TeaASObject *object = getASObject();
	message->m_pBodies->addObject(
								 TeaAMFMessageBody::messageBodyWithTarget(target,response,object));
	
	//std::cout<<object->toString()<<std::endl;
	return message;
}

unsigned int TeaAMFCodec::getVarInt() {
	unsigned char byte;
//	int readnum = 0;
	int value = 0;
	
//	do {
//		++readnum;
//		output(byte);
//		value = ( readnum==4?value<<8:value<<7 ) | (readnum==4?byte:(byte & 0x7F ));
//		//value = ( value<<7 ) | (readnum==4?byte:(byte & 0x7F ));
//
//	} while ( (byte & 0x80) && ( readnum < 4 ) );
    
    do {
        output(byte);
        int b = byte &0xFF;
        if(b<128){
            value = b;
            break;
        }
        value = (b & 0x7F)<<7;
        
        output(byte);
        b = byte & 0xFF;
        if(b<128){
            value = value |b;
            break;
        }
        value = (value | (b & 0x7F))<<7;
        
        output(byte);
        b = byte &0xFF;
        if(b<128){
            value = value |b;
            break;
        }
        value = (value|(b & 0x7F))<<8;
        
        output(byte);
        b = byte&0xFF;
        value = (value|b);
        break;
        
    } while (true);
	
//	
//	if((value&0x10000000)!=0)
//		value = (value&0x10000000) | 0x80000000;
    
    value = (value<<3)>>3;
	
	return value;
}

TeaASObject* TeaAMFCodec::getASObject() {
	
	TeaASObject* object;
	DataType m_eType = (DataType)getData<unsigned char>();
	bool boolValue = false;
	int intValue = 0;
	double doubleValue = 0;
	
	switch (m_eType) {
		case DT_NULL:
            object = new TeaASObject();
            object->m_eType = m_eType;
            object->autorelease();
			break;
		case DT_FALSE:
            object = new TeaASObject();
            object->m_eType = m_eType;
			boolValue = false;
			object->setValue(boolValue);
            object->autorelease();
			break;
		case DT_TRUE:
            object = new TeaASObject();
            object->m_eType = m_eType;
			boolValue = true;
			object->setValue(boolValue);
            object->autorelease();
			break;
		case DT_INTEGER:
            object = new TeaASObject();
            object->m_eType = m_eType;
			intValue = getVarInt();
			object->setValue(intValue);
            object->autorelease();
			break;
		case DT_DOUBLE:
            object = new TeaASObject();
            object->m_eType = m_eType;
			output(doubleValue,true);
			object->setValue(doubleValue);
            object->autorelease();
			break;
		case DT_STRING:
			object = outputStringObject();
			break;
		case DT_DATE:
			object = outputDateObject();
			break;
		case DT_ARRAY:
			object = outputArrayObject();
			break;
		case DT_OBJECT:
			object = outputObject();
			break;
		case DT_BYTEARRAY:
			object = outputByteArrayObject();
			break;
		default:
			assert(false);
	}
//	m_aObjectRefTab->addObject(object);
	return object;
}
TeaASObject* TeaAMFCodec::outputByteArrayObject() {
	int head = getVarInt();
	if ( ( head & TEA_REFERENCE_BIT ) == 0 )	// 梁钟波说可能有误，请梁钟波跟踪
		return (TeaASObject*)m_aObjectRefTab->objectAtIndex(head>>1);
	
	assert(rptr + (head>>1) <= wptr);
	TeaASObject* object = TeaASObject::objectWithString(buffer,head>>1);
	rptr += (head>>1);
	
	return object;
}

TeaASObject* TeaAMFCodec::readTraits(int ref)
{
    if ((ref & 3) == 1) // This is a reference
        return (TeaASObject*)(m_aTraitRefTab->objectAtIndex(ref >> 2));
    
    bool externalizable = ((ref & 4) == 4);
    bool dynamic = ((ref & 8) == 8);
    int count = (ref >> 4); /* uint29 */
    TeaASObject* className = outputStringObject();
    
    TeaASObject* object = TeaASObject::objectWithClassname(className->m_pValue,0);
    m_aTraitRefTab->addObject(object);
    object->m_dynamic = dynamic;
    object->m_externalizable = externalizable;
    
    
//    CCArray	*names = CCArray::createWithCapacity(count);
	for ( int i=0; i<count; ++i ) {
        TeaASObject* name = outputStringObject();
		object->addChild(name);
	}
    
    return object;
}


TeaASObject* TeaAMFCodec::outputObject() {
	int head = getVarInt();
	if ( ( head & TEA_REFERENCE_BIT ) == 0 )
		return (TeaASObject*)m_aObjectRefTab->objectAtIndex(head>>1);
    
    
//    if(!(( head & 0x02 ) && ( (head & 0x04) == 0))){
//        cocos2d::CCLOG("error");
//        
//        assert( ( head & 0x02 ) && ( (head & 0x04) == 0) );
//    }
    
    TeaASObject* traitsInfo = this->readTraits(head);
    bool dynamic = traitsInfo->m_dynamic;
    int count = traitsInfo->getChildren()->count();
    
    
//	bool dynamic = ( head & 0x08 ) ;
//	int count = head>>4;
//	TeaASObject* object = TeaASObject::objectWithClassname(outputStringObject()->m_pValue,count);
    
    TeaASObject* object = TeaASObject::objectWithClassname(traitsInfo->getClassname().c_str(),count);
    
    
	m_aObjectRefTab->addObject(object);//2011.09.05
    
	CCArray	*names = CCArray::createWithCapacity(count);
	for ( int i=0; i<count; ++i ) {
//        TeaASObject* name = outputStringObject();
//		names->addObject(name);
        names->addObject(traitsInfo->getChildren()->objectAtIndex(i));
	}
	
	for ( int i=0; i<count; ++i ) {
//		TeaASObject* sub = outputObject();//bobo 2013
//        std::string s1 =(char*)((TeaASObject*)names->objectAtIndex(i))->m_pValue;
//        if(strcmp(s1.c_str(), "lastLogin")==0){
//            CCLog("asdf");
//        }
//        cocos2d::CCLOG("******************************************%d----%s",i,((TeaASObject*)names->objectAtIndex(i))->getValue());
        TeaASObject* sub = getASObject();
		TEA_COPY_STRING(sub->m_pName, (char*)((TeaASObject*)names->objectAtIndex(i))->m_pValue);
		object->m_aChildren->addObject(sub);
		
	}
//	cocos2d::CCLOG("end");
	if ( dynamic ) {
		for (;;) {
			TeaASObject* name = outputStringObject();
			if ( name->m_sValueLen == 0 ) break;
			TeaASObject* sub = getASObject();
			TEA_COPY_STRING(sub->m_pName, (char*)name->m_pValue);
			object->m_aChildren->addObject(sub);
		}
	}
	//m_aObjectRefTab->addObject(object); by bobo 2011.09.5
	return object;
}
TeaASObject* TeaAMFCodec::outputArrayObject() {
	int head = getVarInt();
	if ( ( head & TEA_REFERENCE_BIT ) == 0 )
		return (TeaASObject*)m_aObjectRefTab->objectAtIndex(head>>1);
	TeaASObject* name = outputStringObject();
	if ( name->m_sValueLen == 0 ) {
		//Array
		int size = head>>1;
		TeaASObject *array = TeaASObject::objectArrayWithCapacity(size);
		m_aObjectRefTab->addObject(array);//by bobo 2011.09.05
		for ( int i=0;i<size;++i )
			array->m_aChildren->addObject(getASObject());
		//m_aObjectRefTab->addObject(array);by bobo 2011.09.05
		return array;
	}else{
		
		// map 梁钟波说不支持    梁钟波：不是不支持，map是当作object处理的
        //map 2013
        TeaASObject *map = TeaASObject::objectWithoutClassName(0);
        m_aObjectRefTab->addObject(map);
        
        while (name!=NULL && name->m_sValueLen>0) {
//            CCLOG("name-------------------------------------%s",name->getValue());
            TeaASObject* sub = this->getASObject();
            
//            TeaASObject* keyValue = TeaASObject::objectWithKeyAndValue((const char*)name->getValue(),(const char*)values->getValue());
            
            TEA_COPY_STRING(sub->m_pName, (const char*)name->getValue());
            
            map->addChild(sub);
            
            name = outputStringObject();
        }
        
        return map;
//		assert(false);
	}
}
TeaASObject* TeaAMFCodec::outputDateObject() {
	int head = getVarInt();
	if ( ( head & TEA_REFERENCE_BIT ) == 0 ) {
		int indexC = head>>1;
		if(indexC>=m_aObjectRefTab->count()){
			CCLOG("error here");
		}
		return (TeaASObject*)m_aObjectRefTab->objectAtIndex(indexC);
	}
	double time;
	output(time,true);
	TeaASObject* object = TeaASObject::objectWithValue(time,DT_DATE);
	m_aObjectRefTab->addObject(object);
	return object;
}
TeaASObject* TeaAMFCodec::outputStringObject() {
	
	int head = getVarInt();
	
	if ( ( head & TEA_REFERENCE_BIT ) == 0 ) {
		// reference;
		//引用，必须复制string,不能直接返回TeaAsObject，否则会有问题
		int indexC = head>>1;
		if(indexC>=m_aStringRefTab->count()){
			CCLOG("error here");
		}
		TeaASObject *t = (TeaASObject*)m_aStringRefTab->objectAtIndex(indexC);
		std::string ss = std::string((char*)t->getValue());
        if(ss.compare("SoldierConfig")==0){
            CCLOG("asdfadsf");
        }
		return TeaASObject::objectWithString(ss.c_str());
	}	
	int len = head>>1;
	if ( len <= 0 ) return TeaASObject::objectWithString("");
	
    int lengths =TEA_MAX_STRING_LENGTH;
    if(len>TEA_MAX_STRING_LENGTH){
        lengths = len;
    }
    
	char buf[lengths+1];
	outputUTFString(buf,len);
	TeaASObject *object = TeaASObject::objectWithString(buf);
	m_aStringRefTab->addObject(object);
//	if(std::string(buf) == "1_great_wall"){
//		std::cout<<"hereeeee"<<std::endl;
//	}
	return object;
}

void TeaAMFCodec::outputUTFString(char* str,unsigned short len) {
	if ( len == 0 )
		output(len,true);
	if ( len <= 0 ) return;
	//assert(len <= TEA_MAX_STRING_LENGTH);
	if(len>(wptr - rptr)) return;
	assert(len <= wptr - rptr);
	unsigned char c1,c2,c3;
	unsigned int pstr=0;
//	U-00000000 - U-0000007F: 0xxxxxxx  
//	U-00000080 - U-000007FF: 110xxxxx 10xxxxxx  
//	U-00000800 - U-0000FFFF: 1110xxxx 10xxxxxx 10xxxxxx  
//	U-00010000 - U-001FFFFF: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx  
//	U-00200000 - U-03FFFFFF: 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx  
//	U-04000000 - U-7FFFFFFF: 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx  
	bool flag = false;
	for (unsigned short i=0; i<len; ++i) {
		output(c1);
//		if ( c1 & 0x80 ) {
//			output(c2);
//			++i;
//			if (!( c1 & 0x20)) {
//				str[pstr++] = (char)(((c1&0x1F)<<6)|(c2&0x3f));
//				flag = true;
//			} else {
//				output(c3);
//				i++;
//				str[pstr++] = (char)(((c1&0x0F)<<12)|((c2&0x3F)<<6)|((c3&0x3F)));
//			}
//		} else {
			str[pstr++] = (char)c1;
//		}
	}
	str[pstr] = 0;
	
	//if(flag){
//		std::string sss(str);
//		std::cout<<sss<<std::endl;
//	}
}

#pragma -
#pragma input AMFMessage

void TeaAMFCodec::inputMessage(TeaAMFMessage* message){
	m_aObjectRefTab->removeAllObjects();
	m_aStringRefTab->removeAllObjects();
	clear();
	//message->m_sVersion = 0;
	input(message->m_sVersion,true);
	unsigned short headCount = message->m_pHeaders->count();
	input(headCount,true);
	CCObject *ccobject;
	CCARRAY_FOREACH(message->m_pHeaders, ccobject) {
		TeaAMFMessageHeader	*header = (TeaAMFMessageHeader*)ccobject;
		inputUTFString(header->m_pName);
		
		input(header->m_bMustUnderstand);
		move(4,true);	// 空四个byte
		input((char)0x11);	// AMF3
		
		inputASObject(header->m_pData);
	}
	
	/////////////////head 和 body的索引是分开的 change by bobo //////////
	m_aObjectRefTab->removeAllObjects();
	m_aStringRefTab->removeAllObjects();
	/////////////////head 和 body的索引是分开的 change by bobo //////////
	
	assert(message->m_pBodies->count() <= 1);
	input((unsigned short)1,true);
	TeaAMFMessageBody *body = (TeaAMFMessageBody*) ( message->m_pBodies->objectAtIndex(0) );
	inputUTFString(body->m_pTarget);
	inputUTFString(body->m_pResponse);
	
	move(4,true);	//空四个byte
	input((char)0x11);	//AMF3
	inputASObject(body->m_pData);
}

void TeaAMFCodec::inputUTFString(const char* str,bool isObject) {
	unsigned short len = strlen(str);
	if ( isObject )
		inputVarInt(len<<1 | 1);
	else 
		input(len,true);
	if ( len == 0 )
		return;
	//	U-00000000 - U-0000007F: 0xxxxxxx  
	//	U-00000080 - U-000007FF: 110xxxxx 10xxxxxx  
	//	U-00000800 - U-0000FFFF: 1110xxxx 10xxxxxx 10xxxxxx  
	//	U-00010000 - U-001FFFFF: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx  
	//	U-00200000 - U-03FFFFFF: 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx  
	//	U-04000000 - U-7FFFFFFF: 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx  
	
	
	/**
	 unsigned char* bytearr = new unsigned char[utflen+1];
	 for(int i=0;i<strlen;i++){
	 c = charr[i];
	 if(c<=0x007F){
	 bytearr[count++] = (unsigned char)c;
	 }else if(c>0x07FF){
	 bytearr[count++] = (unsigned char)(0xE0|((c>>12)&0x0F));
	 bytearr[count++] = (unsigned char)(0x80|((c>>6)&0x3F));
	 bytearr[count++] = (unsigned char)((c>>0)&&0x3F);
	 }else{
	 bytearr[count++] = (unsigned char)(0xC0|((c>>6)&0x1F));
	 bytearr[count++] = (unsigned char)((c>>0)&0x3F);
	 }
	 }
	 原算法肯定有误，请梁钟波更正。 暂时支持ascii
	 **/
	//			for ( unsigned short i=0; i<len ; ++i ) {
	//				assert((str[i] & 0x80) == 0);
	//			}
	inputArray(str,len);
}
void TeaAMFCodec::inputVarInt(unsigned int value) {
	if ( value <= 0x7F ) {
		input((char)value);
		return;
	}
	if ( value <= 0x3FFF ) {
		value = ( ( value & 0x3F80 ) << 1 ) | ( value & 0x7F ) | 0x8000;
		input((unsigned short)value,true);
		return;
	}
	if ( value <=0x1FFFFF ) {
		value = ( ( value & 0x1FC000 ) << 2 ) | ( ( value & 0x3F80 ) << 1 ) | ( value & 0x7F ) | 0x808000;
		input((unsigned short)(value>>8),true);
		input((unsigned char)(value & 0x7F));
		return;
	}
//	assert( value <= 0x0FFFFFFF );
//	value = ( ( value & 0x0FE00000 ) << 3 ) | ( ( value & 0x1FC000 ) << 2 ) | ( ( value & 0x3F80 ) << 1 ) | ( value & 0x7F ) | 0x80808000 ;
//	input(value,true);
    
    
    
    input((unsigned char)(((value>>22)&0x7F)|0x80));
    input((unsigned char)(((value>>15)&0x7F)|0x80));
    input((unsigned char)(((value>>8)&0x7F)|0x80));
    input((unsigned char)(value & 0xFF));
    
    
    
    
    
	
}
void TeaAMFCodec::inputStringObject(TeaASObject* object) {
	assert(object->m_eType == DT_STRING);
	if ( object->m_sValueLen == 0 ) {
		input((char)0x01);
		return;
	}
	/**
	 *	索引暂时只支持同对象索引。未做值判断
	 **/
	int index = m_aStringRefTab->indexOfObject(object);
	if ( index != -1) {
		if(index>=m_aStringRefTab->count()){
			CCLOG("error here");
		}
		inputVarInt(index<<1);
		return;
	}
	
	inputUTFString((const char*)object->m_pValue,true);
	m_aStringRefTab->addObject(object);
}

void TeaAMFCodec::inputAMFInt(int i)
{
    i = i & 0x1FFFFFFF;
    inputVarInt(i);
    
}
void TeaAMFCodec::inputASObject(TeaASObject *object) {
	input((char)object->m_eType);
	switch (object->m_eType) {
		case DT_UNDEFINED:
		case DT_NULL:
		case DT_FALSE:
		case DT_TRUE:
			break;
		case DT_INTEGER:
			inputAMFInt(object->getValue<int>());
			break;
		case DT_DATE:
			input((char)0x01);	// why ? 梁钟波回答  答：这是u29表示接下来的DT_DATE直不是索引  bobo
			input(object->getValue<double>()); //用double来表示时间 bobo
		case DT_DOUBLE:
			input(object->getValue<double>());
			break;
		case DT_STRING:
			inputStringObject(object);
			break;
		case DT_ARRAY:
			inputVarInt( (object->m_aChildren->count())<<1|1 );
			// 空字符串 Object 
			input((char)0x01);
			CCObject *ccobject;
			CCARRAY_FOREACH(object->m_aChildren, ccobject) {
				inputASObject((TeaASObject*)ccobject);
			}
			break;
		case DT_OBJECT:
			// objref
			inputVarInt(0x0B);
			if(object->m_pClassname!=NULL){
				inputUTFString(object->m_pClassname,true);
			}else{
				//inputVarInt(0x01);
				input((char)0x01);
			}
			CCARRAY_FOREACH(object->m_aChildren, ccobject) {
				TeaASObject* tObject = (TeaASObject*)ccobject;
				inputUTFString(tObject->m_pName,true);
				inputASObject(tObject);
			}
			// 空字符串 Object 
			input((char)0x01);
			break;
		case DT_BYTEARRAY:
			inputVarInt(object->m_sValueLen<<1 | 1);
			inputArray((char*)object->m_pValue,object->m_sValueLen);
			break;
		default:
			assert(false);
	}
}

