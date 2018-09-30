/******************************* SOURCE LICENSE *********************************
Copyright (c) 2015 MicroModeler.

A non-exclusive, nontransferable, perpetual, royalty-free license is granted to the Licensee to 
use the following Information for academic, non-profit, or government-sponsored research purposes.
Use of the following Information under this License is restricted to NON-COMMERCIAL PURPOSES ONLY.
Commercial use of the following Information requires a separately executed written license agreement.

This Information is distributed WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

******************************* END OF LICENSE *********************************/

// A commercial license for MicroModeler DSP can be obtained at http://www.micromodeler.com/launch.jsp

#include "FIR_filter.h"

#include <stdlib.h> // For malloc/free
#include <string.h> // For memset

float firfilter_coefficients[24] = {
	0.0000000, 0.0000000, 0.0000000, -0.14919920, 0.023089438, -0.022157961,
	-0.020423685, -0.025641040, -0.062339239, -0.090148337, -0.049975705, 0.079723994,
	0.24088605, 0.31598652, 0.24088605, 0.079723994, -0.049975705, -0.090148337,
	-0.062339239, -0.025641040, -0.020423685, -0.022157961, 0.023089438, -0.14919920
};


firfilter *firfilter_create( void ){
	firfilter *result = (firfilter *)malloc( sizeof( firfilter ) );	// Allocate memory for the object
	firfilter_init( result );											// Initialize it
	return result;																// Return the result
}

void firfilter_destroy( firfilter *pObject ){
	free( pObject );
}

void firfilter_init( firfilter * pThis ){
	firfilter_reset( pThis );

}

void firfilter_reset( firfilter * pThis ){
	memset( &pThis->state, 0, sizeof( pThis->state ) ); // Reset state to 0
	pThis->pointer = pThis->state;						// History buffer points to start of state buffer
	pThis->output = 0;									// Reset output

}

int firfilter_filterBlock( firfilter * pThis, float * pInput, float * pOutput, unsigned int count ){
	float *pOriginalOutput = pOutput;	// Save original output so we can track the number of samples processed
	float accumulator;
 
 	for( ;count; --count )
 	{
 		pThis->pointer[firfilter_length] = *pInput;						// Copy sample to top of history buffer
 		*(pThis->pointer++) = *(pInput++);										// Copy sample to bottom of history buffer

		if( pThis->pointer >= pThis->state + firfilter_length )				// Handle wrap-around
			pThis->pointer -= firfilter_length;
		
		accumulator = 0;
		firfilter_dotProduct( pThis->pointer, firfilter_coefficients, &accumulator, firfilter_length );
		*(pOutput++) = accumulator;	// Store the result
 	}
 
	return pOutput - pOriginalOutput;
}

void firfilter_dotProduct( float * pInput, float * pKernel, float * pAccumulator, short count ){
 	float accumulator = *pAccumulator;
	while( count-- )
		accumulator += ((float)*(pKernel++)) * *(pInput++);
	*pAccumulator = accumulator;
}

int firfilter_filterInChunks( firfilter * pThis, float * pInput, float * pOutput, int length ){
 	int processedLength = 0;
	unsigned int chunkLength, outLength;
	static long random = 0x6428734; // Use pseudo-random number generator to split input into small random length chunks.
	while( length > 0 )
	{
		chunkLength = random & 0xf;											// Choose random chunkLength from 0 - 15
		if( chunkLength > length ) chunkLength = length;					// Limit chunk length to the number of remaining samples
		outLength = firfilter_filterBlock( pThis,  pInput, pOutput, chunkLength );		// Filter the block and determine the number of returned samples
		pOutput += outLength;												// Update the output pointer
		processedLength += outLength;										// Update the total number of samples output
		pInput += chunkLength;												// Update the input pointer
		length -= chunkLength;												// Update the number of samples remaining
		random = random + 0x834f4527;										// Cycle the simple random number generator
	}
	return processedLength;													// Return the number of samples processed
}