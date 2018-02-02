//
//  TAHttpClient.h
//  pkclient
//
//  Created by Touch Age on 13-10-7.
//
//

#ifndef __pkclient__TAHttpClient__
#define __pkclient__TAHttpClient__

#include <iostream>
#include <map>
#include <string>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <functional>
#include "cocos2d.h"
#include "md5.h"
#include "TANotificationCenter.h"

class TAResource {
public:
    std::string     url;
    std::string     key;
    std::string     fileName;
    std::string     host;
    int             port;
    std::string     getPath;
    
    bool            necessary;
    bool            headCompleted;
    int             sock;
    int             status;
    int             delay;
    int             retryed;
    int             fd;
    std::string     buffer;
    int             httpCode;
    int             content_length;
public:
    TAResource():url(),key(),fileName(),host(),port(80),getPath(),necessary(false),sock(-1),delay(-1),retryed(-1),fd(-1) {
        httpCode = 0;
        content_length = 0;
        headCompleted = false;
    }
    ~TAResource() {
    }
    bool addData(char* data,int len);
};
class TAHttpClient : public cocos2d::CCObject {
private:
    TANotificationCenter                    center;
    std::map<std::string,TAResource>         resources;
    bool    isStarted;
    void    update();
private:
    TAHttpClient() {
    }
public:
    virtual ~TAHttpClient() {
        if ( isStarted ) {
            stop();
        }
    }
    static TAHttpClient* shareTAHttpClient();
    void start();
    void stop();
    bool markError(TAResource& resource);
    void markCompleted(TAResource& resource);
    void addResource(std::string url,const char* key=NULL,const char* fileName=NULL,bool necessary = true,bool refresh=false);
    
    int addCompletedObserver(const char* key,std::function<void(int,std::string)> callback);
    void removeCompletedObservers(const char* key);
    void removeCompletedObserver(const char* key,int index);
    
    int addNetworkErrorObserver(std::function<void(int,std::string)> callback);
    void removeNetworkErrorObservers();
    void removeNetworkErrorObserver(int index);
};
#endif /* defined(__pkclient__TAHttpClient__) */
