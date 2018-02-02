//
//  TeaSocket.cpp
//  tea
//
//  Created by yaowei li on 11-8-25.
//  Copyright 2011年 touchage. All rights reserved.
//

#include "TeaSocket.h"
#include "ctype.h"
#include "GameData.h"
#include "DialogManager.h"
using namespace cocos2d;
using namespace tea;


TeaClientSocket::TeaClientSocket(const char* pHostname,const char* pService) {
	m_pService = m_pHostName = NULL;
	m_pBuffer = NULL;
	m_pTimeout.tv_sec = m_pTimeout.tv_usec = 0;
    m_iSock = -1;
	TEA_COPY_STRING(m_pHostName, pHostname);
	TEA_COPY_STRING(m_pService, pService);
}
TeaClientSocket::~TeaClientSocket() {
	CC_SAFE_FREE(m_pHostName);
	CC_SAFE_FREE(m_pService);
	CC_SAFE_RELEASE(m_pBuffer);
    if ( m_iSock >= 0 ) close(m_iSock);
}

void TeaClientSocket::setTimeout(unsigned int uSec, unsigned int uUsec)
{
	m_pTimeout.tv_sec = uSec;
	m_pTimeout.tv_usec = uUsec;
}

int TeaClientSocket::connect() {
	struct addrinfo hints,*pAddrinfo;
	memset(&hints,0,sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_INET;
	
    if(getaddrinfo(m_pHostName, m_pService, &hints, &pAddrinfo) != 0){
        timeoutHandler();
        return -1;
    }
//	assert(getaddrinfo(m_pHostName, m_pService, &hints, &pAddrinfo) == 0 );
	m_iSock = socket(pAddrinfo->ai_family,pAddrinfo->ai_socktype,pAddrinfo->ai_protocol);
	assert(m_iSock >= 0);
	// timeout 
	if ( m_pTimeout.tv_sec > 0 ) {
		setsockopt(m_iSock, SOL_SOCKET, SO_RCVTIMEO, (char*)&m_pTimeout, sizeof(timeval));
		setsockopt(m_iSock, SOL_SOCKET, SO_SNDTIMEO, (char*)&m_pTimeout, sizeof(timeval));
	}
	if ( ::connect(m_iSock, pAddrinfo->ai_addr, pAddrinfo->ai_addrlen) != 0 ) {
		TEA_PERROR("Connect error");
		close(m_iSock);
        m_iSock = -1;
		return -1;
	}
	freeaddrinfo(pAddrinfo);
	return m_iSock;
}
void TeaClientSocket::timeoutHandler() {
    
    DialogManager::getInstance()->showNetWorkError();
    
	//GameData::instance()->createFailedLayerDialog();
    //GameData::instance()->createRetryConnectionDialog();
    
    
//    GameData::getInstance()->showRetryDialog = true;
    
}
int TeaClientSocket::process() {
	int retn = connect();
    CCLOG("retn is %d",retn);
	if ( retn < 0 ) return retn;
	if ( init() ) {
		close(m_iSock);
        m_iSock = -1;
		return -1;
	}	
	if ( send() ) {
		close(m_iSock);
        m_iSock = -1;
		return -1;
	}
	
	size_t	total = 0;
	while ( true ) {
		char buffer[1024];
		
		int rsize = ::recv(m_iSock, buffer, 1024, 0);
//        std::string s = std::string(buffer);
		if ( rsize == -1 ) {
			if ( errno == EAGAIN ) {
				timeoutHandler();
                CCLOG("TeaSocket:timeout");
                //这里出错了必须返回－1，否则返回给amf3解析器的数据是不完整的，直接导致崩溃
                total = -1;  
//                if ( finish(total) == -1 )
//                    total = -1;   
                
            }
			else {
				ioExceptionHandler(errno);
                total = -1;
            }
            break;
		}
		if ( rsize == 0 )	//finish
		{
			if ( finish(total) == -1 )
				total = -1;
			break;
		}
//        CCLOG("------------TeaSocket %s:: %d",m_pHostName,total);
        int recvRetn = recv(buffer,rsize);
		if ( recvRetn == -1 )
			return -1;
		total += rsize;
        if ( recvRetn == 0 ) break;
	}
	return total;
}

/**
 * TeaHttpClient
 **/
int TeaHttpClient::send() {
	char header[1024];
	if ( memcmp(m_pMethod,TEA_HTTP_POST_METHOD,strlen(TEA_HTTP_POST_METHOD)) == 0 ) {
		if ( m_pPostData != NULL && m_pPostData->size() > 0 ) {
            if ( m_pProxyHostName == NULL )
                sprintf(header, "%s /%s HTTP/1.1\r\nHost: %s:%s\r\nUser-Agent: %s\r\nContent-Type: %s\r\nContent-Length: %d\r\nConnection: %s\r\nAccept: */*\r\n\r\n"
                        ,m_pMethod,m_pPage,m_pHostName,m_pService,m_pUserAgent,m_pContentType,m_pPostData->size(),m_pConnectType);
            else
                sprintf(header, "%s http://%s:%s/%s HTTP/1.1\r\nHost: %s:%s\r\nUser-Agent: %s\r\nContent-Type: %s\r\nContent-Length: %d\r\nConnection: %s\r\nAccept: */*\r\n\r\n"
                        ,m_pMethod,m_pHostName,m_pService,m_pPage,m_pHostName,m_pService,m_pUserAgent,m_pContentType,m_pPostData->size(),m_pConnectType);
			if ( _send(header,strlen(header)) < 0 ) 
				return -1;
			if ( _send(m_pPostData->getBuffer(),m_pPostData->size()) < 0 ) return -1;
			return 0;
		}
	}
    if ( m_pProxyHostName == NULL ) 
        sprintf(header, "%s /%s HTTP/1.0\r\nHost: %s:%s\r\nUser-Agent: %s\r\nContent-Type: %s\r\nConnection: %s\r\nAccept: */*\r\n\r\n"
                ,m_pMethod,m_pPage,m_pHostName,m_pService,m_pUserAgent,m_pContentType,m_pConnectType);
    else
        sprintf(header, "%s http://%s:%s/%s HTTP/1.1\r\nHost: %s:%s\r\nUser-Agent: %s\r\nContent-Type: %s\r\nConnection: %s\r\nAccept: */*\r\n\r\n"
                ,m_pMethod,m_pHostName,m_pService,m_pPage,m_pHostName,m_pService,m_pUserAgent,m_pContentType,m_pConnectType);
	if ( _send(header,strlen(header)) < 0 ) 
		return -1;
	return 0;
}
TeaHttpClient::TeaHttpClient(const char* pHostName,const char* pService) : TeaClientSocket(pHostName,pService) {
	m_pMethod = TEA_HTTP_GET_METHOD;
	m_pUserAgent = TEA_HTTP_USER_AGENT;
	m_pContentType = TEA_HTTP_TEXT_CONTENT_TYPE;
    m_pConnectType = TEA_HTTP_CONNECT_CLOSE;
    
	m_pPage = NULL;
	m_pPostData = NULL;
	
	m_pProxyHostName = m_pProxyService = NULL;
	m_iHttpCode = -1;
}
TeaHttpClient::~TeaHttpClient() {
	CC_SAFE_FREE(m_pPage);
	CC_SAFE_RELEASE(m_pPostData);
	CC_SAFE_FREE(m_pProxyHostName);
	CC_SAFE_FREE(m_pProxyService);
}

int TeaHttpClient::connect() {
	if ( m_pProxyHostName == NULL ) return TeaClientSocket::connect();
	
	struct addrinfo hints,*pAddrinfo;
	memset(&hints,0,sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_INET;
	
	//assert(getaddrinfo(m_pProxyHostName, m_pProxyService, &hints, &pAddrinfo) == 0 );
	if(getaddrinfo(m_pProxyHostName, m_pProxyService, &hints, &pAddrinfo) != 0 ){
		return -1;
	}
	m_iSock = socket(pAddrinfo->ai_family,pAddrinfo->ai_socktype,pAddrinfo->ai_protocol);
	assert(m_iSock >= 0);
	// timeout 
	if ( m_pTimeout.tv_sec > 0 ) {
		setsockopt(m_iSock, SOL_SOCKET, SO_RCVTIMEO, (char*)&m_pTimeout, sizeof(timeval));
		setsockopt(m_iSock, SOL_SOCKET, SO_SNDTIMEO, (char*)&m_pTimeout, sizeof(timeval));
	}
	if ( ::connect(m_iSock, pAddrinfo->ai_addr, pAddrinfo->ai_addrlen) != 0 ) {
		TEA_PERROR("Connect error");
		close(m_iSock);
        m_iSock = -1;
		return -1;
	}
	freeaddrinfo(pAddrinfo);
	return m_iSock;
}

int TeaHttpClient::finish(size_t total) {
    //	char* firstLine = m_pBuffer->getLine();
    //    
    //	if ( firstLine == NULL || strlen(firstLine) == 0 ) return -1;
    //	while ( *firstLine == ' ' || *firstLine == '\t' ) firstLine++;
    //	while ( *firstLine != ' ' && *firstLine != '\t' ) firstLine++;
    //	while ( *firstLine == ' ' || *firstLine == '\t' ) firstLine++;
    //	char *end = firstLine;
    //	while ( *end != ' ' && *end != '\t' ) end++;
    //	*end = 0;
    //	m_iHttpCode = atoi(firstLine);
    //	if ( m_iHttpCode == 200 ) {
    //		do {
    //			firstLine = m_pBuffer->getLine();
    //		} while ( strlen(firstLine) > 0 );
    //		return 0;
    //	}
    //	return -1;
    return 0;
}

int TeaHttpClient::recv(const char* pBuffer,const unsigned int uLen) {
    //    if ( m_pBuffer == NULL ) 
    //        m_pBuffer = new TeaBuffer();
    //    
    //    (*m_pBuffer)(uLen)>>pBuffer;
    //    return uLen;
    TeaClientSocket::recv(pBuffer,uLen);
    
    char* line = NULL;
    if ( m_iContentType == TEA_HTTP_RECV_START ) {
        line = m_pBuffer->getLine();
        if ( line == NULL || strlen(line) == 0 ) return uLen;
        while ( *line == ' ' || *line == '\t' ) line++;
        while ( *line != ' ' && *line != '\t' ) line++;
        while ( *line == ' ' || *line == '\t' ) line++;
        char *end = line;
        while ( *end != ' ' && *end != '\t' ) end++;
        *end = 0;
        m_iHttpCode = atoi(line);
        if ( m_iHttpCode != 200 ) {
            return -1;
        }
        m_iContentType = TEA_HTTP_RECV_HEADER;
    }
    if ( m_iContentType == TEA_HTTP_RECV_HEADER ) {
        while ( ( line = m_pBuffer->getLine() ) != NULL ) {
            if ( strlen(line) == 0 ) {
                m_iContentType = TEA_HTTP_RECV_CONTENT;
                break;
            }
            if ( strlen(line) > 14 ) {
                for ( int i=0;i<14;++i )
                    line[i] = toupper(line[i]);
                if ( memcmp(line,"CONTENT-LENGTH",14) == 0 ) {
                    m_uContentLen = atoi(line+15);
                    CCLOG("http Content length: %d",m_uContentLen);
                }
            }
        }
    }
    if ( m_iContentType == TEA_HTTP_RECV_CONTENT ) {
        if ( m_uContentLen <= m_pBuffer->size() ) return 0;
    }
    return uLen;
}

TeaHttpClient* TeaHttpClient::httpClientWithUrl(const char* url) {
	assert(url != NULL && memcmp(url,"http://",7) == 0);
	char *purl=NULL;
	TEA_COPY_STRING(purl, (url+7));
	
	char* p1 = strstr(purl,":");
	char* p2 = strstr(purl,"/");
	if ( p1 == NULL ) p1 = (char*)TEA_HTTP_DEFAULT_SERVICE;
	else {
		p1[0] = 0;
		p1++;
	}
	p2[0] = 0;
	p2++;
	TeaHttpClient *client = new TeaHttpClient(purl,p1);
	client->setPage(p2);
	
	free(purl);
	
	return client;
	
}

TeaNoblockHttpClient::TeaNoblockHttpClient(const char* pHostName,const char* pService) : TeaHttpClient(pHostName,pService) {
    m_pConnectType = TEA_HTTP_CONNECT_KEEP_ALIVE;
}

int TeaNoblockHttpClient::sendPage(const char* pPage) {
    if ( m_iSock == -1 ) {
        if ( connect() == -1 ) return -1;
        if ( init() == -1) return -1;
    }
    errno = 0;
    char buffer[1024];
    int retn;
    

    while ( (retn = ::recv(m_iSock,buffer,1024,0) ) > 0 ) {
        
    }
    CCLOG("%d %d\n",retn,errno);
    if ( retn == -1 && errno != EAGAIN ) {
        if ( m_iSock != -1 ) close(m_iSock);
        connect();
        init();
    }
    setPage(pPage);    

    retn = send();
    if ( retn == -1 ) {
        if ( m_iSock != -1 ) close(m_iSock);
        connect();
        init();
        retn = send();
    }
    return retn;
}

