//
//  TeaAmfCall.h
//  tea
//
//  Created by yaowei li on 11-7-17.
//  Copyright 2011年 touchage. All rights reserved.
//
#ifndef __TEA_AMF_CALL_H__
#define __TEA_AMF_CALL_H__

#include "cocos2d.h"
//#include "ResourceDescriptor.h"
#include "TeaAMFMessage.h"
#include "TeaAMFCodec.h"
#include "tea.h"
#include "TeaResource.h"

using namespace cocos2d;

#define RECV_BUFFER_SIZE	1024
namespace tea {
	class TeaAMFCall : public TeaResource {
	public://TODO BOBO
		TeaAMFMessage *m_pRequest;
		TeaAMFMessage *m_pResponse;
        //是否缓存配置信息到本地sd卡，debug时默认为true
		bool m_isCached;
	public:
        
		void *failureFunc;
		void *successFunc;
		
		TeaAMFCall(const char* target,const char *gatewayUrl);
		~TeaAMFCall();
		
		void addHeader(const char* name,const char* value,bool mustUnderstand = false);
		void addBody(const char* target, TeaASObject* args);
		
		void setTarget(const char* target);
		
		char* getAMFTarget(){
			return (*m_pRequest)[0]->m_pTarget;
		}
		int curAmfIndex;
		/**
		 * param
		 **/
		template <typename T>
		TeaAMFCall& operator()(T& value,DataType type=DT_UNDEFINED) {
			(*m_pRequest)[0]->m_pData->addChild(TeaASObject::objectWithValue(value,type));
			return *this;
		}
		/**
		 * string or bytearray
		 **/
		TeaAMFCall& operator()(const char* value,size_t len=-1) {
			(*m_pRequest)[0]->m_pData->addChild(TeaASObject::objectWithString(value,len));
			return *this;
		}
		virtual bool stream();
		
		//static void setGatewayUrl(const char *url);
		
		inline TeaAMFMessage* getResponseMessage(){
			return m_pResponse;
		}
		
		inline TeaASObject* getResponseData(){
			return (*m_pResponse)[0]->m_pData;
		}
	};
}



#endif
