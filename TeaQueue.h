//
//  TeaQueue.h
//  tea
//
//  Created by yaowei li on 11-7-19.
//  Copyright 2011年 touchage. All rights reserved.
//
#ifndef __TEA_QUEUE_H__
#define	__TEA_QUEUE_H__

#include "cocos2d.h"
//#include "tea.h"

namespace tea {
	template <typename T>
	class TeaQueue : public cocos2d::CCObject {
	protected:
		T*	m_pStore;
		unsigned int m_uHead;
		unsigned int m_uTail;
		unsigned int m_uCapacity;

		bool			m_bIsRefrence;
		bool			m_bIsThreadMetux;
		
		// thread
		pthread_mutex_t		m_mutex;
		
		void queueRequestCapacity(unsigned int uRequest) {
			if ( m_uCapacity - m_uTail >= uRequest ) return;
			if ( m_uHead > 0 )
				memcpy(m_pStore,m_pStore+m_uHead,(m_uTail - m_uHead) * sizeof(T));
			m_uTail -= m_uHead;
			m_uHead = 0;
			
			if ( m_uCapacity - m_uTail >= uRequest ) return;
			m_uCapacity = (m_uTail + uRequest) << 1;
			m_pStore = (T*)realloc(m_pStore,sizeof(T) * m_uCapacity);
		}

	public:
		TeaQueue(unsigned int uCapacity = 4,bool isRefrence = false,bool isThreadMetux=false ) {
			if ( uCapacity > 0 ) m_pStore = (T*)calloc(sizeof(T),uCapacity);
			m_uHead = m_uTail = 0;
			m_uCapacity = uCapacity;
			m_bIsRefrence = isRefrence;
			m_bIsThreadMetux = isThreadMetux;
			
			if ( isThreadMetux )
				pthread_mutex_init(&m_mutex, NULL);
		}
		
		~TeaQueue() {
			if ( m_bIsRefrence ) {
				while ( m_uHead != m_uTail ) 
					m_pStore[m_uHead++]->release();
			}
			if ( m_bIsThreadMetux ) 
				pthread_mutex_destroy(&m_mutex);
			
			CC_SAFE_FREE(m_pStore);
		}
		
		// input, output
//		TeaQueue<T>&	operator>>(T& t) {
//			if ( m_bIsRefrence )
//				t->retain();
//			queueRequestCapacity(1);
//			m_pStore[m_uTail++] = t;
//			return *this;
//		}
        // input, output
		TeaQueue<T>&	operator>>(T& t) {
			if ( m_bIsRefrence )
				t->retain();
                
                if ( m_bIsThreadMetux )
                    pthread_mutex_lock(&m_mutex);
                queueRequestCapacity(1);
                m_pStore[m_uTail++] = t;
                    
                if ( m_bIsThreadMetux )
                    pthread_mutex_unlock(&m_mutex);
            return *this;
		}
        
		TeaQueue<T>&	operator<<(T& t) {
			if ( m_bIsThreadMetux ) 
				pthread_mutex_lock(&m_mutex);
			
			t = m_uHead<m_uTail?m_pStore[m_uHead++]:NULL;
			if ( t && m_bIsRefrence ) {
				t->retain();
				t->autorelease();
				t->release();
			}
			if ( m_bIsThreadMetux )
				pthread_mutex_unlock(&m_mutex);
			
			return (*this);
		}
		
		TeaQueue<T>&	addObject(T& t) {
			return (*this)>>t;
		}
		
		TeaQueue<T>&	getAndRemoveObject(T& t) {
			return (*this)<<t;
		}
		
		inline unsigned int count() {
			return m_uTail - m_uHead;
		}
	};
}

#endif
