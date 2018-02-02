/*
 *  TeaBuffer.cpp
 *  tea
 *
 *  Created by teadust on 11-6-4.
 *  Copyright 2011 touchage. All rights reserved.
 *
 */

#include "TeaBuffer.h"

#define PNG_NO_PEDANTIC_WARNINGS


//#include "zlib.h"

unsigned short	temp_us[TEMPVARNUMER];
short	temp_s[TEMPVARNUMER];
unsigned char	temp_uc[TEMPVARNUMER];
char	temp_c[TEMPVARNUMER];
unsigned int	temp_ui[TEMPVARNUMER];
int	temp_i[TEMPVARNUMER];

#define	UNCOMPRESS_MAXLEN	1024
using namespace tea;
//bool TeaBuffer::uncompress() {
//	size_t end = wptr;
//	
//	z_stream strm;
//	SET_ZERO(strm);
//	inflateInit(&strm);
//	
//	strm.avail_in = end-rptr;
//	strm.next_in = (Byte*)(buffer+rptr);
//	
//	int ret;
//	do {
//		strm.avail_out = MIN_NUMBER(UNCOMPRESS_MAXLEN,strm.avail_in * 3);
//		pack(strm.avail_out);
//		strm.next_out = (Byte*)(buffer+wptr);
//		
//		wptr += strm.avail_out;
//		
//		/**
//		 * 解压错误
//		 * 恢复现场，返回 false
//		 **/
//		ret = inflate(&strm, Z_NO_FLUSH);
//		if ( ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR ) {
//			wptr = end;
//			return false;
//		}
//		wptr -= strm.avail_out;
//		
//	} while ( ret != Z_STREAM_END );
//	rptr = end;
//	pack();
//	inflateEnd(&strm);
//	return true;
//}

