#include <stdio.h>
#include <stdlib.h>

#include "types.h"

u32 arc( u8* n, u8 sz ) {
	s16 w[16] = { 0 };
	s16 wt = sz;
	
	for( int i = 0; i < sz; i++ ) {
		u8 s = n[i] & 0x0F;
		w[s]++;
	}
	
	for( int i = 0; i < 16; i++ ) {
		printf( "w%d  %d\n", i, w[i] );
	}
	
	s16 r[16][2] = { { 0 } };
	
	s16 m = 0;
	for( int i = 0; i < 16; i++ ) {
		if( w[i] > 0 ) {
			s16 l = m;
			m += w[i];
			s16 u = m;
			printf( "%d  %d-%d\n", i, l, u );
			r[i][0] = l;
			r[i][1] = u;
		}
	}
	
	s32 mn = 0;
	s32 mx = wt << ( sz * 2 );
	
	for( int i = 0; i < sz; i++ ) {
		s32 l = r[n[i]][0];
		s32 u = r[n[i]][1];
		s32 range = mx - mn;
		mn += l << ( sz - i ) * 2;
		mx = mn + ( ( u - l ) * range ) / wt;
		
		printf( "%d-%d\n", mn, mx );
	}
	
	return 0;
}

int main( int argc, char* argv[] ) {
	/*
	FILE* f = fopen( argv[1], "rb" );
	fseek( f, 0, SEEK_END );
	size_t sz = ftell( f );
	rewind( f );
	u8* buf = malloc( sz );
	fread( buf, sz, 1, f );
	fclose( f );
	
	
	
	free( buf );
	*/
	
	u8 data[8] = { 5, 3, 5, 5 };
	
	arc( data, 4 );
	
	return 0;
}
