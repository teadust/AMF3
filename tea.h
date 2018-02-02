/*
 *  tea.h
 *  tea
 *
 *  Created by teadust on 11-6-3.
 *
 */
#ifndef __TEA_H__

#define __TEA_H__
#include <stdio.h>
#include <string.h>
#include <vector>
#include <errno.h>
#include <assert.h>
#include <sys/stat.h>
#include <list>
#include <iostream>
#include <algorithm>
#include <pthread.h>

//#include "cocos2d.h"

// Tea lib
//#include "TeaBuffer.h"
//#include "TeaResource.h"

#define TEA_DEBUG

#define TEA_PERROR(msg)	\
perror(msg);

#ifndef SAFE_DELETE
#define SAFE_DELETE(ptr)		{if (ptr) delete ptr;ptr=NULL;}
#endif

#ifndef MAX_NUMBER
#define MAX_NUMBER(a,b)	((a)>(b))?(a):(b)
#endif
#ifndef MIN_NUMBER
#define MIN_NUMBER(a,b)	((a)>(b))?(b):(a)
#endif

namespace tea {
	class TeaBuffer;
	template <typename T>
	void safe_malloc(T*& instance,int num) {
		if (instance)
			delete instance;
		instance = (T*)malloc(sizeof(T)*num);
		memset((void*)instance,0,sizeof(T)*num);
	}
    
	template <typename T>
	T& reserve(T& t) {
		if ( sizeof(T) <= 1 ) return t;
		size_t s = sizeof(T);
		unsigned char tmp;
		unsigned char* tp = (unsigned char*)&t;
		for (int i=0; i<s/2; ++i) {
			tmp = tp[s-i-1];
			tp[s-i-1] = tp[i];
			tp[i] = tmp;
		}
		return t;
		
	}
	extern int	powOf2(int t);
	extern TeaBuffer* recvHttpData(const char* host,const char* page,TeaBuffer* buffer=NULL,const char* service="80",unsigned int timeout=0);
}
#define SET_ZERO(t)	memset(&t,0,sizeof(t))
#define TEA_COPY_STRING(dst,src)	{	SAFE_DELETE(dst); dst=(char*)malloc(strlen(src)+1);memcpy(dst,src,strlen(src)+1); }

#endif
