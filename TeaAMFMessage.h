//
//  TeaAMFMessage.h
//  tea
//
//  Created by yaowei li on 11-7-14.
//  Copyright 2011年 touchage. All rights reserved.
//
//	注： AMF 使用了 CCObject 的内存管理方式。
//		而 CCObject 的 内存管理方式 不是线程安全的。
//		请确认使用AMF 在同一个线程内。

#ifndef __TEA_AMF_MESSAGE_H__
#define __TEA_AMF_MESSAGE_H__

#include "cocos2d.h"
#include "TeaASObject.h"
#include "tea.h"

namespace tea {
	class TeaAMFMessageBody : public CCObject {
	protected:
		TeaAMFMessageBody();
	public:
		char	*m_pTarget;
		char	*m_pResponse;
		TeaASObject	*m_pData;
		
		~TeaAMFMessageBody();
		
		void setResponse(unsigned int response);
		static TeaAMFMessageBody* messageBodyWithTarget(const char* target,const char* response,TeaASObject* data);
		static TeaAMFMessageBody* messageBodyWithTarget(const char* target,unsigned int response,TeaASObject* data);
		
	};
	class TeaAMFMessageHeader : public CCObject {
	protected:
		TeaAMFMessageHeader();
	public:
		char	*m_pName;
		bool	m_bMustUnderstand;
		TeaASObject	*m_pData;
		
		~TeaAMFMessageHeader();
		static TeaAMFMessageHeader*	messageHeaderWithName(const char* name,TeaASObject *data,bool mustUnderStand=false);
	};
	
	class TeaAMFMessage : public CCObject {
	public:
		unsigned short m_sVersion;
		
		CCArray	*m_pHeaders;
		CCArray *m_pBodies;
		
		TeaAMFMessage();
		~TeaAMFMessage() ;
		virtual TeaAMFMessageHeader* operator()(unsigned int index);
		virtual TeaAMFMessageBody*	operator[](unsigned int index);
//		
//		// test 
//		virtual std::string toString() {
//			return (*this)[0]->m_pData->toString();
//		}
        
        TeaASObject* getHeaderDataByName(std::string name);
	};
}



#endif