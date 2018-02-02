//
//  TAResource.cpp
//  pkclient
//
//  Created by Touch Age on 13-11-20.
//
//

#include "TAResourceCenter.h"
#include "cocos2d.h"
#include <sys/stat.h>
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
#include "TATimer.h"
#include "GameDefine.h"
#include "GameData.h"

#include "TAXPyro.h"

#include "md5.h"
#define WORKER_SIZE        10240           // 10K

using namespace std;
using namespace cocos2d;
TAResourceCenter::Worker::Worker(int id) {
    wid=id;
    sock = -1;
    running = false;
    buffer = (char*)malloc(WORKER_SIZE);
    capacity = WORKER_SIZE;
    point = 0;
    sock = -1;
    callback = nullptr;
    headCompleted = completed = false;
}
TAResourceCenter::Worker::~Worker() {
    if ( buffer != nullptr ) free(buffer);
}

void    TAResourceCenter::Worker::reset() {
    running = false;
    point = 0;
    headCompleted = completed = false;
    content_length = 0;
    callback = nullptr;
    timeout = 0;
}
bool TAResourceCenter::Worker::addData(char* data,int len) {
    if ( capacity-point < len ) {
        capacity = (int) (capacity + ceil((len-capacity+point + 1.0f)/WORKER_SIZE) * WORKER_SIZE) + 1;
        buffer = (char*)realloc(buffer, capacity);
    }
    memcpy(buffer+point,data,len);
    point += len;
    buffer[point] = 0;
    if ( headCompleted == false  ) {
        std::string     head = buffer;
        size_t pos,pos1;
        // 头太大了
        if ( point > 10240 ) return false;
        pos = head.find("\r\n\r\n");
        if ( pos != -1 ) {
            headCompleted = true;
            head = head.substr(0,pos);
            if ( point > pos+4 )
                memcpy(buffer,buffer+pos+4,point-pos-4);
            point -= (pos+4);
            
            pos = head.find("HTTP/");
            if ( pos == -1 ) return false;
            pos1 = head.find(" ",pos);
            httpCode = atoi(head.substr(pos1+1,head.find(" ",pos1+1)).c_str());
            if ( httpCode != 200 ) {
                content_length = 0;
                point = 0;
                return true;
            }
            pos = head.find("Content-Length:");
            content_length = atoi(head.substr(pos+15,head.find("\r\n",pos+15)-pos-15).c_str());
            if ( content_length <= 0 ) return false;
        }
    }
    return true;
}

