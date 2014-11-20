function settings(varargin)
%*******************************************************************************
%* settings                                                                    *
%*******************************************************************************

%%%% WORKARROUND, Begin %%%%%
global TIME_SYNC_ENABLE FREQ_SYNC_ENABLE SRC_ENABLE
TIME_SYNC_ENABLE = 1;
FREQ_SYNC_ENABLE = 1;
SRC_ENABLE = 1;	%Sample Rate Conversion
%%%% WORKARROUND, End %%%%%

DEBUG = 0;
VERBOSE_LEVEL = 0;
PRINTTIME = 1;
PLOT_INPUT_SPECTRUM = 0;
PLOT_SYNCHRONISATION = 0;
PLOT_CONSTELLATIONS = 0;
PLOT_CHANNELESTIMATION = 0;
PLOT_SNRESTIMATION = 0;
PLOT_SNR_SPECTRUM = 0;
FLIP_SPECTRUM = 0;
EQUALIZATION = 1;

CHANNELDECODING = 1;
SOURCEDECODING = 1;
PLAYWAV = 1;
WRITEWAV = 0;
SNR_MAX_DB = 30;
SAMPLERATE_OFFSET_INIT_PPM = 0;
SOFT_MSD = 0;
MSD_ITER = 2;
ENABLE_SERVICE1 = 1;
ENABLE_SERVICE2 = 1;
ENABLE_SERVICE3 = 1;
ENABLE_SERVICE4 = 1;
DATA_STORAGE_PATH = [pwd,filesep,'data'];
ENABLE_GUI = 1;
UTILS_PATH = [pwd,filesep,'utils'];
RECORD_INPUT = 0;
JINGLE_BEGIN_FILENAME = 'sounds/blurb_24kHz.wav';
JINGLE_END_FILENAME = 'sounds/schluerf_400ms_24kHz_mono.wav';
CONVENIENCE_NOISE_FILENAME = 'sounds/noise_filtered_48kHz_mono.wav';
SHOW_SIGNAL_INFO = 1;
SHOW_ABOUT = 0;
STORE_NEWS = 1;

% general
INPUT_SOURCE_SOUNDCARD=0; INPUT_SOURCE_FILE=1; INPUT_SOURCE_TESTSIGNAL=2;

input_source = INPUT_SOURCE_SOUNDCARD;
input_filename = '';

offset_smp = 10000;


% settings(1): only GUI variables; settings(2): only non-GUI variables;
% settings: all variables

if (nargin == 0) | (varargin{1} == 1)
    
    settings_handler (3, {  PLOT_INPUT_SPECTRUM, PLOT_SYNCHRONISATION, PLOT_CONSTELLATIONS, PLOT_CHANNELESTIMATION, ...
                            PLOT_SNR_SPECTRUM, FLIP_SPECTRUM, WRITEWAV, PLAYWAV, ENABLE_SERVICE1, ENABLE_SERVICE2, ...
                            ENABLE_SERVICE3, ENABLE_SERVICE4, input_source, 3 - VERBOSE_LEVEL, 5 - MSD_ITER, EQUALIZATION, ...
                            CHANNELDECODING, SOURCEDECODING, RECORD_INPUT, SHOW_SIGNAL_INFO, STORE_NEWS, 0, 0, ...
                            SHOW_ABOUT, 0, 0, input_filename, DATA_STORAGE_PATH, 0, 1, 'N/A'});
end

if (nargin == 0) | (varargin{1} == 2)
    
    settings_handler (5, {  SNR_MAX_DB, SAMPLERATE_OFFSET_INIT_PPM, SOFT_MSD, ENABLE_GUI, UTILS_PATH, ...
                            JINGLE_BEGIN_FILENAME, JINGLE_END_FILENAME, CONVENIENCE_NOISE_FILENAME, ...
                            DEBUG, PRINTTIME, offset_smp});
                
end