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

#include "IIR_filter.h"

#include <stdlib.h> // For malloc/free
#include <string.h> // For memset

float iirfilter_coefficients[20] = 
{
// Scaled for floating point. Bandpass coefficients for Chebycheff type II between 0.5-3 Hz. Stopband = -60 dB
		0.035387811875614, -0.070378164881857, 0.035387811875614, 1.9902057141500744, -0.9904908197439197,// b0, b1, b2, a1, a2
		0.0625, -0.12484429058965466, 0.06249999999999999, 1.9966864658124233, -0.9968986788726024,// b0, b1, b2, a1, a2
		1, -1.9999878334825596, 1, 1.9872551780393801, -0.9877305090708219,// b0, b1, b2, a1, a2
		1, -1.9999450312503175, 1.0000000000000002, 1.9939627151917723, -0.9946055733182634// b0, b1, b2, a1, a2

};


iirfilter *iirfilter_create( void )
{
	iirfilter *result = (iirfilter *)malloc( sizeof( iirfilter ) );	// Allocate memory for the object
	iirfilter_init( result );											// Initialize it
	return result;																// Return the result
}

void iirfilter_destroy( iirfilter *pObject )
{
	free( pObject );
}

 void iirfilter_init( iirfilter * pThis )
{
	iirfilter_reset( pThis );

}

 void iirfilter_reset( iirfilter * pThis )
{
	memset( &pThis->state, 0, sizeof( pThis->state ) ); // Reset state to 0
	pThis->output = 0;					// Reset output

}

void irrfilter_soft_reset (iirfilter * pThis){
	pThis->output = 0;
}

 int iirfilter_filterBlock( iirfilter * pThis, float * pInput, float * pOutput, unsigned int count )
{
	iirfilter_executionState executionState;          // The executionState structure holds call data, minimizing stack reads and writes 
	if( ! count ) return 0;                         // If there are no input samples, return immediately
	executionState.pInput = pInput;                 // Pointers to the input and output buffers that each call to filterBiquad() will use
	executionState.pOutput = pOutput;               // - pInput and pOutput can be equal, allowing reuse of the same memory.
	executionState.count = count;                   // The number of samples to be processed
	executionState.pState = pThis->state;                   // Pointer to the biquad's internal state and coefficients. 
	executionState.pCoefficients = iirfilter_coefficients;    // Each call to filterBiquad() will advance pState and pCoefficients to the next biquad

	// The 1st call to iirfilter_filterBiquad() reads from the caller supplied input buffer and writes to the output buffer.
	// The remaining calls to filterBiquad() recycle the same output buffer, so that multiple intermediate buffers are not required.

	iirfilter_filterBiquad( &executionState);		// Run biquad #0
	executionState.pInput = executionState.pOutput;         // The remaining biquads will now re-use the same output buffer.

	iirfilter_filterBiquad( &executionState);		// Run biquad #1

	iirfilter_filterBiquad( &executionState);		// Run biquad #2

	iirfilter_filterBiquad( &executionState);		// Run biquad #3

	// At this point, the caller-supplied output buffer will contain the filtered samples and the input buffer will contain the unmodified input samples.  
	return count;		// Return the number of samples processed, the same as the number of input samples

}

 void iirfilter_filterBiquad( iirfilter_executionState * pExecState)
{
	// Read state variables
	float w0, x0;
	float w1 = pExecState->pState[0];
	float w2 = pExecState->pState[1];

	// Read coefficients into work registers
	float b0 = *(pExecState->pCoefficients++);
	float b1 = *(pExecState->pCoefficients++);
	float b2 = *(pExecState->pCoefficients++);
	float a1 = *(pExecState->pCoefficients++);
	float a2 = *(pExecState->pCoefficients++);

	// Read source and target pointers
	float *pInput  = pExecState->pInput;
	float *pOutput = pExecState->pOutput;
	short count = pExecState->count;
	float accumulator;

	// Loop for all samples in the input buffer
	while( count-- )
	{
		// Read input sample
		x0 = *(pInput++);
	
		// Run feedback part of filter
		accumulator  = w2 * a2;
		accumulator += w1 * a1;
		accumulator += x0 ;

		w0 = accumulator ;
	
		// Run feedforward part of filter
		accumulator  = w0 * b0;
		accumulator += w1 * b1;
		accumulator += w2 * b2;

		w2 = w1;		// Shuffle history buffer
		w1 = w0;

		// Write output
		*(pOutput++) = accumulator;
	}

	// Write state variables
	*(pExecState->pState++) = w1;
	*(pExecState->pState++) = w2;

}

int iirfilter_filterInChunks( iirfilter * pThis, float * pInput, float * pOutput, int length ){
 	int processedLength = 0;
	unsigned int chunkLength, outLength;
	static long random = 0x6428734; // Use pseudo-random number generator to split input into small random length chunks.
	while(length>0){
		chunkLength = random & 0xf;											// Choose random chunkLength from 0 - 15
		if( chunkLength > length ) chunkLength = length;					// Limit chunk length to the number of remaining samples
		outLength = iirfilter_filterBlock( pThis,  pInput, pOutput, chunkLength );		// Filter the block and determine the number of returned samples
		pOutput += outLength;												// Update the output pointer
		processedLength += outLength;										// Update the total number of samples output
		pInput += chunkLength;												// Update the input pointer
		length -= chunkLength;												// Update the number of samples remaining
		random = random + 0x834f4527;										// Cycle the simple random number generator
	}

	return processedLength;													// Return the number of samples processed

}



