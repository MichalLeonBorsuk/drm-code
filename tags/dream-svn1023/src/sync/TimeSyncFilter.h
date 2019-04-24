/* Automatically generated file with GNU Octave */
/* File name: "TimeSyncFilter.m" */
/* Filter taps in time-domain */

#ifndef _TIMESYNCFILTER_H_
#define _TIMESYNCFILTER_H_

/* Filter bandwidths */
enum EHilbFiltBw { HILB_FILT_BNDWIDTH_5 = 5200, HILB_FILT_BNDWIDTH_10 = 10200, HILB_FILT_BNDWIDTH_50 = 50200 };

/* Filter parameters for 24000 Hz sample rate */
#define NUM_TAPS_HILB_FILT_5_24            41
/* Low pass prototype for Hilbert-filter 5 kHz bandwidth */
extern const float fHilLPProt5_24[NUM_TAPS_HILB_FILT_5_24];

#define NUM_TAPS_HILB_FILT_10_24            41
/* Low pass prototype for Hilbert-filter 10 kHz bandwidth */
extern const float fHilLPProt10_24[NUM_TAPS_HILB_FILT_10_24];

/* Filter parameters for 48000 Hz sample rate */
#define NUM_TAPS_HILB_FILT_5_48            81
/* Low pass prototype for Hilbert-filter 5 kHz bandwidth */
extern const float fHilLPProt5_48[NUM_TAPS_HILB_FILT_5_48];

#define NUM_TAPS_HILB_FILT_10_48            81
/* Low pass prototype for Hilbert-filter 10 kHz bandwidth */
extern const float fHilLPProt10_48[NUM_TAPS_HILB_FILT_10_48];

/* Filter parameters for 96000 Hz sample rate */
#define NUM_TAPS_HILB_FILT_5_96            161
/* Low pass prototype for Hilbert-filter 5 kHz bandwidth */
extern const float fHilLPProt5_96[NUM_TAPS_HILB_FILT_5_96];

#define NUM_TAPS_HILB_FILT_10_96            161
/* Low pass prototype for Hilbert-filter 10 kHz bandwidth */
extern const float fHilLPProt10_96[NUM_TAPS_HILB_FILT_10_96];

/* Filter parameters for 192000 Hz sample rate */
#define NUM_TAPS_HILB_FILT_5_192            321
/* Low pass prototype for Hilbert-filter 5 kHz bandwidth */
extern const float fHilLPProt5_192[NUM_TAPS_HILB_FILT_5_192];

#define NUM_TAPS_HILB_FILT_10_192            321
/* Low pass prototype for Hilbert-filter 10 kHz bandwidth */
extern const float fHilLPProt10_192[NUM_TAPS_HILB_FILT_10_192];

#define NUM_TAPS_HILB_FILT_50_192            321
/* Low pass prototype for Hilbert-filter 50 kHz bandwidth */
extern const float fHilLPProt50_192[NUM_TAPS_HILB_FILT_50_192];

/* Filter parameters for 384000 Hz sample rate */
#define NUM_TAPS_HILB_FILT_5_384            641
/* Low pass prototype for Hilbert-filter 5 kHz bandwidth */
extern const float fHilLPProt5_384[NUM_TAPS_HILB_FILT_5_384];

#define NUM_TAPS_HILB_FILT_10_384            641
/* Low pass prototype for Hilbert-filter 10 kHz bandwidth */
extern const float fHilLPProt10_384[NUM_TAPS_HILB_FILT_10_384];

#define NUM_TAPS_HILB_FILT_50_384            641
/* Low pass prototype for Hilbert-filter 50 kHz bandwidth */
extern const float fHilLPProt50_384[NUM_TAPS_HILB_FILT_50_384];

#endif	/* _TIMESYNCFILTER_H_ */
