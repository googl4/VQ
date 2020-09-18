#include <string.h>
#include <stdio.h>

#include "types.h"

#define RLE_MASK_TYPE    0x80
#define RLE_MASK_LEN     0x7F
#define RLE_TYPE_LITERAL 0x80
#define RLE_TYPE_SPAN    0x00

void rle( void* dst, int* dstLen, void* src, int srcLen, int wordSize ) {
	int outSize = 0;
	
	for( int i = 0; i < srcLen; /*i += wordSize*/ ) {
		u8 word[16];
		memcpy( word, src + i, wordSize );
		
		int matchLen = 1;
		int matchType = RLE_TYPE_LITERAL;
		
		u8 lastWord[16];
		memcpy( lastWord, word, wordSize );
		
		int dc = 0;
		
		for( int j = i + wordSize; j < srcLen + 127 * wordSize; j += wordSize ) {
			int eq = !memcmp( lastWord, src + j, wordSize );
			
			memcpy( lastWord, src + j, wordSize );
			
			if( matchLen == 1 ) {
				if( eq ) {
					matchType = RLE_TYPE_SPAN;
					
				} else {
					matchType = RLE_TYPE_LITERAL;
				}
				
			} else {
				if( matchType == RLE_TYPE_SPAN && !eq ) {
					break;
				}
				
				if( matchType == RLE_TYPE_LITERAL ) {
					if( eq ) {
						dc++;
						if( dc > 2 ) {
							matchLen -= dc;
							break;
						}
						
					} else {
						dc = 0;
					}
				}
			}
			
			matchLen++;
		}
		
		if( matchType == RLE_TYPE_LITERAL ) {
			//printf( "rle literal %d\n", matchLen );
			outSize += 1 + matchLen * wordSize;
		} else {
			//printf( "rle span %d\n", matchLen );
			outSize += 1 + wordSize;
		}
		
		i += matchLen * wordSize;
	}
	
	printf( "rle %d -> %d  ( %.1f%% )\n", srcLen, outSize, ( (float)outSize / srcLen ) * 100.0f );
}
