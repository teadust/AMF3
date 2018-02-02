//
//  TeaSocket.h
//  tea
//
//  Created by yaowei li on 11-8-25.
//  Copyright 2011年 touchage. All rights reserved.
//
#ifndef __TEA_SOCKET_H__
#define __TEA_SOCKET_H__
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>

#include "cocos2d.h"
#include "tea.h"

#include "TeaBuffer.h"

namespace tea {
	class TeaClientSocket {
	protected:
        int m_iSock;
		char *m_pHostName;
		char *m_pService;
		
		timeval		m_pTimeout;
		TeaBuffer	*m_pBuffer;
		
		virtual int connect();
		virtual int init() {
			return 0;
		}
		
		int _send(const char* buffer,size_t len) {
            if ( m_iSock == -1 ) return -1;
			int sent = 0 ;
			while ( sent < len ) {
                //signal(SIGPIPE,SIG_IGN);
				int ret = ::send(m_iSock,buffer+sent,len-sent,0);
				if ( ret == -1 ) {
					ioExceptionHandler(errno);
					close(m_iSock);
                    m_iSock = -1;
					return -1;
				}
				sent += ret;
			}
			return sent;
		}
		virtual int send() {
			return 0;
		}
		
		virtual int recv(const char* pBuffer,const unsigned int uLen) {
			if ( m_pBuffer == NULL ){
                
				m_pBuffer = new TeaBuffer();
            }
			
//            m_pBuffer->inputPChar((unsigned char*)pBuffer,uLen);
			(*m_pBuffer)(uLen)>>pBuffer;
			return uLen;
		}
		
		virtual int finish(size_t total) {
			return 0;
		}
		
		virtual void timeoutHandler();
		
		virtual void ioExceptionHandler(int err) {}
        
	public:
		TeaClientSocket(const char* pHostname,const char* pService);
		virtual ~TeaClientSocket();
		void setTimeout(unsigned int uSec,unsigned int uUsec=0);
		TeaBuffer* getBuffer() {
			return m_pBuffer;
		}
		void setBuffer(TeaBuffer* pBuffer) {
			CC_SAFE_RELEASE(m_pBuffer);
			m_pBuffer = pBuffer;
			CC_SAFE_RETAIN(m_pBuffer);
		}
		int process();
	};
#define TEA_HTTP_GET_METHOD		"GET"
#define TEA_HTTP_POST_METHOD	"POST"
#define TEA_HTTP_USER_AGENT		"TeaNet"
#define TEA_HTTP_TEXT_CONTENT_TYPE	"text/plain"
#define TEA_HTTP_AMF_CONTENT_TYPE	"application/x-amf"

#define TEA_HTTP_CONNECT_CLOSE  "close"
#define TEA_HTTP_CONNECT_KEEP_ALIVE "keep-alive"
    
#define TEA_HTTP_DEFAULT_SERVICE	"80"
    
#define TEA_HTTP_RECV_START     0x00
#define TEA_HTTP_RECV_HEADER    0x01
#define TEA_HTTP_RECV_CONTENT   0x02
    
	class TeaHttpClient : public TeaClientSocket {
	private:
		int m_iContentType;
	protected:
		CC_SYNTHESIZE(const char*,m_pMethod,Method);
		CC_SYNTHESIZE(const char*,m_pUserAgent,UserAgent);
		CC_SYNTHESIZE(const char*,m_pContentType,ContentType);
        CC_SYNTHESIZE(const char*,m_pConnectType,ConnectType);
        
		char *m_pPage;
		TeaBuffer *m_pPostData;
		int		m_iHttpCode;
		uint    m_uContentLen;
        
		char* m_pProxyHostName;
		char* m_pProxyService;
		
		virtual int connect();
		virtual int finish(size_t total);
		virtual int send();
		virtual int init() {
            m_iContentType = TEA_HTTP_RECV_START;
            m_uContentLen = 0;
			return 0;
		}
	public:
		TeaHttpClient(const char* pHostName,const char* pService=TEA_HTTP_DEFAULT_SERVICE);
		virtual ~TeaHttpClient();
		inline void setPage(const char* pPage) {
			TEA_COPY_STRING(m_pPage,pPage);
		}
		
		void setProxy(const char* pProxyHostName,const char* uProxyPort) {
			TEA_COPY_STRING(m_pProxyHostName, pProxyHostName);
			TEA_COPY_STRING(m_pProxyService, uProxyPort);
		}
		void setPostData(TeaBuffer* pPostData) {
			if ( m_pPostData ) m_pPostData->release();
			m_pPostData = pPostData;
			if ( m_pPostData ) {
				m_pPostData->retain();
				m_pMethod = TEA_HTTP_POST_METHOD;
			}
		}
		virtual int recv(const char* pBuffer,const unsigned int uLen);
		static TeaHttpClient* httpClientWithUrl(const char* url) ;
	};
	
    class TeaNoblockHttpClient : public TeaHttpClient {
    public:
        TeaNoblockHttpClient(const char* pHostName,const char* pService=TEA_HTTP_DEFAULT_SERVICE);
        virtual ~TeaNoblockHttpClient(){};
        virtual int process() {
            return 0;
        }
        
        virtual int init() {
//            int on = 1;
            int fl = fcntl(m_iSock, F_GETFL,0);
            assert(fl != -1);
            assert(fcntl(m_iSock,F_SETFL,fl|O_NONBLOCK) != -1);
			
//#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
//            setsockopt(m_iSock, SOL_SOCKET, SO_NOSIGPIPE, (void*)&on, sizeof(on));
//#endif
//#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
//			/* 忽略终端 I/O信号,STOP信号 */
//			signal(SIGTTOU,SIG_IGN);
//			signal(SIGTTIN,SIG_IGN);
//			signal(SIGTSTP,SIG_IGN); 
//			signal(SIGHUP ,SIG_IGN);
//			signal(SIGPIPE,SIG_IGN);
//#endif
			return 0;
		}

        virtual int sendPage(const char* pPage);
    };
}


#endif
