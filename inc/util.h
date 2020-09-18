#ifndef _UTIL_H
#define _UTIL_H

static inline int clamp( int n, int a, int b ) {
	if( n < a ) {
		return a;
	}
	if( n > b ) {
		return b;
	}
	return n;
}

static inline int saturate( int n ) {
	return clamp( n, 0, 255 );
}

#endif
