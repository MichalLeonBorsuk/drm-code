/* Automatically generated file with GNU Octave */

/* File name: "IQInputFilter.octave" */
/* Filter taps in time-domain */

#include "IQInputFilter.h"

/* Low pass prototype for Hilbert-filter */
const float fHilFiltIQ[NUM_TAPS_IQ_INPUT_FILT] =
{
	-3.736957051449150360e-06f,
	-1.949361188773379022e-03f,
	1.203244046696168841e-06f,
	-1.377938922973749505e-03f,
	-7.286442527055340922e-07f,
	-1.853062624290775263e-03f,
	-8.208564673110997449e-07f,
	-2.427196128945900352e-03f,
	-9.107156265071787189e-07f,
	-3.113364316704105191e-03f,
	-9.962980523727442819e-07f,
	-3.925998868143509325e-03f,
	-1.075640558807862836e-06f,
	-4.881296663153458608e-03f,
	-1.146799840353189473e-06f,
	-5.997740248464871610e-03f,
	-1.207908898204542781e-06f,
	-7.296844830686835537e-03f,
	-1.257215753211608818e-06f,
	-8.804233870279051799e-03f,
	-1.293138791114585299e-06f,
	-1.055120063798133632e-02f,
	-1.314295988998222096e-06f,
	-1.257700496665508164e-02f,
	-1.319551455844310813e-06f,
	-1.493231281675627339e-02f,
	-1.308060329496818497e-06f,
	-1.768446856730550992e-02f,
	-1.279281200862170237e-06f,
	-2.092581418178510524e-02f,
	-1.233001879515619054e-06f,
	-2.478729096468635859e-02f,
	-1.169359383011782991e-06f,
	-2.946166242553371548e-02f,
	-1.088842349510299921e-06f,
	-3.524532409956730034e-02f,
	-9.922747961742540110e-07f,
	-4.261870013437370902e-02f,
	-8.808166293594269658e-07f,
	-5.241431744885489963e-02f,
	-7.559565538325407336e-07f,
	-6.620892100050541906e-02f,
	-6.194455619809827864e-07f,
	-8.738768368296170874e-02f,
	-4.733006012279122596e-07f,
	-1.247623388759305785e-01f,
	-3.197412292910920716e-07f,
	-2.106614869578013749e-01f,
	-1.611402251605884152e-07f,
	-6.361033760689378536e-01f,
	0.000000000000000000e+00f,
	6.361033760689378536e-01f,
	1.611402251605884152e-07f,
	2.106614869578013749e-01f,
	3.197412292910920716e-07f,
	1.247623388759305785e-01f,
	4.733006012279122596e-07f,
	8.738768368296170874e-02f,
	6.194455619809827864e-07f,
	6.620892100050541906e-02f,
	7.559565538325407336e-07f,
	5.241431744885489963e-02f,
	8.808166293594269658e-07f,
	4.261870013437370902e-02f,
	9.922747961742540110e-07f,
	3.524532409956730034e-02f,
	1.088842349510299921e-06f,
	2.946166242553371548e-02f,
	1.169359383011782991e-06f,
	2.478729096468635859e-02f,
	1.233001879515619054e-06f,
	2.092581418178510524e-02f,
	1.279281200862170237e-06f,
	1.768446856730550992e-02f,
	1.308060329496818497e-06f,
	1.493231281675627339e-02f,
	1.319551455844310813e-06f,
	1.257700496665508164e-02f,
	1.314295988998222096e-06f,
	1.055120063798133632e-02f,
	1.293138791114585299e-06f,
	8.804233870279051799e-03f,
	1.257215753211608818e-06f,
	7.296844830686835537e-03f,
	1.207908898204542781e-06f,
	5.997740248464871610e-03f,
	1.146799840353189473e-06f,
	4.881296663153458608e-03f,
	1.075640558807862836e-06f,
	3.925998868143509325e-03f,
	9.962980523727442819e-07f,
	3.113364316704105191e-03f,
	9.107156265071787189e-07f,
	2.427196128945900352e-03f,
	8.208564673110997449e-07f,
	1.853062624290775263e-03f,
	7.286442527055340922e-07f,
	1.377938922973749505e-03f,
	-1.203244046696168841e-06f,
	1.949361188773379022e-03f,
	3.736957051449150360e-06f
};
