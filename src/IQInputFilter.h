/* Filter taps in time-domain */

#ifndef _IQINPUTFILTER_H_
#define _IQINPUTFILTER_H_

#define NUM_TAPS_IQ_INPUT_FILT        101
#define IQ_INP_HIL_FILT_DELAY         50

/* Low pass prototype for Hilbert-filter */
extern const float fHilFiltIQ[NUM_TAPS_IQ_INPUT_FILT];

#endif /* _IQINPUTFILTER_H_ */
