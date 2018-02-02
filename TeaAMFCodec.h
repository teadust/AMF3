//
//  TeaAMFCodec.h
//  tea
//
//  Created by yaowei li on 11-7-14.
//  Copyright 2011年 touchage. All rights reserved.
//
#ifndef __TEA_AMF_CODEC_H__
#define __TEA_AMF_CODEC_H__

#include "cocos2d.h"
#include "TeaBuffer.h"
#include "TeaASObject.h"
#include "TeaAMFMessage.h"


#define TEA_MAX_STRING_LENGTH		2048
namespace tea {
	
	class TeaAMFCodec : public TeaBuffer {
	private:
		cocos2d::CCArray	*m_aObjectRefTab;
		cocos2d::CCArray	*m_aStringRefTab;
		cocos2d::CCArray    *m_aTraitRefTab;
	public:
		TeaAMFCodec();
		~TeaAMFCodec();
		/**
		 *	output TeaAMFMessage
		 **/
		TeaAMFMessage* outputMessage();
		
		/**
		 * input TeaAMFMessage
		 **/
		void inputMessage(TeaAMFMessage* message);
	private:
		unsigned int getVarInt();
		TeaASObject* getASObject();
		TeaASObject* outputStringObject();
		TeaASObject* outputDateObject();
		TeaASObject* outputArrayObject();
		TeaASObject* outputObject();
		TeaASObject* outputByteArrayObject();
		void outputUTFString(char* str,unsigned short len=0);
		
		void inputUTFString(const char* str,bool isObject=false);
		void inputVarInt(unsigned int value);
        void inputAMFInt(int i);
		void inputStringObject(TeaASObject* object);
		void inputASObject(TeaASObject *object);
        
        
        TeaASObject* readTraits(int ref);
	};
}


#endif
