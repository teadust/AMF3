/*
 *  tea.cpp
 *  tea
 *
 *  Created by teadust on 11-6-5.
 *
 */

#include "tea.h"

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "TeaBuffer.h"

namespace tea {
	int	powOf2(int t) {
		if (t == 0) return 0;
		t--;
		unsigned char bit=0;
		do {
			bit++;
			t >>= 1;
		} while ( t );
		return (1<<bit);
	}
	
	TeaBuffer* recvHttpData(const char* host,const char* page,TeaBuffer* buffer,const char* service,unsigned int timeout) {
		// create socket
		int err;
		struct addrinfo hints,*res;
		
		memset(&hints,0,sizeof(hints));
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_family = AF_INET;
		
		if ( (err = getaddrinfo(host, service, &hints, &res) ) != 0 ) {
			fprintf(stderr, "error %d\n",err);
			return NULL;
		}
		
		int sock = socket(res->ai_family,res->ai_socktype,res->ai_protocol);
		if ( sock < 0 ) {
			TEA_PERROR("Can't create TCP socket");
			close(sock);
			freeaddrinfo(res);
			return NULL;
		}
		// timeout
		if ( timeout > 0 ) {
			struct timeval _timeout;
			_timeout.tv_sec = timeout/5;
			_timeout.tv_usec = 0;
			
			if ( setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&_timeout, sizeof(timeval)) ) {
				TEA_PERROR("setsockopt failed");
				close(sock);
				freeaddrinfo(res);
				return NULL;
			}
			if ( setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&_timeout, sizeof(timeval)) ) {
				TEA_PERROR("setsockopt failed");
				close(sock);
				freeaddrinfo(res);
				return NULL;
			}
		}
		if ( (err = connect(sock, res->ai_addr, res->ai_addrlen) ) != 0 ) {
			TEA_PERROR("Connect error");
			close(sock);
			freeaddrinfo(res);
			return NULL;
		}
		// build get query
		const char *tpl = "GET /%s HTTP/1.0\r\nHost: %s\r\nUser-Agent: HTMLGET 1.0\r\n\r\n";
		char	query[1024];
		sprintf(query,tpl,(page[0]=='/'?page+1:page),host);
		
		// send
		int sent = 0;
		while ( sent < strlen(query) ) {
			err = send(sock,query+sent,strlen(query)-sent,0);
			if ( err == -1 ) {
				TEA_PERROR("Can't send query");
				close(sock);
				freeaddrinfo(res);
				return NULL;
			}
			sent += err;
		}
		
		//recv
		char cbuf[1024];
		bool start = false;
		TeaBuffer	*buf = NULL;
		
		time_t stime = time(NULL);
		errno = 0;
		while ( true ) {
			err = recv(sock, cbuf, 1024, 0);
			if ( err == 0 ) break;
			if ( err == -1 ) {
				if ( errno == EAGAIN )	// timeout
				{
					if ( timeout < time(NULL) - stime ) break;
					else continue;
				}
				break;
			}
			if ( start ) {
				(*buffer)(err)>>cbuf;
			} else {
				char* content = strstr(cbuf,"\r\n\r\n");
				if ( content != NULL ) {
					if ( buffer == NULL ) buf = new TeaBuffer();
					else buf = buffer;
					buf->clear();
					content = content + 4;
					(*buffer)(cbuf+err-content)>>content;
					start = true;
				}
			}
		}
		fprintf(stderr, "%d\n",errno);
		close(sock);
		freeaddrinfo(res);
		if ( err < 0 || !start ) {
			fprintf(stderr,"Error (%ld) (%d) : %s/%s\n",time(NULL)-stime,errno,host,page);
			TEA_PERROR("Error receiving data");
			if ( buffer == NULL && buf != NULL )
				delete buf;
			return NULL;
		}
		return buffer;
	}
}