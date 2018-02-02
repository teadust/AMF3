//
//  TAResource.h
//  pkclient
//
//  Created by Touch Age on 13-11-20.
//
//

#ifndef __TOUCHAGE__TAResourceCenter__
#define __TOUCHAGE__TAResourceCenter__

#include <iostream>
#include <map>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <algorithm>

#include "cocos2d.h"
#include "TAXMovieClipStage.h"

class TAResourceCenter : public cocos2d::CCObject {
private:
    class Worker {
    public:
        int             wid;
        int             sock;
        bool            running;
        char            *buffer;
        int             capacity;
        int             point;
        std::string     fileName;
        
        bool            headCompleted;
        bool            completed;
        int             content_length;
        int             httpCode;
        
        int             timeout;
        std::function<void(std::string&)>   callback;
    public:
        void            reset();
        bool            addData(char* data,int len);
    public:
        Worker(int wid);
        ~Worker();
    };
    typedef struct {
        cocos2d::CCObject    *object;
        std::function<void(std::string&,int,bool)>  callback;
    }   Waiting;
private:
    TAResourceCenter();
    static  TAResourceCenter*      s_taResourceCenter;
public:
    ~TAResourceCenter();
    static TAResourceCenter*   sharedInstance();
    void    init(std::string baseURL,std::string storePath);
    //  rurl 下载的url
    //  force  0 - 如果本地有，直接返回，如果md5不对，将文件加入待更新列表
    //          1 - m5.txt 比较不对，则直接更新
    //          2 - 直接到网上更新
    // callback 带入参数，文件本地存储的绝对路径，返回码，是否已经更新
    void    loadResource(const char* rurl,int force,std::function<void(std::string&,int,bool)> callback,
                         cocos2d::CCObject* target=nullptr);
    void    loadResource(std::vector<std::string> rurls,bool force,
                         std::function<void(std::shared_ptr<std::vector<std::string>>,int,int,int)> callback,
                         cocos2d::CCObject* target=nullptr);
    void loadTaxAniNode(const char* name,std::function<void(TAXMovieClipStage*)> callback,
                        cocos2d::CCObject* target=nullptr);
    TAXMovieClipStage* loadTaxAniNode(const char* name,bool cached=true);
public:
    void    startDownloadMD5() {
        m_cdn_status = 1;
    }
    bool    isFinishDownloadNeed() {
        return m_cdn_status == 4;
    }
private:
    void    handle();
    void    tick();
    void    finishWorker(Worker& worker);
private:
    std::string                           m_host;
    int                                   m_port;
    std::string                           m_baseurl;
    
    std::mutex                            m_resource_lock;
    std::map<std::string,std::string>     m_resources;
    std::vector<std::string>              m_waiting_updates;
    
    std::map<std::string,std::string>     m_cdn_resources;
    std::vector<std::string>              m_can_resources;
    std::vector<std::string>              m_need_resources;
    
    std::atomic<int>                      m_cdn_status;
    
    std::mutex  m_waiting_lock;
    std::map<std::string,std::pair<int,std::vector<Waiting>>>  m_waitings;
    
    std::set<cocos2d::CCObject*>          m_objects;
    
    std::vector<Worker*>                  m_workers;
    
    std::string                           m_basePath;
    
    std::thread                           *m_thread;
    std::atomic<bool>                     m_running;
//    //  i5 临时解决
//public:
//    bool        isI5FileName(const char* fileName) {
//        if ( memcmp(fileName,"i5/",3) == 0 ) return true;
//        for ( int j=strlen(fileName)-1;j>=0;--j ) {
//            if ( fileName[j] == '/' ) {
//                return j >= 3 && fileName[j-1] == '5' && fileName[j-2]=='i' && fileName[j-3] =='/';
//            }
//        }
//        return false;
//    }
//    char*       chargeI5FileName(const char* fileName,bool reverse=false) {
//        char    *ret = new char[strlen(fileName) + 4];
//        int j = strlen(fileName) - 1;
//        for ( ;j>=0;--j)
//            if ( fileName[j] == '/' ) break;
//        memcpy(ret, fileName, strlen(fileName)+1);
//        if ( j == -1 ) {
//            if ( reverse ) return ret;
//            else {
//                sprintf(ret,"i5/%s",fileName);
//                return ret;
//            }
//        }
//        if ( isI5FileName(fileName) ) {
//            if ( reverse ) {
//                sprintf(ret+j-3,"%s",fileName+j);
//            }
//        } else {
//            if ( !reverse ) {
//                sprintf(ret+j,"/i5/%s",fileName+j+1);
//            }
//        }
//        return ret;
//    }
public:
    static std::function<bool(void)>      s_downloadAll;        // 请注入是否可以后台下载全部的资源文件。
//    static bool                           s_isI5;               // 注入是否为I5
    //    static const char*                    s_platform;           // 请注入platform, 用于下载不同版本的资源。
};
#endif /* defined(__TOUCHAGE__TAResourceCenter__) */