TAResourceCenter::TAResourceCenter() {
    // 初始化worker，一开始初始化5个worker
    for ( int i=0;i<5;++i )
        m_workers.push_back(new Worker(i));
    m_running = true;
    m_thread = new thread([this](){
        while ( m_running ) {
            this->handle();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
    CCDirector::sharedDirector()->getScheduler()->scheduleSelector(schedule_selector(TAResourceCenter::tick), this, 1.0f, false);
}
TAResourceCenter::~TAResourceCenter() {
    m_running = false;
    m_thread->join();
    delete m_thread;
    for ( Worker *worker : m_workers ) {
        delete worker;
    }
}
void   TAResourceCenter::init(string baseURL,string storePath)
{
    m_basePath  = storePath + ( *storePath.rbegin() == '/' ?"":"/" );
    mkdir(m_basePath.c_str(),0755);
    if ( baseURL.substr(0,7) == "http://" ) {
        baseURL = baseURL.substr(7);
    }
    
    int pos = baseURL.find("/");
    if ( pos == -1 ) return;
    m_host = baseURL.substr(0,pos);
    m_baseurl = baseURL.substr(pos);
    if (*m_baseurl.rbegin() != '/') {
        m_baseurl += "/";
    }
    pos = m_host.find(":");
    if ( pos != -1 ) {
        m_port = atoi(m_host.substr(pos).c_str());
        m_host = m_host.substr(0,pos);
    } else {
        m_port = 80;
    }
    pos = 0;
    m_cdn_status = 0;
    
}
void TAResourceCenter::loadResource(const char* rurl,int force,function<void(string&,int,bool)> callback,
                                    CCObject* target) {
    float start = getCurrentTime();
    //    // 非 var 直接返回
    //    if ( memcmp(rurl, "var/", 4) != 0 && forceHttp == false ) {
    //        if ( callback != nullptr ) {
    //            std::string fn = rurl;
    //            callback(fn,0);
    //        }
    //        return;
    //    }
    lock_guard<mutex>   guard(m_waiting_lock);
    lock_guard<mutex>   guard1(m_resource_lock);
    std::string fn = CCFileUtils::sharedFileUtils()->fullPathForFilename(rurl);
    
    
#ifdef IS_LOCAL_GAME
    CCLog("loadResource %s--------%s",rurl,fn.c_str());
    if ( callback != nullptr )
        callback(fn,0,false);
    
    return;
#endif
    
    
    
    // 判断文件存在，如果不存在直接下载。直接判断fullPath返回的是否相等，如果相等，则认为找不到。
    if ( fn != rurl && force != 2 ) {
        //        CCLOG("---- Exist %s ----",fn.c_str());
        if ( m_resources.find(rurl) == m_resources.end() ) {
            m_resources[rurl] = file_md5(fn.c_str());
        }
        // 不强制的
        // 强制 ： md5.txt 已经下载完成，md5 存在该文件，并且两个md5相等；
        if ( m_cdn_status < 3 || (m_cdn_resources.count(rurl) != 0 && m_resources[rurl] != m_cdn_resources[rurl]) ) {
            if ( force == false ) {
                if ( find(m_waiting_updates.begin(),m_waiting_updates.end(),rurl) == m_waiting_updates.end() )
                    m_waiting_updates.push_back(rurl);
                if ( callback != nullptr )
                    callback(fn,0,false);
                return;
            }
        } else {
            if ( callback != nullptr )
                callback(fn,0,false);
            return;
        }
    } else {
        CCLOG("---- Not Exist %s,or force refresh file ----",fn.c_str());
    }
    if ( m_waitings.find(rurl) == m_waitings.end() ) {
        m_waitings[rurl] = pair<int, vector<Waiting>>(0,vector<Waiting>());
    }
    if ( callback != nullptr ) {
        Waiting waiting;
        waiting.callback = callback;
        waiting.object = target;
        if ( target != nullptr ) {
            if ( m_objects.count(target) == 0 ) {
                target->retain();
                m_objects.insert(target);
            }
        }
        m_waitings[rurl].second.push_back(waiting);
    }
    start = getCurrentTime() - start;
    if ( start > 0.5 ) {
        CCLOG(" --- TAResource loadurl %0.2lf" , start);
    }
    
}
void TAResourceCenter::loadResource(vector<string> rurls,bool force,function<void(shared_ptr<vector<string>>,int,int,int)> callback,CCObject* target){
    if ( rurls.size() == 0 ) return;
    // 引用一次，保证所有的单独下载，均调用回调
    
    if ( target != nullptr ) {
        target->retain();
    }
    int     count = rurls.size();
    auto    index = shared_ptr<int>(new int(0));
    
    shared_ptr<vector<string>>      paths = shared_ptr<vector<string>>(new vector<string>);
    std::function<void(std::string&,int,bool)>     _callback = [count,index,callback,target,paths](std::string& path,int ret,bool isNew) {
        if ( ret == 0 )
            paths->push_back(path);
        // 如果只有2，则除了这里的引用外，
        if ( target == nullptr || target->retainCount() > 2 )
            callback(paths,*index,count,ret);
        *index = *index + 1;
        CCLOG(" -- -- %d %d ----",*index,count);
        if ( *index == count && target != nullptr ) {
            target->release();
        }
    };
    for ( auto url : rurls ) {
        loadResource(url.c_str(), force?1:0, _callback,target);
    }
}
void TAResourceCenter::loadTaxAniNode(const char* name,function<void(TAXMovieClipStage*)> callback,
                                      CCObject *target) {
    char buf[1024];
    sprintf(buf,"%s.ani",name);
    vector<string>  urls;
    urls.push_back(buf);
    sprintf(buf,"%s.png",name);
    urls.push_back(buf);
    loadResource(urls, 0, [callback](shared_ptr<vector<string>> paths, int index, int count, int retcode){
        if ( index == count - 1 ) {
            if ( paths->size() == count ) {
                string&     ani = paths->operator[](0);
                string&     png = paths->operator[](1);
                
                if ( *(ani.rbegin()) == 'g' ) {
                    ani = paths->operator[](1);
                    png = paths->operator[](0);
                }
                TAXMovieClipStage   *stage = TAXMovieClipStage::createWithANIFileAndTextureFile(ani.c_str(), png.c_str());
                stage->addMovieClipInstanceLifeOb(TAXPyroOb::create(stage));
                if ( callback != nullptr )
                    callback(stage);
            }
        }
    },target);
}

TAXMovieClipStage* TAResourceCenter::loadTaxAniNode(const char* name,bool cached) {
    CCString* imageName = CCString::createWithFormat("%s.png",name);
    CCString* aniName = CCString::createWithFormat("%s.ani",name);
    
    std::string ani = aniName->getCString();
    
    CCTexture2D*    texture = CCTextureCache::sharedTextureCache()->addImage(imageName->getCString());
    
    TAXMovieClipStage* stage = TAXMovieClipStage::createWithANIFileAndTextureFile(aniName->getCString(), imageName->getCString());
    stage->addMovieClipInstanceLifeOb(TAXPyroOb::create(stage));
    
    if(!cached){
        CCTextureCache::sharedTextureCache() ->removeTexture(texture);
    }
    return stage;
}

void  TAResourceCenter::finishWorker(Worker& worker) {
    bool succ = (worker.point == worker.content_length) && ( worker.httpCode == 200 || worker.httpCode == 304 );
    CCLOG(" - Download %s finish - httpCode : %d",worker.fileName.c_str(),worker.httpCode);
    std::string fn = m_basePath + worker.fileName;
    if ( succ && worker.httpCode == 200 && worker.point > 0 ) {
        int pos = 0;
        while ( true ) {
            pos = worker.fileName.find("/",pos+1);
            if ( pos == -1 ) break;
            std::string  dirp = m_basePath + worker.fileName.substr(0,pos);
            mkdir(dirp.c_str(),0755);
        }
        int fd = open(fn.c_str(),O_WRONLY | O_CREAT | O_TRUNC,0644);
        if ( fd != -1 ) {
            write(fd, worker.buffer, worker.point);
            close(fd);
        }
    }
    std::lock(m_waiting_lock, m_resource_lock);
    // 如果下载成功，直接更新 m_resources 和 m_cdn_resources。并且删除waiting_update
    if ( succ ) {
        m_resources[worker.fileName] = file_md5(fn.c_str());
        m_cdn_resources[worker.fileName] = file_md5(fn.c_str());
        std::remove(m_waiting_updates.begin(), m_waiting_updates.end(), worker.fileName);
        CCTextureCache::sharedTextureCache()->removeTextureForKey(worker.fileName.c_str());
    }
    if ( m_waitings.find(worker.fileName) != m_waitings.end() ) {
        m_waitings[worker.fileName].first = succ?worker.httpCode:2;
    }
    if ( worker.callback != nullptr && succ ) {
        worker.callback(fn);
    }
    m_resource_lock.unlock();
    m_waiting_lock.unlock();
    
    worker.completed = true;
}
void TAResourceCenter::tick() {
    float start = getCurrentTime();
    
    lock_guard<mutex>   guard(m_waiting_lock);
    std::map<string,pair<int, vector<Waiting>>>     waitings;
    
    std::set<CCObject*>         objects;
    
    for ( auto it : m_waitings ) {
        vector<Waiting>     u;
        for ( Waiting waiting : it.second.second ) {
            if ( waiting.object != nullptr && waiting.object->retainCount() <= 1 ) {  // 已经无人引用
            } else {
                u.push_back(waiting);
            }
        }
        // 已经完成
        std::string fn = m_basePath + it.first;
        if ( it.second.first > 0 ) {
            for ( Waiting waiting : u ) {
                if ( it.second.first == 200 )
                    waiting.callback(fn,0,true);
                else if ( it.second.first == 304 )
                    waiting.callback(fn,0,false);
                else
                    waiting.callback(fn,1,false);
            }
        } else if ( it.second.first < -3 ) {       // 失败三次以上
            for ( Waiting waiting : u ) {
                std::string empty;
                waiting.callback(empty,2,false);      // 网络错误
            }
        } else {
            for ( Waiting waiting : u ) {
                if ( waiting.object != nullptr )
                    objects.insert(waiting.object);
            }
            waitings[it.first] = pair<int,vector<Waiting>>(it.second.first,u);
        }
    }
    if ( objects.size() != m_objects.size() ) {
        for ( CCObject* object : m_objects ) {
            if (objects.count(object) == 0 ) {
                object->release();
            }
        }
        m_objects = std::move(objects);
    }
    if ( m_waitings.size() != waitings.size() ) {
        m_waitings = std::move(waitings);
    }
    
    if ( getCurrentTime() - start > 0.5 ) {
        CCLOG("--- TAResourceCenter tick %0.2lf",getCurrentTime()-start);
    }
}
void TAResourceCenter::handle() {
    std::vector<Worker*>         idles;
    std::vector<std::string>    running;
    for ( Worker* worker : m_workers ) {
        if ( worker->running )
            CCLOG(" - Workder(%d %d) - running %s (%d/%d)",worker->wid,worker->sock,worker->fileName.c_str(),worker->point,worker->content_length);
        
        if ( worker->running && worker->sock != -1 ) {
            while ( true ) {
                char    buffer[1024];
                int ret = recv(worker->sock, buffer,1024,0);
                if ( ret == -1 ) {
                    if ( errno != EAGAIN || worker->timeout > 10 ){
                        close(worker->sock);
                        worker->sock = -1;
                    } else {
                        worker->timeout++;
                    }
                    //                    CCLOG(" - Workder(%d) recv error %d/%d %d",worker->wid,errno,EAGAIN,worker->timeout);
                    break;
                }
                else if ( ret == 0 ) {
                    finishWorker(*worker);
                    close(worker->sock);
                    worker->sock = -1;
                    break;
                }
                if ( ret > 0 ) {
                    if ( worker->addData(buffer, ret) == false ) {
                        close(worker->sock);
                        worker->sock = -1;
                        break;
                    }
                    if ( worker->headCompleted && worker->point >= worker->content_length ) {
                        finishWorker(*worker);
                        break;
                    }
                    worker->timeout = 0;
                }
            }
            if ( worker->sock == -1 ) {
                lock_guard<mutex>   guard(m_waiting_lock);
                if ( m_waitings.count(worker->fileName) > 0 ) {
                    m_waitings[worker->fileName].first--;
                    //                    CCLOG(" - Workder(%d) - error %s %d",worker->wid,worker->fileName.c_str(),m_waitings[worker->fileName].first);
                }
            } else if ( worker->completed ) {
                close(worker->sock);
                worker->sock = -1;
                worker->running = false;
            }
        }
        if ( worker->sock == -1 || worker->running == false ) {
            worker->reset();
            idles.push_back(worker);
        } else {
            running.push_back(worker->fileName);
        }
    }
    // 处理Waiting
    vector<Worker*>      workers;
    // 处理Waiting
    {
        lock_guard<mutex>   guard(m_waiting_lock);
        for ( auto it : m_waitings ) {
            if ( it.second.first > 0 || it.second.first < -3 ) continue;
            if ( idles.size() > 0 && find(running.begin(),running.end(),it.first) == running.end() ) {
                Worker*  worker = idles[idles.size()-1];
                idles.pop_back();
                worker->fileName = it.first;
                workers.push_back(worker);
            }
        }
    }
    // 空置次数，当空置次数达到一定数量，下载更新md5.txt
    static  int     idle_time = 0;
    static const int    max_idle_time = 5000;
    if ( idles.size() > 0 ) {
        // 如果cdn资源为空，则下载cdn资源
        if ( ( m_cdn_status >= 3 && idle_time > max_idle_time) || m_cdn_status == 1 ) {
            
#ifdef IS_LOCAL_GAME
            if(m_cdn_status==1){
                m_cdn_status = 4;
                m_cdn_resources.clear();
                return;
            }
#endif
            
            
            m_cdn_status = 2;
            m_cdn_resources.clear();
            Worker*  worker = idles[idles.size()-1];
            idles.pop_back();
//            worker->fileName = "md5.txt";
            worker->fileName = GameData::getInstance()->getMD5File();
            worker->callback = [this](string& path) {
                FILE *fp = fopen(path.c_str(),"r");
                if ( fp != NULL ) {
                    char    buffer[1024];
                    vector<std::string>     keys;
                    while ( !feof(fp) ) {
                        fgets(buffer,1024,fp);
                        if ( strlen(buffer) > 0 && buffer[0] != '#' ) {
                            if ( memcmp(buffer, "downasyoucan", 12) == 0 ) {
                                char * key = strtok(buffer, " ");
                                if ( key != NULL ) {
                                    char *value;
                                    while ( (value = strtok(NULL, " ")) != NULL ) {
                                        int index = atoi(value);
                                        if ( keys.size() > index ) {
//                                            if ( isI5FileName(keys[index].c_str()) == false )
                                                m_can_resources.push_back(keys[index]);
                                        }
                                    }
                                }
                            }  else if ( memcmp(buffer, "needFile", 8) == 0 ) {
                                char * key = strtok(buffer, " ");
                                if ( key != NULL ) {
                                    char *value;
                                    while ( (value = strtok(NULL, " ")) != NULL ) {
                                        int index = atoi(value);
                                        if ( keys.size() > index ) {
//                                            if ( isI5FileName(keys[index].c_str()) == false )
                                                m_need_resources.push_back(keys[index]);
                                        }
                                    }
                                }
                            } else {
                                char*   key = strtok(buffer, " ");
                                if ( key != NULL ) {
                                    char * value = strtok(NULL, " ");
                                    if ( value != NULL ) {
                                        if ( value[strlen(value)-1] == '\n' )
                                            value[strlen(value)-1] = 0;
//                                        if ( isI5FileName(key) == s_isI5 ) {
//                                            char *rkey = chargeI5FileName(key,s_isI5);
//                                            m_cdn_resources[key] = value;
//                                            m_cdn_resources[rkey] = value;
//                                            delete []rkey;
//                                        } else
                                        if ( m_cdn_resources.count(key) == 0 )
                                            m_cdn_resources[key] = value;
                                        keys.push_back(key);
                                    }
                                }
                            }
                        }
                    }
                    m_cdn_status = 3;
                }
            };
            workers.push_back(worker);
        }
        // 完成 md5 初始化
        if ( m_cdn_status == 3 ) {
            // 必备资源
            bool isfinish = true;
            for ( auto it : m_need_resources ) {
                if ( idles.size() == 0 )    break;
                if ( find(running.begin(),running.end(),it) == running.end() ) {
                    std::string     fn = m_basePath + it;
                    lock_guard<mutex>   guard(m_resource_lock);
                    std::string     md5 = "";
                    if ( m_resources.count(it) == 0 ) {
                        md5 = file_md5(fn.c_str());
                        if ( md5.empty() == false ) {
                            m_resources[it] = md5;
                        }
                    } else {
                        md5 = m_resources[it];
                    }
                    if ( md5 != m_cdn_resources[it] ) {
                        Worker*  worker = idles[idles.size()-1];
                        idles.pop_back();
                        worker->fileName = it;
                        workers.push_back(worker);
                        isfinish = false;
                    }
                } else {
                    isfinish = false;
                }
            }
            if ( isfinish ) m_cdn_status = 4;
        }
        if ( m_cdn_status == 4 ) {
            // 如果需要
            for ( auto it : m_can_resources ) {
                if ( idles.size() <= 3 ) break;
                if ( find(running.begin(),running.end(),it) == running.end() ) {
                    std::string     fn = m_basePath + it;
                    lock_guard<mutex>   guard(m_resource_lock);
                    std::string     md5 = "";
                    if ( m_resources.count(it) == 0 ) {
                        md5 = file_md5(fn.c_str());
                        if ( md5.empty() == false ) {
                            m_resources[it] = md5;
                        }
                    } else {
                        md5 = m_resources[it];
                    }
                    if ( md5 != m_cdn_resources[it] ) {
                        Worker*  worker = idles[idles.size()-1];
                        idles.pop_back();
                        worker->fileName = it;
                        workers.push_back(worker);
                    }
                    
                }
            }
            //  下载那些更新的
            if ( idles.size() > 3 ) {
                lock_guard<mutex>   guard(m_resource_lock);
                auto    it = m_waiting_updates.begin();
                for ( ;it!=m_waiting_updates.end();++it ) {
                    if ( idles.size() <= 3 ) break;
                    if ( find(running.begin(),running.end(),*it) == running.end() ) {
                        Worker*  worker = idles[idles.size()-1];
                        idles.pop_back();
                        worker->fileName = *it;
                        workers.push_back(worker);
                    }
                }
                m_waiting_updates.erase(m_waiting_updates.begin(), it);
            }
            if ( idles.size() > 3 && s_downloadAll != nullptr && s_downloadAll() ) {
                for ( auto it : m_cdn_resources ) {
                    if ( idles.size() <= 3 ) break;
//                    if ( isI5FileName(it.first.c_str()) ) break;
                    if ( find(running.begin(),running.end(),it.first) == running.end() ) {
                        std::string     fn = m_basePath + it.first;
                        lock_guard<mutex>   guard(m_resource_lock);
                        std::string     md5 = "";
                        if ( m_resources.count(it.first) == 0 ) {
                            md5 = file_md5(fn.c_str());
                            if ( md5.empty() == false ) {
                                m_resources[it.first] = md5;
                            }
                        } else {
                            md5 = m_resources[it.first];
                        }
                        if ( md5 != it.second ) {
                            Worker*  worker = idles[idles.size()-1];
                            idles.pop_back();
                            worker->fileName = it.first;
                            workers.push_back(worker);
                        }
                    }
                }
            }
        }
    }
    // 发送请求
    for ( Worker* worker : workers ) {
        if ( worker->sock == -1 ) {
            worker->sock = socket(AF_INET,SOCK_STREAM,0);
            struct sockaddr_in  server;
            if ( worker->sock != -1 ) {
                server.sin_addr.s_addr = inet_addr(m_host.c_str());
                if( server.sin_addr.s_addr == (unsigned long)INADDR_NONE )
                {
                    struct hostent *hostp = gethostbyname(m_host.c_str());
                    if(hostp == NULL){
                        CCLOG("get Host error");
                        fprintf(stderr,"%s:host %d''%s/n",
                                hstrerror(h_errno),h_errno,m_host.c_str());
                        
                        break;
                    }
                    memcpy(&server.sin_addr, hostp->h_addr, sizeof(server.sin_addr));
                }
                server.sin_family = AF_INET;
                server.sin_port = htons(m_port);
                if ( connect(worker->sock,(struct sockaddr*)&server,sizeof(server)) >= 0 ) {
                    int flags = fcntl(worker->sock,F_GETFL,0);
                    fcntl(worker->sock,F_SETFL,flags | O_NONBLOCK);
                } else {
                    CCLOG("%d",errno);
                    worker->sock = -1;
                }
            }
        }
        if ( worker->sock != -1 ) {
            char    headBuf[1024];
//            char*   realFileName = chargeI5FileName(worker->fileName.c_str(),!s_isI5);
            
            const char*   realFileName = worker->fileName.c_str();
            
            if ( m_cdn_status == 4 && m_cdn_resources.count(realFileName) > 0 )
                sprintf(headBuf,"GET %s HTTP/1.1\r\n",(m_baseurl + realFileName).c_str());
            else
                sprintf(headBuf,"GET %s HTTP/1.1\r\n",(m_baseurl + worker->fileName).c_str());
//            if ( realFileName != nullptr )
//                delete[] realFileName;
            
            sprintf(headBuf+strlen(headBuf), "Connection: Keep-Alive\r\nUser-Agent: Mozilla/4.0\r\n");
            std::string     md5 = "";
            if ( m_resources.count(worker->fileName) == 0 ) {
                md5 = file_md5(worker->fileName.c_str());
                if ( md5.empty() == false ) {
                    m_resources[worker->fileName] = md5;
                }
            }
            if ( m_resources.find(worker->fileName) != m_resources.end() )
                sprintf(headBuf+strlen(headBuf), "If-None-Match: %s\r\n",m_resources[worker->fileName].c_str());
            sprintf(headBuf+strlen(headBuf), "HOST: %s",m_host.c_str());
            if ( m_port != 80 )
                sprintf(headBuf+strlen(headBuf), ":%d",m_port);
            sprintf(headBuf+strlen(headBuf), "\r\n\r\n");
            int ret = send(worker->sock,headBuf,strlen(headBuf),0);
            if ( ret == strlen(headBuf) ) {
                //                CCLOG("%s",headBuf);
                worker->running = true;
                worker->point = 0;
                continue;
            }
            close(worker->sock);
            worker->sock = -1;
        }
        // 纪录错误
        lock_guard<mutex>   guard(m_waiting_lock);
        if ( m_waitings.count(worker->fileName) > 0 ) {
            m_waitings[worker->fileName].first = m_waitings[worker->fileName].first - 1;
        }
    }
    // 如果全部空置
    if ( idles.size() == 5 ) {
        idle_time ++ ;
    } else {
        idle_time = 0;
    }
    // 关闭闲置的worker
    for ( Worker* worker : idles ) {
        if ( worker->sock > 0 ) {
            close(worker->sock);
            worker->sock = -1;
        }
    }
}
function<bool(void)>   TAResourceCenter::s_downloadAll = nullptr;
//const char*                 TAResourceCenter::s_platform = nullptr;
//bool                    TAResourceCenter::s_isI5 = false;

TAResourceCenter*      TAResourceCenter::s_taResourceCenter = new TAResourceCenter();
TAResourceCenter*   TAResourceCenter::sharedInstance() {
    return s_taResourceCenter;
}