/*
 *  TeaBuffer.h
 *  tea
 *
 *  Created by teadust on 11-6-3.
 *
 */
#ifndef __TEA_BUFFER_H__
#define __TEA_BUFFER_H__

#define	TEMPVARNUMER	20

#define	INIT_BUFFER_SIZE	10
#define	REALLOC_BUFFER_SIZE	9
#include "tea.h"

#include "cocos2d.h"

extern unsigned short	temp_us[TEMPVARNUMER];
extern short	temp_s[TEMPVARNUMER];
extern unsigned char	temp_uc[TEMPVARNUMER];
extern char	temp_c[TEMPVARNUMER];
extern unsigned int	temp_ui[TEMPVARNUMER];
extern int	temp_i[TEMPVARNUMER];

//using namespace tea;

namespace tea {
	template <typename T>
	T& ReverseNumber(T& t) {
		if ( sizeof(T) <= 1 ) return t;
		size_t s = sizeof(T);
		unsigned char tmp;
		unsigned char* tp = (unsigned char*)&t;
		for (int i=0; i<s/2; ++i) {
			tmp = tp[s-i-1];
			tp[s-i-1] = tp[i];
			tp[i] = tmp;
		}
		return t;
	}
	
#define getReverseNumber(stream,type,param)	\
{ type tmp; stream<<tmp;param = ReverseNumber(tmp); }
#define	getStreamData(stream,type,param)	\
{ type tmp; stream<<tmp;param = tmp; }
#define getStreamString(stream,param)	\
{ unsigned short t; stream<<t; ReverseNumber(t); stream(t)<<param; }
    
	class TeaBuffer : public cocos2d::CCObject {
	protected:
		char*	buffer;
		size_t	max,rptr,wptr;
		
		size_t	count;
	public:
//        int testId;
		TeaBuffer(size_t _max=0)
		{
			buffer = NULL;
			count = rptr = wptr = 0;
			max = _max;
//			if ( max )
//				safe_malloc(buffer, max);
            if(max){
                buffer = (char*)malloc(sizeof(char)*max);
                memset((void*)buffer,0,sizeof(char)*max);
            }
			init();
		}
		TeaBuffer(const char* content,size_t len)
		{
			buffer = NULL;
			count = rptr = wptr = max = 0;
			pack(len);
            if ( content ) {
                memcpy(buffer, content, len);
                wptr = len;
            }
			init();
		}
		TeaBuffer(TeaBuffer& father,size_t len)
		{
			buffer = NULL;
			count = rptr = wptr = max = 0;
			assert(father.wptr >= father.rptr + len);
			pack(len);
			memcpy(buffer, father.buffer+father.rptr, len);
			wptr = len;
            
			father.rptr += len;
			init();
		}
		~TeaBuffer() {
//            CCLOG("~TeaBuffer---------------------%d",testId);
			SAFE_DELETE(buffer);
		}

	protected:
		virtual void init() {
//            static int kk;
//            kk++;
//			testId = kk;
		}
	public:
        void setBuffer(TeaBuffer& otherBuffer,size_t len)
		{
			buffer = NULL;
			count = rptr = wptr = max = 0;
			assert(otherBuffer.wptr >= otherBuffer.rptr + len);
			pack(len);
			memcpy(buffer, otherBuffer.buffer+otherBuffer.rptr, len);
			wptr = len;
			init();
		}
        
