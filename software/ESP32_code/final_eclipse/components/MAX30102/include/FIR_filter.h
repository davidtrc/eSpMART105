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

// Begin header file, firfilter.h

#ifndef FIR_FILTER_H_ // Include guards
#define FIR_FILTER_H_

static const int firfilter_length = 24;
extern float firfilter_coefficients[24];

typedef struct
{
	float * pointer;
	float state[48];
	float output;
} firfilter;


firfilter *firfilter_create( void );
void firfilter_destroy( firfilter *pObject );
 void firfilter_init( firfilter * pThis );
 void firfilter_reset( firfilter * pThis );
 int firfilter_filterInChunks( firfilter * pThis, float * pInput, float * pOutput, int length );
#define firfilter_writeInput( pThis, input )  \
	firfilter_filterBlock( pThis, &input, &pThis->output, 1 );

#define firfilter_readOutput( pThis )  \
	pThis->output

 int firfilter_filterBlock( firfilter * pThis, float * pInput, float * pOutput, unsigned int count );
#define firfilter_outputToFloat( output )  \
	(output)

#define firfilter_inputFromFloat( input )  \
	(input)

 void firfilter_dotProduct( float * pInput, float * pKernel, float * pAccumulator, short count );
#endif // FILTER1_H_
	
