//
//  TAHttpClient.cpp
//  pkclient
//
//  Created by Touch Age on 13-10-7.
//
//

#include "TAHttpClient.h"

using namespace std;
using namespace cocos2d;
#define TAHTTPCLIENT_STATUS_STOP     0
#define TAHTTPCLIENT_STATUS_RUNNING  1
#define TAHTTPCLIENT_STATUS_FINISHED 2
#define TAHTTPCLIENT_STATUS_ERROR    4

#define TAHTTPCLIENT_BUFFER_SIZE     10240

#define TAHTTPCLIENT_RETRY_COUNT     3

bool TAResource::addData(char* data,int len) {
    if ( headCompleted ) {
        if ( fd >= 0 ) {
            len = len >content_length?content_length:len;
            write(fd,data,len);
            content_length -= len;
        }
        return true;
    }
    data[len] = 0;
    buffer += data;
    size_t pos,pos1;
    pos = buffer.find("\r\n\r\n");
    if ( pos != -1 ) {
        headCompleted = true;
        std::string head = buffer.substr(0,pos);
        std::cout<<head<<std::endl;
        data = data + pos + 4;
        len -= (pos+4);
        pos = head.find("HTTP/");
        if ( pos == -1 ) return false;
        pos1 = head.find(" ",pos);
        httpCode = atoi(head.substr(pos1+1,head.find(" ",pos1+1)).c_str());
        if ( httpCode == 304 ) {
            content_length = 0;
            return true;
        }
        if ( httpCode != 200 ) return false;
        pos = head.find("Content-Length:");
        content_length = atoi(head.substr(pos+15,head.find("\r\n",pos+15)-pos-15).c_str());
        if ( content_length <= 0 ) return false;
        std::string fname = cocos2d::CCFileUtils::sharedFileUtils()->getWritablePath() + fileName;
        fd = open(fname.c_str(),O_WRONLY | O_CREAT | O_TRUNC,0644);
        if ( fd < 0 )
            return false;
        if ( fd >= 0 && len > 0 ) {
            len = len >content_length?content_length:len;
            write(fd,data,len);
            content_length -= len;
        }
    }
    return true;
}

static inline void stopResource(TAResource& resource) {
    if ( resource.sock >= 0 ) {
        close(resource.sock);
        resource.sock = -1;
    }
    if ( resource.fd >= 0 ) {
        close(resource.fd);
        resource.fd = -1;
    }
    
}

TAHttpClient* TAHttpClient::shareTAHttpClient() {
    static TAHttpClient*    instance=NULL;
    if ( instance == NULL ) {
        instance = new TAHttpClient();
    }
    return instance;
}

bool TAHttpClient::markError(TAResource& resource) {
    resource.retryed++;
    resource.status = TAHTTPCLIENT_STATUS_STOP;
    if ( resource.retryed > TAHTTPCLIENT_RETRY_COUNT ) {
        if ( resource.necessary ) {
            stop();
            resource.retryed = 0;
            center.postNotification("NETWORKERROR",resource.key);
            return false;
        } else {
            center.postNotification(resource.key.c_str(), "");
        }
        resource.status = TAHTTPCLIENT_STATUS_ERROR;
    }
    stopResource(resource);
    resource.delay = 200;  //等待帧数
    return true;
}

void TAHttpClient::markCompleted(TAResource &resource) {
    resource.status = TAHTTPCLIENT_STATUS_FINISHED;
    resource.buffer = "";
    stopResource(resource);
    center.postNotification(resource.key.c_str(),cocos2d::CCFileUtils::sharedFileUtils()->getWritablePath() + resource.fileName);
}
void    TAHttpClient::update() {
    int     current = 0;
    char    buffer[TAHTTPCLIENT_BUFFER_SIZE+1];
    for ( auto iter = resources.begin();iter != resources.end();++iter ) {
        if ( iter->second.status == TAHTTPCLIENT_STATUS_RUNNING ) {
            int ret=recv(iter->second.sock,buffer,TAHTTPCLIENT_BUFFER_SIZE,0);
            if ( ret == -1 && errno != EAGAIN ) {
                markError(iter->second);
                if ( isStarted == false ) return;
                continue;
            }
            else if ( ret == 0 ) {
                markCompleted(iter->second);
                continue;
            }
            current += 1;
            if ( ret > 0 ) {
                if ( iter->second.addData(buffer, ret) == false ) {
                    markError(iter->second);
                    if ( isStarted == false ) return;
                } else {
                    if ( iter->second.headCompleted && iter->second.content_length <= 0 ) {
                        markCompleted(iter->second);
                        continue;
                    }
                }
            }
        }
    }
    for ( auto iter = resources.begin();current<5 && iter != resources.end();++iter ) {
        if ( iter->second.status == TAHTTPCLIENT_STATUS_STOP && (iter->second.delay--) <= 0 ) {
            iter->second.sock = socket(AF_INET,SOCK_STREAM,0);
            struct sockaddr_in  server;
            if ( iter->second.sock != -1 ) {
                server.sin_addr.s_addr = inet_addr(iter->second.host.c_str());
                if((server.sin_addr.s_addr = inet_addr(iter->second.host.c_str())) == (unsigned long)INADDR_NONE)
                {
                    struct hostent *hostp = gethostbyname(iter->second.host.c_str());
                    if(hostp == NULL) {
                        stop();
                        center.postNotification("NETWORKERROR","Connect error");
                        return;
                    }
                    memcpy(&server.sin_addr, hostp->h_addr, sizeof(server.sin_addr));
                }
                server.sin_family = AF_INET;
                server.sin_port = htons(iter->second.port);
                if ( connect(iter->second.sock,(struct sockaddr*)&server,sizeof(server)) >= 0 ) {
                    int flags = fcntl(iter->second.sock,F_GETFL,0);
                    fcntl(iter->second.sock,F_SETFL,flags | O_NONBLOCK);
                } else {
                    CCLOG("%d",errno);
                    iter->second.sock = -1;
                }
            }
            if ( iter->second.sock >= 0 ) {
                iter->second.status = TAHTTPCLIENT_STATUS_RUNNING;
                
                current+=1;
                std::ostringstream  headstr;
                std::string fname = cocos2d::CCFileUtils::sharedFileUtils()->getWritablePath() + iter->second.fileName;

                headstr<<"GET "<<iter->second.getPath<<" HTTP/1.1\r\n";
                headstr<<"Connection: Close\r\n";
                headstr<<"User-Agent: Mozilla/4.0\r\n";
                headstr<<"If-None-Match: "<<file_md5(fname.c_str())<<"\r\n";
                headstr<<"HOST: "<<iter->second.host;
                
                if ( iter->second.url.find(iter->second.host + ":") != -1 )
                    headstr<<":"<<iter->second.port;
                headstr<<"\r\n\r\n";
                const char* msg = headstr.str().c_str();
                send(iter->second.sock,msg,strlen(msg),0);
            } else {
                markError(iter->second);
                if ( isStarted == false ) return;
            }
            
        }
    }
    
}