		/**
		 * output
		 **/
		template <typename T>
		T getData(bool isReverse=false) {
			T t;
			output(t,isReverse);
			return t;
		}
		template <typename T>
        TeaBuffer& output(T& t,bool isReverse=false) {
            size_t s = sizeof(T);
//            cocos2d::CCLog("%d,%d",wptr,rptr);
            assert(wptr >= rptr + s);
			memcpy((void*)&t,buffer+rptr,s);
			rptr += sizeof(T);
            if ( isReverse && s > 1 ) {
                unsigned char tmp;
                unsigned char* tp = (unsigned char*)&t;
                for (int i=0; i<s/2; ++i) {
                    tmp = tp[s-i-1];
                    tp[s-i-1] = tp[i];
                    tp[i] = tmp;
                }
            }
			return *this;
        }
		template <typename T>
		TeaBuffer& outputForTest(T& t,bool isReverse=false) {
            size_t s = sizeof(T);
            assert(wptr >= rptr + s);
			memcpy((void*)&t,buffer+rptr,s);
			//rptr += sizeof(T);
            if ( isReverse && s > 1 ) {
//				CCLOG("OUTPUT FOR TEST ");
                unsigned char tmp;
                unsigned char* tp = (unsigned char*)&t;
                for (int i=0; i<s/2; ++i) {
                    tmp = tp[s-i-1];
                    tp[s-i-1] = tp[i];
                    tp[i] = tmp;
                }
            }
			return *this;
        }
		
        template <typename T>
        TeaBuffer& outputArray(T* t,int num=1,bool isReverse=false) {
            if ( num <= 0 ) num = -1;
            while (num) {
                output(*t,isReverse);
                if ( num > 0 ) num--;
                else if ( num < 0 && *t == 0 ) num = 0;
                ++t;
            }
            return *this;
        }
        
        TeaBuffer& outputBuffer(TeaBuffer& other,int num=0) {
            if ( num <= 0 ) num = wptr - rptr;
            assert(num <= wptr - rptr);
            other.pack(num);
            memcpy(other.buffer+other.wptr,buffer+rptr,num);
            rptr += num;
            other.wptr += num;
            return *this;
        }
		
		template <typename T>
		TeaBuffer& operator<<(T& t) {
			return output(t);
		}
		template <typename T>
		TeaBuffer& operator<<(T* t) {
			outputArray(t,count);
			count = 0;
			return *this;
		}
		
//		/**
//		 * input
//		 **/
//		TeaBuffer& intputFromFile(const char *url) {
//			// load files by cocos2d
//			cocos2d::CCFileData data(url, "rb");
//			unsigned long nSize  = data.getSize();
//			unsigned char* pBuffer = data.getBuffer();
//			assert( NULL != pBuffer && 0 != nSize);
//			return (*this)(nSize)>>pBuffer;
//			
//		}
		
		template <typename T>
		TeaBuffer& input(T t,bool isReverse=false) {
			size_t s = sizeof(T);
            if ( isReverse && s > 1 ) {
                unsigned char tmp;
                unsigned char* tp = (unsigned char*)&t;
                for (int i=0; i<s/2; ++i) {
                    tmp = tp[s-i-1];
                    tp[s-i-1] = tp[i];
                    tp[i] = tmp;
                }
            }
			pack(s);
			memcpy(buffer + wptr,(void*)&t,sizeof(T));
			wptr += sizeof(T);
			return *this;
			
		}
		
		template <typename T>
        TeaBuffer& inputArray(T* t,int num=1,bool isReverse=false) {
//            if ( num <= 0 ) num = -1;
//            while (num) {
//                input(*t,isReverse);
//                if ( num > 0 ) num--;
//                else if ( num < 0 && *t == 0 ) num = 0;
//                ++t;
//            }
//            return *this;
            
            if(isReverse){
                if ( num <= 0 ) num = -1;
                while (num) {
                    input(*t,isReverse);
                    if ( num > 0 ) num--;
                    else if ( num < 0 && *t == 0 ) num = 0;
                    ++t;
                }
            }else{
                size_t s = sizeof(T);
                int size = num*s;
                pack(size);
                memcpy(buffer + wptr,t,size);
                wptr += size;
            }
            
            return *this;
            
        }
//        
//        TeaBuffer& inputPChar(unsigned char* pchar,int size) {
//            pack(size);
//            memcpy(buffer + wptr, pchar, size);
//            wptr += size;
//            return *this;
//        }
		
        TeaBuffer& inputBuffer(TeaBuffer& other,int num=0) {
			other.outputBuffer(*this,num);
            return *this;
        }
		
