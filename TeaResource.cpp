//
//  TeaResource.cpp
//  tea
//
//  Created by yaowei li on 11-7-18.
//  Copyright 2011年 touchage. All rights reserved.
//

#include "TeaResource.h"
#include "tea.h"
#include "cppUtils.h"
#include "TeaSocket.h"
#include "GameDefine.h"
#include "GameData.h"
#include "EvilDebugDefine.h"
#include "TeaMd5.h"
using namespace tea;
using namespace cocos2d;


TeaResource::TeaResource(const char* url,long liveTime)
{
    showNetWork = false;
    isNeed = false;
    
	m_uRetry_Count = 0;
	m_uMax_Retry_Count = 0;
	m_eStatus = TEA_RESOURCE_STATUS_WAITING;
	m_pUrl = std::string(url);
	realURL = std::string(url);
    
    callBackArray = CCArray::create();
    callBackArray->retain();
    
    m_pBuffer = new TeaBuffer();
//    static int k =0;
//    k++;
//    testId=k;
}
TeaResource::~TeaResource() {
//    CCLog("TeaResource dealloc ---%s,%d,%d",realURL.c_str(),m_pBuffer->testId,testId);
    CC_SAFE_RELEASE_NULL(callBackArray);
    CC_SAFE_RELEASE_NULL(m_pBuffer);
}

//bool TeaResource::stream() {
//	if (realURL.empty()) {
//		EVILLOG("realURL is null just return");
//		return false;
//	}
//	bool result = true;
//	if ( memcmp(realURL.c_str(),"http://",7) == 0 ) {
//
//        std::string _requestURL = realURL;
//        
//		TeaHttpClient* client = TeaHttpClient::httpClientWithUrl(_requestURL.c_str());
//        
//        
//#ifdef USE_CHARLES_BOBO
//        client->setProxy(CHARLES_URL,CHARLES_PORT);
//#endif
//        
//		client->setTimeout(90);//
//		client->setBuffer(m_pBuffer);
//		if ( client->process() > 0 ) 
//		{
//			m_eStatus = TEA_RESOURCE_STATUS_LOADED;
//            delete client;
//			return true;
//		} else {
//            delete client;
//			m_eStatus = TEA_RESOURCE_STATUS_FAILED;
//			return false;
//		}
//	}
//	else
//		result = readLocalResource();
//	return result;
//}

//bool TeaResource::readLocalResource() {
//    //bobo 2013
//    
//    m_eStatus = TEA_RESOURCE_STATUS_LOADING;
//
//    
////	EVILLOG("###...realURL...%s",realURL.c_str());
//    
//    unsigned long nSize = 0;
//    unsigned char * pBuffer = CCFileUtils::sharedFileUtils()->getFileData(realURL.c_str(), "rb", &nSize);
//    if ( NULL == pBuffer || 0 == nSize)
//	{
//		EVILLOG("TeaResource::pbuffer is null or nSize is 0  %s",m_pUrl.c_str());
//		//m_eStatus = TEA_RESOURCE_STATUS_FAILED;
////        string command = "rm -r \"" + this->realURL+"\"";
////        system(command.c_str());
//        
//        this->realURL = GameData::getInstance()->teaResourceManager->decideWhichUrl(this->getUrl(),true);
//        
//        m_pBuffer->clear();
//		return false;
//	}
//    
////    m_pBuffer->inputPChar((unsigned char*)pBuffer, nSize);
//    
//    (*m_pBuffer)(nSize)>>pBuffer;
//    
//    
//    
//    EVILLOG("read from local");
//    
//#ifndef RESOURCE_ALWARY_IN_BOUNDLE
//    //本地文件md5 校验
//    std::string md5Value = stringFromMD5((unsigned char*)this->m_pBuffer->getBuffer(), this->m_pBuffer->size());
//    if(!ResourceStreamManager::getInstance()->checkMd5(this->getUrl(), md5Value)){
//        //本地md5错误
//        
////        string command = "rm -r \"" + this->realURL+"\"";
////        system(command.c_str());
//        
//        this->realURL = GameData::getInstance()->teaResourceManager->decideWhichUrl(this->getUrl(),true);
//        
//        m_pBuffer->clear();
//        
//        CC_SAFE_DELETE_ARRAY(pBuffer);
//        
//        return false;
//    }
//    
//    
//    EVILLOG("%s",md5Value.c_str());
//#endif
//    
//    m_eStatus = TEA_RESOURCE_STATUS_LOADED;
//    
//    
//    CC_SAFE_DELETE_ARRAY(pBuffer);
//
//	
//	return true;
//	
//}

const char* TeaResource::getUserDataBasePath()
{
    return GameData::getInstance()->writeAblePath.c_str();
}

static size_t recvData(char *ptr,size_t size,size_t nmemb,void* userdata) {
	TeaBuffer* buffer = (TeaBuffer*)userdata;
    
//    buffer->inputPChar((unsigned char*)ptr, size*nmemb);
    
	(*buffer)(size*nmemb)>>ptr;
	return size*nmemb;
}

//bool TeaResource::readHttpResource() {
//    return false;
//}

bool TeaResource::isLoaded() {
    return (m_eStatus & 0xff) == TEA_RESOURCE_STATUS_LOADED || 
	(m_eStatus & 0xff) == TEA_RESOURCE_STATUS_READY ;
}
bool TeaResource::isFailed() {
    return (m_eStatus & 0xff) == TEA_RESOURCE_STATUS_FAILED;  
}
bool TeaResource::isPending() {
    return (m_eStatus & 0xff) == TEA_RESOURCE_STATUS_LOADING;  
}
