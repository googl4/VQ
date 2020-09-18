#include <string.h>
#include <stdio.h>
#include <limits.h>

#include "types.h"

typedef struct {
	int node;
	int a, b;
	int w;
} treeEntry_t;

int outBits = 0;

void printTree( treeEntry_t* tree, int index, int depth, int code ) {
	if( tree[index].node ) {
		printTree( tree, tree[index].a, depth + 1, code );
		printTree( tree, tree[index].b, depth + 1, code | ( 1 << depth ) );
		
	} else {
		/*
		printf( "%d, %d, ", tree[index].a, tree[index].w );
		for( int i = 0; i < depth; i++ ) {
			printf( "%d", ( code >> i ) & 1 );
		}
		printf( "\n" );
		*/
		outBits += depth * tree[index].w;
	}
}

void huff( void* dst, int* dstLen, void* src, int srcLen, int wordSize ) {
	treeEntry_t tree[4096];
	int treeLen = 0;
	
	/*
	for( int i = 0; i < 256; i++ ) {
		tree[i].node = FALSE;
		tree[i].a = i;
		tree[i].w = 0;
	}
	*/
	
	for( int i = 0; i < srcLen; i++ ) {
		int n = ((u8*)src)[i];
		
		int exists = FALSE;
		for( int t = 0; t < treeLen; t++ ) {
			if( !tree[t].node && tree[t].a == n ) {
				tree[t].w += 1;
				exists = TRUE;
				break;
			}
		}
		
		if( !exists ) {
			tree[treeLen].node = FALSE;
			tree[treeLen].a = n;
			tree[treeLen].w = 1;
			treeLen++;
		}
	}
	
	/*
	for( int i = 0; i < 256; i++ ) {
		if( tree[i].w > 0 ) {
			printf( "%d : %d\n", i, tree[i].w );		
		}
	}
	
	fflush( stdout );
	*/
	
	int coded[4096];
	memset( coded, 0, 4096 * 4 );
	
	for( int i = 0; i < 4096; i++ ) {
		int n1 = -1;
		int w1 = INT_MAX;
		int n2 = -1;
		int w2 = INT_MAX;
		
		for( int j = 0; j < treeLen; j++ ) {
			if( coded[j] ) {
				continue;
			}
			
			if( tree[j].w == 0 ) {
				continue;
			}
			
			if( tree[j].w < w1 ) {
				n2 = n1;
				w2 = w1;
				n1 = j;
				w1 = tree[j].w;
				
			} else {
				if( tree[j].w < w2 ) {
					n2 = j;
					w2 = tree[j].w;
				}
			}
		}
		
		if( n1 < 0 || n2 < 0 ) {
			break;
		}
		
		tree[treeLen].node = TRUE;
		tree[treeLen].a = n1;
		tree[treeLen].b = n2;
		tree[treeLen].w = w1 + w2;
		treeLen++;
		
		//printf( "%d : %d\n", n1, n2 );
		
		coded[n1] = TRUE;
		coded[n2] = TRUE;
	}
	
	//printf( "%d\n", treeLen );
	outBits = 0;
	printTree( tree, treeLen - 1, 0, 0 );
	printf( "coded frame: %d bytes\n", ( outBits + 7 ) / 8 );
}
