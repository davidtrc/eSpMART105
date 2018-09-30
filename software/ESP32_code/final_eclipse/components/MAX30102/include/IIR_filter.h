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

// Begin header file, IIR_FILTER.h

#ifndef IIR_FILTER_H_ // Include guards
#define IIR_FILTER_H_

static const int iirfilter_numStages = 4;
static const int iirfilter_coefficientLength = 20;
extern float iirfilter_coefficients[20];

typedef struct
{
	float state[16];
	float output;
} iirfilter;

typedef struct
{
	float *pInput;
	float *pOutput;
	float *pState;
	float *pCoefficients;
	short count;
} iirfilter_executionState;


iirfilter *iirfilter_create( void );
void iirfilter_destroy( iirfilter *pObject );
 void iirfilter_init( iirfilter * pThis );
 void iirfilter_reset( iirfilter * pThis );
 void irrfilter_soft_reset (iirfilter * pThis);
 int iirfilter_filterInChunks( iirfilter * pThis, float * pInput, float * pOutput, int length );
#define iirfilter_writeInput( pThis, input )  \
	iirfilter_filterBlock( pThis, &input, &pThis->output, 1 );

#define iirfilter_readOutput( pThis )  \
	pThis->output

 int iirfilter_filterBlock( iirfilter * pThis, float * pInput, float * pOutput, unsigned int count );
#define iirfilter_outputToFloat( output )  \
	(output)

#define iirfilter_inputFromFloat( input )  \
	(input)

 void iirfilter_filterBiquad( iirfilter_executionState * pExecState );
#endif // FILTER1_H_
	