		template <typename T>
		TeaBuffer& operator>>(T t) {
			return input(t);
		}
		template <typename T>
		TeaBuffer& operator>>(T* t) {
			inputArray(t,count);
			count = 0;
			return *this;
		}
		TeaBuffer& operator>>(TeaBuffer& other) {
            return inputBuffer(other);
        }
		TeaBuffer& operator()(size_t _count) {
			count = _count;
			return *this;
		}
		
		TeaBuffer& operator+=(const int &a) {
			assert(wptr >= rptr + a);
			rptr += a;
			return *this;
		}
		TeaBuffer& operator-=(const int &a) {
			if ( rptr > a ) {
				rptr -= a;
			}
			return *this;
		}
		TeaBuffer& operator=(const int p) {
			assert(wptr >= p);
			rptr = p;
			return *this;
		}
        
		char* getLine() {
            int ret = rptr;
			while ( wptr > rptr ) {
				if ( buffer[rptr] == '\n' ) {
					buffer[rptr] = 0;
					rptr++;
					return buffer + ret;
				}
				if ( buffer[rptr] == '\r' && buffer[rptr+1] == '\n' ) {
					buffer[rptr] = 0;
					rptr += 2;
					return buffer + ret;
				}
				rptr++;
			}
            rptr = ret;
            if ( ret == wptr ) {
                return NULL;
            }
            else {
                rptr = wptr;
                return buffer+ret;
            }
		}
		
        char* operator+(const int p) {
            assert(wptr - rptr - p > 0);
            return buffer + rptr + p;
        }
        char operator[](const unsigned int p) {
            assert(wptr- rptr - p > 0);
            return buffer[rptr+p];
        }
        void clear() {
            rptr = wptr = 0;
        }
		void pack(size_t increase=0) {
            if ( increase==0 || rptr ) {
                memcpy(buffer,buffer+rptr,wptr-rptr);
                wptr -= rptr;
                rptr = 0;
//                return;//如果increase>rptr，就会造成内存混乱问题,所以注释掉
            }
			if ( max < wptr + increase ) {
				size_t _max = MAX_NUMBER(1<<INIT_BUFFER_SIZE, (((wptr + increase)>>REALLOC_BUFFER_SIZE) + 1) << REALLOC_BUFFER_SIZE);
				char *tmp=NULL;
				if ( _max > max ) {
//					safe_malloc(tmp, _max);
                    tmp = (char*)malloc(sizeof(char)*_max);
                    memset((void*)tmp,0,sizeof(char)*_max);
                    
					max = _max;
				}
				else {
                    tmp = buffer;
                }
                
				if (wptr > rptr && buffer != NULL)
					memcpy(tmp,buffer,wptr);
				if ( tmp != buffer )
					SAFE_DELETE(buffer);
				buffer = tmp;
				memset(buffer+wptr,0,max-wptr);
			}
		}
		/**
		 * 临时解决
		 **/
		template <typename T>
		T	getNumber() {
			T t;
			(*this)<<t;
			if ( sizeof(T) > 1 )
				ReverseNumber(t);
			return t;
		}
		
		/**
		 * temp
		 **/
		unsigned int getVarint() {
			unsigned int work;
			work = getNumber<unsigned char>();
			if ( work & 0x80 )
				return work & 0x7f;
			return work + ( (getNumber<unsigned char>() & 127 ) << 7 );
		}
		/**
		 * 移动写指针 危险，临时解决
		 **/
		void	move(int ptr,bool offset=false) {
			if ( offset ) ptr = wptr + ptr;
			assert ( ptr >= rptr );
			wptr = ptr;
		}
		size_t getReadPoint() {
			return rptr;
		}
		size_t getWritePoint() {
			return wptr;
		}
		unsigned int size() {
			return wptr-rptr;
		}
		
		char*	getBuffer() {
			return buffer+rptr;
		}
		
		void resetForRead(){
			rptr = 0;
		}
		
//		bool uncompress();
	};
	
};
#endif