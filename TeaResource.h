//
//  TeaResource.h
//  tea
//
//  Created by yaowei li on 11-7-18.
//  Copyright 2011年 touchage. All rights reserved.
//
#ifndef __TEA_RESOURCE_H__
#define __TEA_RESOURCE_H__

#include "cocos2d.h"
#include "tea.h"
#include "TeaBuffer.h"

using namespace cocos2d;

namespace tea {
	enum TEA_RESOURCE_STATUS {
		TEA_RESOURCE_STATUS_WAITING = 0,
		TEA_RESOURCE_STATUS_LOADING,
		TEA_RESOURCE_STATUS_LOADED,
		TEA_RESOURCE_STATUS_READY,
		TEA_RESOURCE_STATUS_FAILED
	};
	class TeaResourceManager;
	class TeaResource : public CCObject {// public TeaProtocolDispatch<TeaResourceProtocol> {
		friend class TeaResourceManager;
	private:
        std::string		m_pUrl;//配置上的url,一般用来坐为key
		volatile TEA_RESOURCE_STATUS m_eStatus;
	public://test by bobo protected
		TeaBuffer	*m_pBuffer;
		unsigned int	m_uRetry_Count;
		unsigned int	m_uMax_Retry_Count;
	public:
//        int testId;
        bool showNetWork;//是否显示loading界面
        bool isNeed;//是否是必须的
        
        //资源下载用
        CCArray* callBackArray;
		/**
		 *	Construction
		 **/
		TeaResource(const char* url,long liveTime=0);
		~TeaResource();
		
		std::string realURL;//资源的真实地址，用来下载资源，可以是zip，sdcard，服务器上
		
		/**
		 * inline
		 **/
		inline std::string	getUrl() const { return m_pUrl; }
		inline unsigned int	getStatus() const { return m_eStatus; }
		
		/**
		 *	loading resource
		 **/
		virtual bool stream() = 0;
		
		bool isLoaded();
		bool isFailed();
		bool isPending();
	
		/**
		 *	get user data path
		 **/
		static const char* getUserDataBasePath();
	protected:
//		virtual bool readLocalResource();
//		virtual bool readHttpResource();
	};
}

#endif
