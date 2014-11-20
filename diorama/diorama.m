%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  University of Kaiserslautern, Institute of Communications Engineering     %
%%  Copyright (C) 2005 Andreas Dittrich, Torsten Schorr                       %
%%                                                                            %
%%  Author(s)    : Andreas Dittrich (dittrich@eit.uni-kl.de)                  %
%%                 Torsten Schorr (schorr@eit.uni-kl.de)                      %
%%  Project start: 12.03.2004                                                 %
%%  Last change  : 16.03.2005                                                 %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  This program is free software; you can redistribute it and/or modify      %
%%  it under the terms of the GNU General Public License as published by      %
%%  the Free Software Foundation; either version 2 of the License, or         %
%%  (at your option) any later version.                                       %
%%                                                                            %
%%  This program is distributed in the hope that it will be useful,           %
%%  but WITHOUT ANY WARRANTY; without even the implied warranty of            %
%%  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             %
%%  GNU General Public License for more details.                              %
%%                                                                            %
%%  You should have received a copy of the GNU General Public License         %
%%  along with this program; if not, write to the Free Software               %
%%  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  diorama.m                                                                 %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  Description:                                                              %
%%  diorama is a complete Digital Radio Mondiale (DRM) receiver for Matlab    %
%%  allowing real time decoding and playback of the radio signal              %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function diorama( varargin )

%*******************************************************************************
%* constant global variables                                                   *
%*******************************************************************************
INPUT_SOURCE_SOUNDCARD=0; 
INPUT_SOURCE_FILE=1;

RUN_COMMAND_NONE = 0; 
RUN_COMMAND_RESTART = -1; 
RUN_COMMAND_QUIT = 99; 

RUN_STATE_POWERON = -2; 
RUN_STATE_INIT = -1; 
RUN_STATE_FIRSTRUN = 0; 
RUN_STATE_NORMAL = 1; 
RUN_STATE_LASTRUN = 99;
RUN_STATE_STOP = 100;


%*******************************************************************************
%* global variables                                                            *
%*******************************************************************************
global run_command;	% 0 = none, -1 = restart, 99 = quit
global run_state;	% -2 = power on, -1 = init, 0 = first normal run, 1 = normal run, 99 = last run
global matlab_version;

global robustness_mode;	% -1 or 0 = unknown, 1 = mode A, 2 = mode B ...
global spectrum_occupancy;	% -1 = unknown, 0..5
global do_synchronize;
global is_frame_sync;
global fac_valid;
global is_time_sync;
global sdc_data_valid;
global samplerate_offset_estimation;

% input_data_reading --> demodulation_and_equalization
global input_samples_buffer;	% twice the number of samples for a period of 400ms @ 48kHz sample-rate
global input_samples_buffer_writeptr; % has to be set by demodulation_and_equalization
global input_samples_buffer_request;	% has to be set by demodulation_and_equalization 

% demodulation_and_equalization --> channel_decoding
% worst case: robustness mode A, spectrum occupancy 5 (Kmin=-110, Kmax=350), 15 symbols per frame, 2 super-frames
global transmission_frame_buffer;	% is filled by demodulation_and_equalization
global transmission_frame_buffer_writeptr;	% has to be set by channel_decoding
global transmission_frame_buffer_data_valid;
global channel_transfer_function_buffer;	% is filled by demodulation_and_equalization
global K_dc;
global K_modulo;
global symbols_per_frame;
global symbol_period;
global mean_energy_of_used_cells_list;
global WMER_FAC

% demodulation_and_equalization --> output_data_writing
global smp_rate_conv_in_phase_offset;		% fractional part of input delay in samples
global smp_rate_conv_in_out_delay;			% integer part of input/output delay in samples
global r_record;

% channel_decoding --> source_decoding
global channel_decoded_data_buffer;
global channel_decoded_data_buffer_data_valid;

