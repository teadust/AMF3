//
//  TeaArray.h
//  tea
//
//  Created by yaowei li on 11-7-18.
//  Copyright 2011年 touchage. All rights reserved.
//
#ifndef __TEA_ARRAY_H__
#define __TEA_ARRAY_H__

#include "cocos2d.h"

using namespace cocos2d;

namespace tea {
#define TEA_ARRAY_FOREACH(__array__,__iterator__,__PC__)				\
if (__array__ && __array__->count() > 0)													\
for(__PC__* __iterator__ = __array__->begin();					\
__iterator__ < __array__->end();										\
__iterator__++)

	template < typename T >
	class TeaArray : public CCObject {
	private:
		T				*m_pStore;
		unsigned int	m_uPoint;
		unsigned int	m_uMax;
		
		bool			m_bIsRefrence;
		bool			m_bIsWeakRefrence;
		bool			m_bIsMulti;
		
		void arrayDoubleCapacity() {
			if ( m_uMax == 0 )
				m_uMax = 4;
			else m_uMax <<= 1;
			m_pStore = (T*)realloc(m_pStore,sizeof(T)*m_uMax);
		}
		
	public:
		TeaArray(unsigned int capacity=4,bool isMulti = true ,bool isRefrence = false,bool isWeakRefrence = false) {
			if ( capacity > 0 )
				m_pStore = (T*)calloc(sizeof(T),capacity);
			else 
				m_pStore = NULL;
			m_uPoint = 0;
			m_uMax = capacity;
			m_bIsRefrence = isRefrence;
			m_bIsWeakRefrence = isWeakRefrence;
			m_bIsMulti = isMulti;
		}
		
		~TeaArray() {
			CCLOG("tea Array destory---");
			removeAllChildren();
			CC_SAFE_FREE(m_pStore);
		}
		
		
		int indexOfChild(T child) {
			for ( unsigned int i=0; i< m_uPoint; i++ )
				if ( m_pStore[i] == child ) return i;
			return -1;
		}
		
		bool containsChild(T child) {
			return indexOfChild(child) != -1;
		}
		
		T	childAtIndex(unsigned int index) {
			assert(index >=0 && index < m_uPoint);
			return m_pStore[index];
		}
		void addChild(T child,int index = -1) {
			if ( !m_bIsMulti && indexOfChild(child) != -1 ) return; 
			if ( m_uMax == m_uPoint ) arrayDoubleCapacity();
			if ( m_bIsRefrence ) child->retain(m_bIsWeakRefrence);
			if ( index >= 0 && index < m_uPoint ) 
				memmove(m_pStore + index + 1,m_pStore + index,(m_uPoint - index) * sizeof(T));
			else index = m_uPoint;
			m_uPoint++;
			m_pStore[index] = child;
		}
		
		void addChildren(TeaArray* otherArray,int index = -1) {
			for ( unsigned int i = 0 ; i < otherArray->m_uPoint ; ++i ) {
				addChild(otherArray->m_pStore[i],index);
				if ( index != -1 ) ++index;
			}
		}
		
		T removeChild(int index=-1,bool isFastRemove=false) {
			T r = 0;
			if ( index == -1 ) index = m_uPoint - 1;
			if ( m_uPoint <= index && index < 0 ) return r;
			
			r = m_pStore[index];
			if ( m_bIsRefrence ) {
				r->retain();
				r->autorelease();
				r->release(m_bIsWeakRefrence);
			}
			
			if ( index < m_uPoint ) {
				if ( isFastRemove ) 
					m_pStore[index] = m_pStore[m_uPoint-1];
				else 
					memmove(m_pStore + index,m_pStore + index + 1,(m_uPoint - index) * sizeof(T));
			}
			m_pStore[--m_uPoint] = NULL;
			return r;
		}
		
		void removeChild(T child,bool isFastRemove=false) {
			int index = indexOfChild(child);
			if ( index != -1 ) 
				removeChild(index,isFastRemove);
		}
		
		void removeChildren(TeaArray otherArray,bool isFastRemove=false) {
			for ( unsigned int i = 0 ; i < otherArray->m_uPoint ; ++i )
				removeChild(otherArray->m_pStore[i],isFastRemove);
		}
			
		void removeAllChildren() {
			if ( m_bIsRefrence ) {
				while (m_uPoint) 
					m_pStore[--m_uPoint]->release(m_bIsWeakRefrence);
			} 
			m_uPoint = 0;
		}
		
		void push(T child) {
			addChild(child);
		}
		
		T	pop() {
			return removeChild();
		}
		
		//	inline
		inline unsigned int count() {
			return m_uPoint;
		}
		inline unsigned int capacity() {
			return m_uMax;
		}

		// for each
		inline T*	begin() const {
			return m_pStore;
		}
		
		inline T*	end() const {
			return m_pStore + m_uPoint;
		}
		
		// overloading operator
		
		// Array index
		T&		operator[](unsigned int index) const {
			assert(index >=0 && index < m_uPoint);
			return m_pStore[index];
		}
		
		// push pop
		TeaArray<T>&	operator>>(T& child) {
			addChild(child);
			return (*this);
		}
		//	+
		TeaArray<T>&	operator+(const T child) {
			addChild(child);
			return (*this);			
		}
		TeaArray<T>&	operator<<(T&	child) {
			child = removeChild();
			return (*this);			
		}
		
		//	-
		TeaArray<T>&	operator-(const T child) {
			removeChild(child);
			return (*this);
		}
	};
}

#endif
