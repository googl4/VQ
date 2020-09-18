#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "hf.h"

int main( int argc, char* argv[] ) {
	FILE* f = fopen( argv[1], "rb" );
	fseek( f, 0, SEEK_END );
	size_t sz = ftell( f );
	rewind( f );
	u8* buf = malloc( sz );
	fread( buf, sz, 1, f );
	fclose( f );
	
	huff( NULL, NULL, buf, sz, 2 );
	
	free( buf );
	
	return 0;
}
