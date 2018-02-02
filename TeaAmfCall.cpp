//
//  TeaAmfCall.cpp
//  tea
//
//  Created by yaowei li on 11-7-17.
//  Copyright 2011年 touchage. All rights reserved.
//

#include "TeaAmfCall.h"
#include "TeaSocket.h"
#include "GameDefine.h"
#include "cocos2d.h"
#include "cocos-ext.h"
#include "GameUtil.h"
#include "EvilDebugDefine.h"
#include "GameData.h"
using namespace tea;
using namespace cocos2d;





static unsigned int total_response = 1;


TeaAMFCall::TeaAMFCall(const char* target,const char *gateway_url) :
TeaResource(gateway_url),
failureFunc(NULL),
successFunc(NULL),
//m_uRetry_Count(0),
m_pResponse(NULL),
m_isCached(true),
curAmfIndex(0)
{
	m_uRetry_Count = 0;
	m_pRequest = new TeaAMFMessage();
    
    
}
TeaAMFCall::~TeaAMFCall() {
//	CCLOG("TeaAMFCALL release");
	CC_SAFE_RELEASE(m_pRequest);
	if(m_pResponse!=NULL)
		CC_SAFE_RELEASE(m_pResponse);
}

void TeaAMFCall::addHeader(const char* name,const char* value,bool mustUnderstand) {
	m_pRequest->m_pHeaders->addObject(TeaAMFMessageHeader::messageHeaderWithName(name,TeaASObject::objectWithString(value),mustUnderstand));
}

void TeaAMFCall::addBody(const char* target, TeaASObject* args){
	// 暂时只支持一个body
	m_pRequest->m_pBodies->addObject(TeaAMFMessageBody::messageBodyWithTarget(target,total_response++,args));
	TeaAMFMessageBody	*body = (*m_pRequest)[0];
	TEA_COPY_STRING(body->m_pTarget, target);
}

static size_t recvData(char *ptr,size_t size,size_t nmemb,void* userdata) {
	TeaAMFCodec* decodec = (TeaAMFCodec*)userdata;
//    decodec->inputPChar((unsigned char*)ptr, size*nmemb);
	(*decodec)(size*nmemb)>>ptr;
	CCLOG("recvData---%d",size*nmemb);
	return size*nmemb;
}

bool TeaAMFCall::stream() {
	//std::cout<<"start resquest"<<std::endl;
//	CCLOG("start request url is %s and target is %s",getUrl().c_str(),getAMFTarget());
	TeaAMFCodec* encodec = new TeaAMFCodec();
	TeaAMFCodec* decodec = new TeaAMFCodec();
	encodec->autorelease();
	decodec->autorelease();
	
	encodec->inputMessage(m_pRequest);
	
	TeaHttpClient*	client = TeaHttpClient::httpClientWithUrl(getUrl().c_str());
	
	client->setPostData(encodec);
	client->setTimeout(GameData::getInstance()->AMFCALL_TIMEOUT);
    
    btea(encodec->getBuffer(),encodec->size(),1);
    
	client->setBuffer(decodec);
	client->setContentType(TEA_HTTP_AMF_CONTENT_TYPE);
	
#ifdef USE_CHARLES_BOBO
    client->setProxy(CHARLES_URL,CHARLES_PORT);
//    client->setProxy("192.168.2.86","8888");
#endif
    int process = client->process();
    
	if ( process <= 0 ) 
	{
		CCLOG("request error");
		delete client;
		return false;
	}
    
//    int _size = client->m_uContentLen;
	delete client;
    
	//判断是否是amf格式的返回结果
	unsigned short checkVersion = 99;
    
    int resSize = decodec->size();
    
    btea(decodec->getBuffer(),decodec->size(),0);
    
//    CCLOG("decodec size is  -------------------------------%d---%d",resSize,process);
    if(resSize<=0){
        CCLOG("error response size is 0");
        return false;
    }
    
	decodec->outputForTest(checkVersion,true);
	if(checkVersion!=3){
		if(decodec->size()<20000){
			CCLOG("error response is no amf formate %s and size is %d,check version %d",decodec->getBuffer(),decodec->size(),checkVersion);
		}else{
			CCLOG("error response is no amf formate size is %d,check version %d",decodec->size(),checkVersion);
		}
		
		return false;
	}
    
    //缓存到本地
//    if(m_isCached)
//    {
        //TeaAMFCodec* tmp =(TeaAMFCodec*) client->getBuffer();
//        char* cacheData = decodec->getBuffer();
        
//    }
    
	// decode
	m_pResponse = decodec->outputMessage();
	// 持有结果
	m_pResponse->retain();
	
	CCLOG("receive response");
	return true;
	
}