% channel_decoding --> source_decoding --> output_data_writing
global multiplex_description
global application_information
global audio_information
global stream_information
global time_and_date

global text_message

% source_decoding --> output_data_writing
global output_samples_buffer;	% fixed size data block
global output_samples_buffer_data_valid;
global output_sampling_rate
global channels
global num_frames
global samples_per_audioframe

global gui_parameters
global nongui_parameters

%*******************************************************************************
%* initialization                                                              *
%*******************************************************************************


run_command = RUN_COMMAND_NONE;	% 0 = none, -1 = restart, 99 = quit
run_state = RUN_STATE_POWERON;	% -2 = power on, -1 = init, 0 = first normal run, 1 = normal run, 99 = last run
matlab_version=[sscanf(version,'%f')];

robustness_mode = 0;	% -1 or 0 = unknown, 1 = mode A, 2 = mode B ...
spectrum_occupancy = -1;	% -1 = unknown, 0..5
do_synchronize = 1;
is_frame_sync = 0;
fac_valid = -1;

input_samples_buffer = zeros( 400*48*2, 1);	% twice the number of samples for a period of 400ms @ 48kHz sample-rate
input_samples_buffer_writeptr = []; % has to be set by demodulation_and_equalization
input_samples_buffer_request = [];	% has to be set by demodulation_and_equalization 

transmission_frame_buffer = zeros( (110 + 350 + 1)*15*2*3, 1 );	% is filled by demodulation_and_equalization
transmission_frame_buffer_writeptr = [];	% has to be set by channel_decoding
transmission_frame_buffer_data_valid = [];
channel_transfer_function_buffer = zeros( (110 + 350 + 1)*15*2*3, 1 );	% is filled by demodulation_and_equalization
K_dc = [];
K_modulo = [];

% demodulation_and_equalization --> output_data_writing
smp_rate_conv_in_phase_offset = [];		% fractional part of input delay in samples
smp_rate_conv_in_out_delay = [];			% integer part of input/output delay in samples

% channel_decoding --> source_decoding
channel_decoded_data_buffer = zeros(28788,1);
channel_decoded_data_buffer_data_valid = [];

% source_decoding --> output_data_writing
output_samples_buffer = int16(zeros(2, 400*48*2));	% fixed size data block
output_samples_buffer_data_valid = [];


%*******************************************************************************
%* main processing loop                                                        *
%*******************************************************************************

settings(2);

ENABLE_GUI = settings_handler(6,4);

if (ENABLE_GUI)

    graphics_toolkit qt;
    
    if (isempty(findobj('Tag','radio_gui')))
        radio_gui;
        settings;
    else
        settings_handler(2);
    end
    set(findobj('Tag','stop_pushbutton'),'String','Stop');  
    set(findobj('Tag','stop_pushbutton'),'Callback','if (exist(''settings_handler.m'')),settings_handler(1, 23, gcbo, get(gcbo,''Value''));end');

end

UTILS_PATH = settings_handler(6,5);
path(UTILS_PATH,path);


eval('message(''clear'')'); toctic;
loop_counter = 0;	% free running counter for the outer loop
while	( run_state<99 )
   loop_counter = loop_counter + 1;
   drawnow;
   gui_parameters = settings_handler(9);
   nongui_parameters = settings_handler(10);
   
   
   if ( run_command ~= RUN_COMMAND_NONE )
      run_state = run_command;
      run_command = RUN_COMMAND_NONE;
   end
   
   input_data_reading;
   demodulation_and_equalization; 
   channel_decoding;
   source_decoding;
   output_data_writing;
   
   message('flush');
   
   SHOW_SIGNAL_INFO = settings_handler(4,20);
   
   if (SHOW_SIGNAL_INFO)
        show_signal_info(1);
   end
   show_signal_info(2,47,1);
   
   if ( run_state < RUN_STATE_NORMAL )
      run_state = run_state + 1;
   end
   
end
