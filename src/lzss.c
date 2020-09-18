#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"

typedef struct {
	u16 type:1;
	u16 len:7;
	u16 offset:8;
} lzChunk_t;

void lzss( u8* buf, size_t sz ) {
	/*
	u8 dict[256];
	int di = 0;
	
	memset( dict, 0, 256 );
	*/
	
	int osz = 0;
	
	int n = 0;
	int immLen = 0;
	while( n < sz ) {
		int maxMatch = -1;
		int mOffset = -1;
		
		for( int off = 0; off < 8192; off++ ) {
			if( off + 1 > n ) {
				break;
			}
			
			for( int len = 0; len < 256; len++ ) {
				if( n + len + 1 >= sz ) {
					break;
				}
				
				int match = ( memcmp( buf + n, buf + n - ( off + 1 ), len + 1 ) == 0 );
				
				if( match && len > maxMatch ) {
					maxMatch = len;
					mOffset = off;
				}
			}
		}
		
		if( maxMatch > 2 ) {
			if( immLen > 0 ) {
				//printf( "imm %d\n", immLen );
				osz += 2 + immLen;
				immLen = 0;
			}
			
			//printf( "ref %d, %d\n", -( mOffset + 1 ), maxMatch + 1 );
			n += maxMatch + 1;
			
			osz += 2;
			
		} else {
			immLen++;
			n++;
		}
	}
	
	if( immLen > 0 ) {
		//printf( "imm %d\n", immLen );
		osz += 2 + immLen;
	}
	
	printf( "LZSS %d -> %d\n", sz, osz ); fflush( stdout );
}