void TAHttpClient::start() {
    if ( isStarted == false ) {
        CCDirector::sharedDirector()->getScheduler()->scheduleSelector(schedule_selector(TAHttpClient::update), this, 1.0/20, false);
        isStarted = true;
    }
}

void TAHttpClient::stop() {
    isStarted = false;
    CCDirector::sharedDirector()->getScheduler()->unscheduleSelector(schedule_selector(TAHttpClient::update), this);
    for ( auto iter = resources.begin();iter != resources.end();++iter ) {
        if ( iter->second.status == TAHTTPCLIENT_STATUS_RUNNING ) {
            stopResource(iter->second);
            iter->second.status = TAHTTPCLIENT_STATUS_STOP;
            iter->second.retryed = 0;
        }
    }
}
void TAHttpClient::addResource(std::string url,const char* key,const char* fileName,bool necessary,bool refresh) {
    
    
    CCLOG("%s,%s,%s",url.c_str(),key,fileName);
    
    TAResource  resource;
    resource.url = url;
    if ( url.substr(0,7) == "http://" ) {
        url = url.substr(7);
    }
    if ( key != NULL ) {
        resource.key = key;
    } else {
        resource.key = url;
    }
    if ( resources.count(resource.key) == 1 ) {
        resource = resources[resource.key];
        if ( resource.status == TAHTTPCLIENT_STATUS_FINISHED ) {
            if ( refresh )
                resource.status = TAHTTPCLIENT_STATUS_STOP;
            else return;
        }
        else if ( resource.status == TAHTTPCLIENT_STATUS_ERROR ) {
            resource.status = TAHTTPCLIENT_STATUS_STOP;
        } else
            return;
    }
    if ( fileName != NULL ) {
        resource.fileName = fileName;
    } else {
        resource.fileName = key;
    }
    int pos = url.find("/");
    
    CCLOG("%s,%d,",url.c_str(),pos);
    
    if ( pos == -1 ) return;
    resource.host = url.substr(0,pos);
    resource.getPath = url.substr(pos);
    pos = resource.host.find(":");
    
    
    CCLOG("%s,%s,%d,",resource.host.c_str(),resource.getPath.c_str(),pos);
    
    
    if ( pos != -1 ) {
//        resource.port = atoi(resource.host.substr(pos).c_str());
        resource.port = atoi(resource.host.substr(pos+1).c_str());
//        TODO BOBO
        resource.host = resource.host.substr(0,pos);
    } else {
        resource.port = 80;
    }
    CCLOG("%s,%s,%d,%d",resource.host.c_str(),resource.getPath.c_str(),pos,resource.port);
    
    pos = 0;
    while ( true ) {
        pos = resource.key.find("/",pos+1);
        if ( pos == -1 ) break;
        std::string  dirp = CCFileUtils::sharedFileUtils()->getWritablePath() + resource.fileName.substr(0,pos);
        mkdir(dirp.c_str(),0755);
    }
    resource.status = TAHTTPCLIENT_STATUS_STOP;
    resource.delay = 0;
    resource.retryed = 0;
    resource.sock = -1;
    resource.fd = -1;
    resource.headCompleted = false;
    resources[resource.key] = resource;
}

int TAHttpClient::addCompletedObserver(const char* key,std::function<void(int,std::string)> callback) {
    return center.addObserver(key, callback);
}
int TAHttpClient::addNetworkErrorObserver(std::function<void(int,std::string)> callback) {
    return center.addObserver("NETWORKERROR", callback);
}
void TAHttpClient::removeCompletedObservers(const char* key) {
    center.removeObserver(key);
}
void TAHttpClient::removeCompletedObserver(const char* key,int index) {
    center.removeObserver(key,index);
}
void TAHttpClient::removeNetworkErrorObservers() {
    center.removeObserver("NETWORKERROR");
}
void TAHttpClient::removeNetworkErrorObserver(int index) {
    center.removeObserver("NETWORKERROR",index);
}

