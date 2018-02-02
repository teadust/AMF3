//
//  TeaAMFMessage.cpp
//  tea
//
//  Created by yaowei li on 11-7-14.
//  Copyright 2011年 touchage. All rights reserved.
//

#include "TeaAMFMessage.h"

using namespace tea;
using namespace cocos2d;

#pragma -
#pragma MessageBody

TeaAMFMessageBody::TeaAMFMessageBody():
m_pTarget(NULL),
m_pResponse(NULL),
m_pData(NULL) {
	
}
TeaAMFMessageBody::~TeaAMFMessageBody() {
	CC_SAFE_DELETE(m_pTarget);
	CC_SAFE_DELETE(m_pResponse);
	//CC_SAFE_DELETE(m_pData);
	CC_SAFE_RELEASE(m_pData);
}

TeaAMFMessageBody* TeaAMFMessageBody::messageBodyWithTarget(const char* target,const char* response,TeaASObject* data) {
	TeaAMFMessageBody* body = new TeaAMFMessageBody();
	body->autorelease();
	TEA_COPY_STRING(body->m_pTarget, target);
	TEA_COPY_STRING(body->m_pResponse, response);
	body->m_pData = data;
	body->m_pData->retain();
	return body;
}
TeaAMFMessageBody* TeaAMFMessageBody::messageBodyWithTarget(const char* target,unsigned int response,TeaASObject* data) {
	char	buf[16];
	sprintf(buf, "/%d",response);
	return messageBodyWithTarget(target, buf, data);
}

void TeaAMFMessageBody::setResponse(unsigned int response) {
	char	buf[16];
	sprintf(buf, "/%d",response);
	TEA_COPY_STRING(m_pResponse, buf);
}

#pragma -
#pragma MessageHeader

TeaAMFMessageHeader::TeaAMFMessageHeader() :
m_pName(NULL),
m_bMustUnderstand(false),
m_pData(NULL) {
		
}
TeaAMFMessageHeader::~TeaAMFMessageHeader() {
	CC_SAFE_RELEASE(m_pData);
	//CC_SAFE_DELETE(m_pData);
	CC_SAFE_DELETE(m_pName);
}
TeaAMFMessageHeader*	TeaAMFMessageHeader::messageHeaderWithName(const char* name,TeaASObject *data,bool mustUnderStand) {
	TeaAMFMessageHeader *header = new TeaAMFMessageHeader();
	header->autorelease();
	
	TEA_COPY_STRING(header->m_pName,name);
	header->m_bMustUnderstand = mustUnderStand;
	header->m_pData = data;
	header->m_pData->retain();
	
	return header;
}

#pragma -
#pragma Message

TeaAMFMessage::TeaAMFMessage() {
	m_pHeaders = CCArray::createWithCapacity(4);
	m_pHeaders->retain();
	m_pBodies = CCArray::createWithCapacity(1);
	m_pBodies->retain();
	m_sVersion = 0x3;//
}
TeaAMFMessage::~TeaAMFMessage() {
	CC_SAFE_RELEASE(m_pHeaders);
	CC_SAFE_RELEASE(m_pBodies);
}
TeaAMFMessageHeader* TeaAMFMessage::operator()(unsigned int index) {
	return (TeaAMFMessageHeader*)m_pHeaders->objectAtIndex(index);
}

TeaAMFMessageBody*	TeaAMFMessage::operator[](unsigned int index) {
	return (TeaAMFMessageBody*)m_pBodies->objectAtIndex(index);
}

TeaASObject* TeaAMFMessage::getHeaderDataByName(std::string name)
{
    int count = m_pHeaders->count();
    for(int i=0; i<count; i++)
    {
        TeaAMFMessageHeader* tmpHeader = (TeaAMFMessageHeader*)m_pHeaders->objectAtIndex(i);
        std::string tmpStr(tmpHeader->m_pName);
        if(tmpStr == name)
        {
            return tmpHeader->m_pData;
        }
    }
    return NULL;
}
