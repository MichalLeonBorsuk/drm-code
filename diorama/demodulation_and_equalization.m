%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  University of Kaiserslautern, Institute of Communications Engineering     %
%%  Copyright (C) 2004 Andreas Dittrich                                       %
%%                                                                            %
%%  Author(s)    : Andreas Dittrich (dittrich@eit.uni-kl.de)                  %
%%  Project start: 27.05.2004                                                 %
%%  Last change: 02.05.2005, 11:30                                            %
%%  Changes      : |                                                          %
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
%%  Last change: 02.05.2005, 11:30                                            %
%%               included show_signal_info                                    %
%%  By         : Andreas Dittrich                                             %
%%  Description: small changes because of modified calling arguments in       %
%%               get_ofdm_symbol_sync.m and plot_channelestimation.m          %
%%               fixed bug in spectrum-occupancy-estimation                   %
%%               display of sample-rate-offset and frequency instead of       %
%%               sample-rate-offset-correction / frequ.-off.-correction       %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  demodulation_and_equalization.m                                           %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  Description:                                                              %
%%  Demodulation, time and frequency synchronisation                          %
%%  channel estimation and equalization                                       %
%%                                                                            %
%%  To be used after input_data_reading.m                                     %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function demodulation_and_equalization()

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
global run_state;	% -2 = power on, -1 = init, 0 = first normal run, 1 = normal run, 99 = last run

% input_data_reading --> demodulation_and_equalization
global input_samples_buffer;	% twice the number of samples for a period of 400ms @ 48kHz sample-rate
global input_samples_buffer_writeptr; % has to be set by demodulation_and_equalization
global input_samples_buffer_request;	% has to be set by demodulation_and_equalization 

global robustness_mode;	% -1 or 0 = unknown, 1 = mode A, 2 = mode B ...
global spectrum_occupancy;	% -1 = unknown, 0..5
global do_synchronize;
global is_frame_sync;
global is_time_sync;
global fac_valid;
global samplerate_offset_estimation;
global WMER_FAC

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

% demodulation_and_equalization --> output_data_writing
global r_record;
global gui_parameters
global nongui_parameters

% demodulation_and_equalization --> input_data_writing
global smp_rate_conv_fft_phase_diff
global smp_rate_conv_fft_phase_offset

% demodulation_and_equalization --> input_data_writing, output_data_writing
global smp_rate_conv_in_out_delay;			% integer part of input/output delay in samples

%*******************************************************************************
%* static variables                                                            *
%*******************************************************************************
persistent N_symbols_mode_detection N_symbols_frequency_pilot_search SNR_time_frequ_sync ...
           SNR_mode_detection MIN_ABS_H SNR_MIN_DB SNR_TIMEOUT FAC_NOT_VALID_TIMEOUT ...
           robustness_mode_string_list h0 h1 h2 h3 rs_buffer rs_buffer_writeptr ...
           symbol_buffer symbol_buffer_writeptr time_offset_log_last symbol_counter ...
           Zi_filter_demodulate0 Zi_filter_demodulate1 Zi_filter_demodulate2 ...
           Zi_filter_demodulate3 Ts_list K_min_K_max_list equalization_init_ok ...
           gain_ref_cells_theta_1024_list time_ref_cells_k_list Tg_list frames_per_superframe ...
           time_ref_cells_theta_1024_list ...
           freq_ref_cells_theta_1024_list mode_and_occupancy_code_table y_list Tu_list ...
           gain_ref_cells_a_list symbols_per_2D_window_list W_pilots_int16_list ...
           gain_ref_cells_k_list symbols_per_frame_list W_symbol_int16_list ...
           gain_ref_cells_subset_list symbols_to_delay_list is_frequency_sync ...
           symbols_per_2D_window Ts time_offset_fractional_init ...
           freq_offset_init delta_time_offset_I_init Tu Zi_get_ofdm_symbol time_ref_cells_k ...
           time_ref_cells_theta_1024 symbols_to_delay spectrum_occupancy_estimation ...
           mode_and_occupancy_code_last y time_offset_fractional freq_offset ...
           delta_time_offset_I phi_freq_correction_last gain_ref_cells_per_y_symbols ...
           gain_ref_cells_subset gain_ref_cells_per_frame gain_ref_cells_k ...
           gain_ref_cells_theta_1024 gain_ref_cells_a next_pilots W_symbol_int16 ...
           W_pilots_int16 K_min K_max FAC_cells_k fac_not_valid_counter SNR_timeout_counter ...
           Tg
           
           
EQUALIZATION = gui_parameters{16};           
SHOW_SIGNAL_INFO = gui_parameters{20};
FLIP_SPECTRUM = gui_parameters{6};
PLOT_INPUT_SPECTRUM = gui_parameters{1};
PLOT_SYNCHRONISATION  = gui_parameters{2};
PLOT_CHANNELESTIMATION = gui_parameters{4};
PRINTTIME = nongui_parameters{10};
VERBOSE_LEVEL = 3 - gui_parameters{14};
input_source = gui_parameters{13};


%*******************************************************************************
% check state and do initialization if required                                *
%*******************************************************************************
if ( run_state == RUN_STATE_POWERON)
   % aquisition parameter
	N_symbols_mode_detection = 20;
	N_symbols_frequency_pilot_search = 15;	% no of symbols used for detection of frequency pilots
	SNR_time_frequ_sync = 10^(15/10);	% assumed SNR for time and frequency sync, here: 15dB
   SNR_mode_detection = 10^(15/10);	% assumed SNR for mode detection, here: 15dB
   MIN_ABS_H = (8e-5)^2;
	SNR_MIN_DB = 3;
   SNR_TIMEOUT = 3;	% no of frames with SNR<SNR_MIN_DB to force a new synchronization
   FAC_NOT_VALID_TIMEOUT = 4;% no of consecutive frames with invalid FAC to force a new synchronization
   
   robustness_mode_string_list = {'n/a', 'A','B','C','D'};
   
   % load fixed data
   
   if (isempty(equalization_init_ok))
       
       values = equalization_init;
       
       [Ts_list,Tu_list,Tg_list,symbols_per_frame_list,frames_per_superframe,K_min_K_max_list, ...
        time_ref_cells_k_list,time_ref_cells_theta_1024_list,freq_ref_cells_theta_1024_list,y_list, ...
        symbols_per_2D_window_list,symbols_to_delay_list,gain_ref_cells_k_list,gain_ref_cells_theta_1024_list, ...
        gain_ref_cells_a_list,W_symbol_int16_list,W_pilots_int16_list,mean_energy_of_used_cells_list, ...
        mode_and_occupancy_code_table,gain_ref_cells_subset_list,equalization_init_ok] = deal (values{:});
 
       
   end
   
   % filter designed with "fdatool"
   h_low_1 = [-5.938233e-002, -7.324223e-002, -3.237394e-002, -9.931939e-003, 8.455106e-002, 1.492561e-001, 2.305222e-001, 2.394004e-001, 2.305222e-001, 1.492561e-001, 8.455106e-002, -9.931939e-003, -3.237394e-002, -7.324223e-002, -5.938233e-002];
   h_low_2 = [3.575090e-002, 1.602849e-001, 3.040085e-001, 3.040085e-001, 1.602849e-001, 3.575090e-002];
   h_low_3 = [0.25,0,0.5,0,0.25];  %shifted DC-notch
   h_lowpass = conv( conv(h_low_1,h_low_2),h_low_3 );

	% create polyphase filter
	h0 = h_lowpass(1:4:end);
	h1 = h_lowpass(2:4:end);
	h2 = h_lowpass(3:4:end);
   h3 = h_lowpass(4:4:end);
   
   % our internal processing buffers: 
   %complex samples
   rs_buffer = zeros(1,320*(2*15+2));
   rs_buffer_writeptr = 1;	% 
   %ofdm cells
   symbol_buffer = zeros(1,288*2*15);
   symbol_buffer_writeptr = 1;
   
   time_offset_log_last = 0;
   symbol_counter=0;
   
   return;
elseif ( run_state == RUN_STATE_INIT )
   N_samples_needed = N_symbols_mode_detection*320;
   input_samples_buffer_request = 4*N_samples_needed;	%we request N_symbols_mode_detection mode-A symbols
   input_samples_buffer_writeptr = 1;
   SNR_timeout_counter = SNR_TIMEOUT;
   fac_not_valid_counter = FAC_NOT_VALID_TIMEOUT;
   mode_and_occupancy_code_last = -1;
   
	is_time_sync = 0;
	is_frequency_sync = 0;
   is_frame_sync = 0;
   
   % clear filter state
   Zi_filter_demodulate0 = zeros( length(h0)-1, 1 );
   Zi_filter_demodulate1 = zeros( length(h1)-1, 1 );
   Zi_filter_demodulate2 = zeros( length(h2)-1, 1 );
   Zi_filter_demodulate3 = zeros( length(h3)-1, 1 );
   
   % reset internal buffer
   rs_buffer_writeptr = 1;
   symbol_buffer_writeptr = 1;
   
   samplerate_offset_estimation = 0;
   smp_rate_conv_fft_phase_diff = 0;	% is required by input_data_reading
   smp_rate_conv_fft_phase_offset = 0;	% is required by input_data_reading and output_data_writing
   
   transmission_frame_buffer_data_valid = 0;
   
   show_signal_info(2,10,0);  %input rms
   show_signal_info(2,21, 0);   %time sync indicator
   show_signal_info(2,20, 0);   %frame sync indicator
   show_signal_info( 2, 11, NaN );	%frequ-offset
   show_signal_info( 2, 12, NaN );	%sample-rate-offset
   show_signal_info( 2, 13, NaN );	%FAC MER
   show_signal_info( 2, 14, NaN );	%FAC WMER
   
   return;
end

transmission_frame_buffer_data_valid = 0;

if (fac_valid == 0)
   fac_not_valid_counter = fac_not_valid_counter - 1;
   if ( fac_not_valid_counter<=0 )
      do_synchronize = 1;
      fac_not_valid_counter = FAC_NOT_VALID_TIMEOUT;
   end
elseif (fac_valid == 1)
   fac_not_valid_counter = FAC_NOT_VALID_TIMEOUT;
end


if (do_synchronize)
	is_time_sync = 0;
	is_frequency_sync = 0;
   is_frame_sync = 0;
   
   symbol_buffer_writeptr = 1;
	smp_rate_conv_in_out_delay = 0;
   samplerate_offset_estimation = 0;
   smp_rate_conv_fft_phase_diff = 0;	% is required by input_data_reading
   smp_rate_conv_fft_phase_offset = 0;	% is required by input_data_reading and output_data_writing
   
   do_synchronize = 0;
   
   show_signal_info(2,21, 0);   %time sync indicator
   show_signal_info(2,20, 0);   %frame sync indicator
   show_signal_info( 2, 11, NaN );	%frequ-offset
   show_signal_info( 2, 12, NaN );	%sample-rate-offset
   show_signal_info( 2, 13, NaN );	%FAC MER
   show_signal_info( 2, 14, NaN );	%FAC WMER
end


%*******************************************************************************
% demodulate to baseband                                                       *
%*******************************************************************************

if ( input_samples_buffer_writeptr>1 )
   
   % calculate some statistical info
   input_mean =  sum(input_samples_buffer(1:(input_samples_buffer_writeptr-1)))/input_samples_buffer_writeptr;
   input_rms = sqrt( sum(input_samples_buffer(1:(input_samples_buffer_writeptr-1)).^2)/input_samples_buffer_writeptr - input_mean^2 );
   
  	% demodulate, filter, subsample by 4
	[rs0, Zi_filter_demodulate0] = filter(h0, 1, input_samples_buffer(4:4:(input_samples_buffer_writeptr-1))', Zi_filter_demodulate0);
	[rs1, Zi_filter_demodulate1] = filter(h1, 1, input_samples_buffer(3:4:(input_samples_buffer_writeptr-1))', Zi_filter_demodulate1);
	[rs2, Zi_filter_demodulate2] = filter(h2, 1, input_samples_buffer(2:4:(input_samples_buffer_writeptr-1))', Zi_filter_demodulate2);
	[rs3, Zi_filter_demodulate3] = filter(h3, 1, input_samples_buffer(1:4:(input_samples_buffer_writeptr-1))', Zi_filter_demodulate3);
   
   length_rs0 = length(rs0);
   
   if (FLIP_SPECTRUM==1)
      rs_buffer(rs_buffer_writeptr-1+[1:length_rs0]) = [(rs0 - rs2) - j*(rs1 - rs3)];
   else
      rs_buffer(rs_buffer_writeptr-1+[1:length_rs0]) = [(rs0 - rs2) + j*(rs1 - rs3)];
   end
   
   rs_buffer_writeptr = rs_buffer_writeptr + length_rs0;
   
   if ( PLOT_INPUT_SPECTRUM )
      plot_input_spectrum(2, input_samples_buffer, input_samples_buffer_writeptr, rs_buffer, rs_buffer_writeptr, input_mean, input_rms);
   end
   
   input_samples_buffer_writeptr = 1;
	message(PRINTTIME<=VERBOSE_LEVEL, sprintf('%5.0fms - demodulation, filtering, subsampling, input mean: %5.2f, rms: %2.2f\n', toctic*1000, input_mean, input_rms)); 
	if (SHOW_SIGNAL_INFO)
      show_signal_info(2,10, input_rms);  %input rms
   end

end


%if equalization is deactivated, go out after this
if ( ~EQUALIZATION )
   N_samples_needed = N_symbols_mode_detection*320;
   input_samples_buffer_request = 4*N_samples_needed;	%we request N_symbols_mode_detection mode-A symbols
   input_samples_buffer_writeptr = 1;
   SNR_timeout_counter = SNR_TIMEOUT;
   fac_not_valid_counter = FAC_NOT_VALID_TIMEOUT;
   mode_and_occupancy_code_last = -1;
   
	do_synchronize = 1;
   
   % reset internal buffer
   rs_buffer_writeptr = 1;
   symbol_buffer_writeptr = 1;
   
   return;
end



%*******************************************************************************
% robustness mode detection, symbol-time-synchronisation,                      *
% fractional-frequency detection                                               *
%*******************************************************************************

if( ~is_time_sync )
   is_frequency_sync = 0;
	if (SHOW_SIGNAL_INFO)
      show_signal_info(2,21,3);	%time sync indicator -> pending (yellow)
	   show_signal_info(2,20,0);	%frame sync indicator -> neutral (transparent)
	end
   
   % do we have enough data for time synchronisation? 
   N_samples_needed = N_symbols_mode_detection*320 - rs_buffer_writeptr + 1;
   if ( N_samples_needed > 0 )
   	input_samples_buffer_request = N_samples_needed*4;
   	input_samples_buffer_writeptr = 1;
         
      return;
   end
   
   spectrum_occupancy = -1;	%set here to "unknown", so later spectrum_occupancy_estimation is used

	[robustness_mode, time_offset, samplerate_offset_estimation, frequency_offset_fractional_init] = get_robustness_mode( rs_buffer(1:(N_symbols_mode_detection*320)) );
   time_offset_integer = floor( time_offset + 0.5 );
   
	message(PRINTTIME<=VERBOSE_LEVEL,sprintf('%5.0fms - time sync & fractional frequ. sync, sample rate offset estimation: %.0f ppm, robustness_mode: %s\n', toctic * 1000, (1/(samplerate_offset_estimation+1)-1)*1e6, robustness_mode_string_list{robustness_mode+1} ));   

   if ( (abs(samplerate_offset_estimation)>200e-6) && (input_source==INPUT_SOURCE_SOUNDCARD) && (robustness_mode>0) )
      % samplerate offset is to big. We have to correct this in the recording module applying sample-rate-conversion
      % so, throw away all samples and get new one, which have been resampled
   	N_samples_needed = N_symbols_mode_detection*320;
   	input_samples_buffer_request = 4*N_samples_needed;	%we request N_symbols_mode_detection mode-A symbols
      input_samples_buffer_writeptr = 1;
      rs_buffer_writeptr = 1; 
      
      return;
   end
   
   if (robustness_mode>0) 
      is_time_sync = 1;
		if (SHOW_SIGNAL_INFO)
         show_signal_info(2,21,1);   %time sync indicator -> OK (green)
	   	show_signal_info(2,20,3);	 %frame sync indicator -> pending (yellow)
		end
      
      % set the appropriate parameter
      Ts = Ts_list(robustness_mode);
      Tu = Tu_list(robustness_mode);
      Tg = Tg_list(robustness_mode);
      
		Tgh = floor( Tg/2 + 0.5 ); %half guard period
      symbols_per_frame = symbols_per_frame_list(robustness_mode);
      
		K_dc = 1+Tu/2;	%this is the index, we want to place DC-carrier
		K_modulo = Tu; %index-distance between same carrier-cell of two consecutive symbols in our memory block
      
      time_ref_cells_k = time_ref_cells_k_list{robustness_mode};
		time_ref_cells_theta_1024 = time_ref_cells_theta_1024_list{robustness_mode};
      
		y = y_list(robustness_mode);	%vertical ofdm-symbol-distance between gain-ref-pilots
      symbols_per_2D_window = symbols_per_2D_window_list(robustness_mode);
	   symbols_to_delay = symbols_to_delay_list(robustness_mode);

      % symbol align rs_buffer
      rs_buffer_writeptr = rs_buffer_writeptr - time_offset_integer;
      rs_buffer(1:(rs_buffer_writeptr-1)) = rs_buffer( time_offset_integer + [1:(rs_buffer_writeptr-1)] );
      
   else
		if (SHOW_SIGNAL_INFO)
   		show_signal_info(2,21,2);	%time sync indicator -> bad (red)
	   	show_signal_info(2,20,0);	%frame sync indicator -> neutral (transparent)
		end
      
      % we cant do anything more now, so request new data and return
      samplerate_offset_estimation = 0;
      
      rs_buffer_writeptr = rs_buffer_writeptr - 320*N_symbols_mode_detection;
      rs_buffer(1:(rs_buffer_writeptr-1)) = rs_buffer( 320*N_symbols_mode_detection + [1:(rs_buffer_writeptr-1)] );
      
      N_samples_needed = N_symbols_mode_detection*320 - rs_buffer_writeptr + 1;
   	if ( N_samples_needed > 0 )
   		input_samples_buffer_request = N_samples_needed*4;
         input_samples_buffer_writeptr = 1;
      else
   		input_samples_buffer_request = 0;
      end
            
      return;
      
   end
end

if (SHOW_SIGNAL_INFO)
    show_signal_info(2,21,1);   %time sync indicator -> OK (green)
end

%*******************************************************************************
% integer frequency-offset-correction                                          *
%*******************************************************************************

if( ~is_frequency_sync )
  	is_frame_sync = 0;

   % do we have enough data for pilot search?
   N_samples_needed = (N_symbols_frequency_pilot_search+1)*Ts - (rs_buffer_writeptr - 1);
   if ( N_samples_needed > 0 )
   	input_samples_buffer_request = N_samples_needed*4;
   	input_samples_buffer_writeptr = 1;
         
      return;
   end
   
	Zi_get_ofdm_symbol = 0;
	delta_time_offset_integer = 0;
	freq_offset_init = frequency_offset_fractional_init;
	time_offset_fractional_init = time_offset - time_offset_integer;
	delta_time_offset_I_init = samplerate_offset_estimation*Ts;
   
   t_smp = 0;
	for ( s = [1:N_symbols_frequency_pilot_search] )   
   	[symbol_temp, delta_time_offset_integer, time_offset_fractional, freq_offset, delta_time_offset_I, Zi_get_ofdm_symbol] = get_ofdm_symbol( rs_buffer(t_smp+[1:2*Ts]), ...
   		time_offset_fractional_init, freq_offset_init, delta_time_offset_I_init, Ts, Tu, Zi_get_ofdm_symbol);
   
   	symbol_buffer( (s-1)*K_modulo + [1:Tu] ) = symbol_temp;
      t_smp = t_smp + Ts + delta_time_offset_integer;	% next symbol
   end
   
	freq_offset_integer = get_frequency_offset_integer( symbol_buffer, N_symbols_frequency_pilot_search, K_dc, K_modulo, Tu );
   %todo: do consistence/reliability-check 
   %todo: correct the last symbols and adjust freq-offset for only the following symbols
   %for now, reset to startvalues, set integer-frequency-correction and start loop again
   
	delta_time_offset_integer = 0;
   freq_offset_init = - freq_offset_integer + frequency_offset_fractional_init;
	time_offset_fractional_init = 0;
	Zi_get_ofdm_symbol = 0;
   
   % try to find out frequency occupancy: check power of K_min/K_max-carrier and neighbor
   %todo: skip this historical S_buffer stuff and do work directly on symbol_buffer
   S_buffer = reshape( symbol_buffer(1:N_symbols_frequency_pilot_search*K_modulo), K_modulo, N_symbols_frequency_pilot_search );
	S_buffer = S_buffer(1:Tu,1:N_symbols_frequency_pilot_search);

   spectrum_occupancy_indicator = zeros(6,1);
   for (spectrum_occupancy_index = [0:3])	% 4,5 not supported yet -> if so, we have to use doubled sampling rate
   %for (spectrum_occupancy_index = [0:5])
  		K_min_ = K_min_K_max_list(spectrum_occupancy_index*2 + (robustness_mode-1)*2*6 + 1);
      K_max_ = K_min_K_max_list(spectrum_occupancy_index*2 + (robustness_mode-1)*2*6 + 2);
        
      if (K_min_~=K_max_)
      	K_dc_indx = rem( floor(freq_offset_integer/2/pi + 0.5) + Tu/2, Tu ) + 1;
      	K_dc_plus2_indx = rem( K_dc_indx + 2 + Tu - 1, Tu ) + 1;
      	K_min_indx = rem( K_dc_indx + K_min_ + Tu - 1, Tu ) + 1;
      	K_min_minus4_indx = rem( K_min_indx -4 + Tu - 1, Tu ) + 1;	% for spectr.-occu. 0/1 K_min_minus1 equals the dc-carrier
      	K_max_indx = rem( K_dc_indx + K_max_ + Tu - 1, Tu ) + 1;
      	K_max_plus1_indx = rem( K_max_indx +1 + Tu - 1, Tu ) + 1;

			energy_ratio_K2_to_K0 = abs( S_buffer(K_dc_plus2_indx,:)*S_buffer(K_dc_plus2_indx,:)' ) ...
            / abs( S_buffer(K_dc_indx,:)*S_buffer(K_dc_indx,:)' );
			energy_ratio_K_max_to_K_max_p1 = abs( S_buffer(K_max_indx,:)*S_buffer(K_max_indx,:)' ) ...
            / abs( S_buffer(K_max_plus1_indx,:)*S_buffer(K_max_plus1_indx,:)' );
			energy_ratio_K_min_to_K_min_m4 = abs( S_buffer(K_min_indx,:)*S_buffer(K_min_indx,:)' ) ...
            / abs( S_buffer(K_min_minus4_indx,:)*S_buffer(K_min_minus4_indx,:)' );
         
         spectrum_occupancy_indicator(spectrum_occupancy_index+1) = (energy_ratio_K_min_to_K_min_m4 + energy_ratio_K_max_to_K_max_p1);
      else
         spectrum_occupancy_indicator(spectrum_occupancy_index+1) = 0;
      end
   end
   
   [dummy, temp] = max(spectrum_occupancy_indicator);
   spectrum_occupancy_estimation = temp - 1;
	is_frequency_sync = 1;   
	message(PRINTTIME<=VERBOSE_LEVEL,sprintf('%5.0fms - integer frequency synchronisation, spectrum_occupancy: %i\n', toctic  * 1000, spectrum_occupancy_estimation ));
   
end



%*******************************************************************************
% frame synchronization, if not already synced                                 *
%*******************************************************************************

if ( ~is_frame_sync )

	%do we have enough data in our symbol_buffer?
	N_symbols_needed = symbols_per_frame + symbols_per_2D_window - 1;
	N_samples_needed = (N_symbols_needed+1)*Ts - (rs_buffer_writeptr-1);	%one additional symbol because of delta_time_offset_integer correction
	if ( N_samples_needed > 0 )
		input_samples_buffer_request = N_samples_needed*4;
		input_samples_buffer_writeptr = 1;
		return;
	end

	t_smp = 0;
	for ( s = [1:symbols_per_frame] )
  		[symbol_temp, delta_time_offset_integer, time_offset_fractional, freq_offset, delta_time_offset_I, Zi_get_ofdm_symbol] = get_ofdm_symbol( rs_buffer(t_smp+[1:2*Ts]), ...
		   time_offset_fractional_init, freq_offset_init, delta_time_offset_I_init, Ts, Tu, Zi_get_ofdm_symbol);
   
   	symbol_buffer( (s-1)*K_modulo + [1:Tu] ) = symbol_temp;
   	t_smp = t_smp + Ts + delta_time_offset_integer;	% next symbol
      phi_freq_correction_last = Zi_get_ofdm_symbol(2);
	end

   % search first symbol of frame, by using time-reference-cells   
   symbol_0 = get_symbol_index( symbol_buffer, symbols_per_frame, time_ref_cells_k, time_ref_cells_theta_1024, K_dc, K_modulo );
   symbol_no_to_equalize = rem( symbol_0 - symbols_to_delay - 1 + symbols_per_frame, symbols_per_frame ) + 1;
   
   is_frame_sync = 1;
   symbol_counter = 0;
   
   message(PRINTTIME<=VERBOSE_LEVEL,sprintf('%5.0fms - frame synchronisation\n', toctic  * 1000 )); 

  	% frame align rs_buffer
   if (symbol_no_to_equalize~=1)
		rs_buffer_writeptr = rs_buffer_writeptr - (symbol_no_to_equalize-1)*Ts;
		rs_buffer(1:(rs_buffer_writeptr-1)) = rs_buffer( (symbol_no_to_equalize-1)*Ts + [1:(rs_buffer_writeptr-1)] );
   end
   
   symbol_buffer_writeptr=1;
   Zi_get_ofdm_symbol=0;
	t_smp = 0;
	for ( s = [1:symbols_per_2D_window] )
	   [symbol_temp, delta_time_offset_integer, time_offset_fractional, freq_offset, delta_time_offset_I, Zi_get_ofdm_symbol] = get_ofdm_symbol( rs_buffer(t_smp+[1:2*Ts]), ...
   		time_offset_fractional_init, freq_offset_init, delta_time_offset_I_init, Ts, Tu, Zi_get_ofdm_symbol);
   
   	symbol_buffer( (s-1)*K_modulo + [1:Tu] ) = symbol_temp;
   	symbol_buffer_writeptr = symbol_buffer_writeptr + 1;
      
      t_smp = t_smp + Ts + delta_time_offset_integer;	% next symbol
	   phi_freq_correction_last = Zi_get_ofdm_symbol(2);
	end
   
	% symbol align rs_buffer
	rs_buffer_writeptr = rs_buffer_writeptr - t_smp;
	rs_buffer(1:(rs_buffer_writeptr-1)) = rs_buffer( t_smp + [1:(rs_buffer_writeptr-1)] );
   
end

if (SHOW_SIGNAL_INFO)
    show_signal_info(2,20,1);   %time sync indicator -> OK (green)
end

%*******************************************************************************
% channel estimation based on pilots                                           *
%*******************************************************************************
% do we have enough data in our rs_buffer/symbol_buffer?
N_symbols_needed = symbols_per_frame + symbols_per_2D_window - (symbol_buffer_writeptr-1);
N_samples_needed = (N_symbols_needed+1)*Ts - (rs_buffer_writeptr-1);	%one additional symbol because of delta_time_offset_integer correction
if ( N_samples_needed > 0 )
	input_samples_buffer_request = N_samples_needed*4;
	input_samples_buffer_writeptr = 1;
         
	return;
end

% set the parameter depending on spectrum_occupancy
if (spectrum_occupancy<0)
   if (spectrum_occupancy_estimation<0)
   	spectrum_occupancy = 3;
   else
      spectrum_occupancy = spectrum_occupancy_estimation;
   end
end

spectrum_occupancy = min(spectrum_occupancy,3);	% 4,5 not supported yet!
mode_and_occupancy_code = mode_and_occupancy_code_table( (robustness_mode-1)*6 + spectrum_occupancy + 1);  

if (mode_and_occupancy_code<0)
  	spectrum_occupancy = 3;
	mode_and_occupancy_code = mode_and_occupancy_code_table( (robustness_mode-1)*6 + spectrum_occupancy + 1);   
end% todo: better go out here and restart robustness_mode-detection

if (mode_and_occupancy_code ~= mode_and_occupancy_code_last)
	K_min = K_min_K_max_list(spectrum_occupancy*2 + (robustness_mode-1)*2*6 + 1);
	K_max = K_min_K_max_list(spectrum_occupancy*2 + (robustness_mode-1)*2*6 + 2);
	carrier_per_symbol = K_max - K_min + 1;
   
   %reformat the pilot index stuff into the K_dc/K_modulo memory block
   gain_ref_cells_k = gain_ref_cells_k_list{mode_and_occupancy_code+1};
   temp = floor( (gain_ref_cells_k-K_min)/carrier_per_symbol )*(K_modulo-carrier_per_symbol);
   gain_ref_cells_k = ( gain_ref_cells_k + temp ) + ( K_dc - 1 );
   
	gain_ref_cells_theta_1024 = gain_ref_cells_theta_1024_list{mode_and_occupancy_code+1};
   gain_ref_cells_a = gain_ref_cells_a_list{mode_and_occupancy_code+1};
   
   FAC_cells_k = Create_FAC_Demapper(robustness_mode, K_dc, K_modulo);
   W_symbol_int16 = W_symbol_int16_list{mode_and_occupancy_code+1};
   W_pilots_int16 = W_pilots_int16_list{mode_and_occupancy_code+1};
   gain_ref_cells_subset = gain_ref_cells_subset_list{mode_and_occupancy_code+1};
   mode_and_occupancy_code_last = mode_and_occupancy_code;
   
   H_last = zeros( carrier_per_symbol, 1);
   next_pilots = zeros( size(W_pilots_int16{y},2), 1 );
   
   gain_ref_cells_per_frame = length(gain_ref_cells_k);
   gain_ref_cells_per_y_symbols = gain_ref_cells_per_frame / (symbols_per_frame/y);
end

%for display
toctic_equalization = 0;
toctic_fft = 0;
freq_offset_log = zeros(1, symbols_per_frame);
time_offset_log = zeros(1, symbols_per_frame);

t_smp = 0;
%lets equalize one frame
for ( s = [1:symbols_per_frame] )
   symbol_counter = symbol_counter + 1;
   
   %shifted symbol index
   nn = rem(s-1-symbols_to_delay + symbols_per_frame, symbols_per_frame);
   n = rem(nn, y);
   m = floor(nn/y);
   gain_ref_cells_subset_nn = rem( m*gain_ref_cells_per_y_symbols+gain_ref_cells_subset{n+1} - 1, gain_ref_cells_per_frame ) + 1;
   
   training_cells_relative_index = rem( gain_ref_cells_k(gain_ref_cells_subset_nn) + (symbols_per_frame-nn)*K_modulo, ...
      K_modulo*symbols_per_frame);
   
   actual_pilots_relative_index = find( training_cells_relative_index - (symbols_per_2D_window-1)*K_modulo >= 0 );
   
   normalized_training_cells = symbol_buffer( training_cells_relative_index + ((s-1)*K_modulo + 1) ) ...
      .*exp(-j*2*pi*gain_ref_cells_theta_1024(gain_ref_cells_subset_nn)/1024) ...
      ./gain_ref_cells_a(gain_ref_cells_subset_nn);
   
   actual_pilots = normalized_training_cells( actual_pilots_relative_index );
   delta_freq_offset = angle( conj(actual_pilots) * next_pilots + MIN_ABS_H );
   
   %channel estimation with precomputed (polyphase) wiener matrix
   H = mul_int16( normalized_training_cells, W_symbol_int16{n+1} );
   next_pilots = mul_int16( normalized_training_cells, W_pilots_int16{n+1} );
   
   %equalize
   transmission_frame_buffer( (transmission_frame_buffer_writeptr - 1 + (s-1)*K_modulo + K_dc) + [K_min:K_max] ) = transpose( symbol_buffer( ((s-1+symbols_to_delay)*K_modulo + K_dc) + [K_min:K_max] ) ) .* ( conj(H) ./ ( abs(H).^2 + MIN_ABS_H ) );
   channel_transfer_function_buffer( (transmission_frame_buffer_writeptr - 1 + (s-1)*K_modulo + K_dc) + [K_min:K_max] ) = H;
   toctic_equalization = toctic_equalization + toctic;
   
   %get next symbol
   [symbol_temp, delta_time_offset_integer, time_offset_fractional, freq_offset, delta_time_offset_I, phi_freq_correction_last, symbol_position_offset] = ...
   	get_ofdm_symbol_sync( rs_buffer(t_smp+[1:2*Ts]), Ts, Tu, H, delta_freq_offset, time_offset_fractional, freq_offset, delta_time_offset_I, phi_freq_correction_last);
   H_last = H;
   toctic_fft = toctic_fft + toctic;

   symbol_buffer( (symbol_buffer_writeptr-1)*K_modulo + [1:Tu] ) = symbol_temp;
   symbol_buffer_writeptr = symbol_buffer_writeptr + 1;
   
   t_smp = t_smp + Ts + delta_time_offset_integer;	% next symbol
   
   freq_offset_log(s) = freq_offset;
   time_offset_log(s) = time_offset_log_last + delta_time_offset_integer + time_offset_fractional;
   time_offset_log_last = time_offset_log_last + delta_time_offset_integer;
      
   if (0) %for debugging only
	   plot_channelestimation(2, H, symbol_position_offset, Tg, Tu, K_min, K_max, K_dc, K_modulo, gain_ref_cells_k(gain_ref_cells_subset_nn), normalized_training_cells );
      drawnow;
      keyboard; %pause;
	end

end


if ( PLOT_CHANNELESTIMATION )
	plot_channelestimation(2, H, symbol_position_offset, Tg, Tu, K_min, K_max, K_dc, K_modulo, gain_ref_cells_k(gain_ref_cells_subset_nn), normalized_training_cells );
end

if ( PLOT_SYNCHRONISATION==1 )
   plot_synchronisation(2, time_offset_log/12, -freq_offset_log*12000/Tu/(2*pi), -freq_offset_init*12000/Tu/(2*pi), N_symbols_needed, Ts/12000);
end

%if we jump over a sample or skip one, our clock is too fast/slow --> we have to speed-up/slow-down also the playing of the
%decoded data, assuming, that the clock for recording and playing is the same or locked.
smp_rate_conv_fft_phase_diff = 4*(t_smp - N_symbols_needed*Ts);
smp_rate_conv_fft_phase_offset = 4*time_offset_fractional;
smp_rate_conv_in_out_delay = smp_rate_conv_in_out_delay + smp_rate_conv_fft_phase_diff;

message(PRINTTIME<=VERBOSE_LEVEL,sprintf('%5.0fms - fft, freq. offset: %0.1f Hz, samplerate offset: %.0f ppm\n', toctic_fft  * 1000, -freq_offset*12000/Tu/(2*pi), (1/(delta_time_offset_I/Ts+1)-1)*1e6 )); 

% symbol align rs_buffer
rs_buffer_writeptr = rs_buffer_writeptr - t_smp;
rs_buffer(1:(rs_buffer_writeptr-1)) = rs_buffer( t_smp + [1:(rs_buffer_writeptr-1)] );

%swap symbolbuffer
symbol_buffer_writeptr = symbol_buffer_writeptr - symbols_per_frame;
symbol_buffer(1:(symbol_buffer_writeptr-1)*K_modulo) = symbol_buffer( symbols_per_frame*K_modulo + [1:(symbol_buffer_writeptr-1)*K_modulo] );


%SNR-estimation based on FAC-cells
mean_energy_of_used_cells = mean_energy_of_used_cells_list(spectrum_occupancy + (robustness_mode-1)*6 + 1);

FAC_cells_sequence = transmission_frame_buffer( transmission_frame_buffer_writeptr - 1 + FAC_cells_k );
FAC_squared_noise_sequence = (abs(real(FAC_cells_sequence))-sqrt(0.5)).^2 + (abs(imag(FAC_cells_sequence))-sqrt(0.5)).^2; 
squared_weight_sequence = abs(channel_transfer_function_buffer( transmission_frame_buffer_writeptr - 1 + FAC_cells_k )).^2;
MER_FAC = -10*log10( sum(FAC_squared_noise_sequence)/length(FAC_cells_sequence)  + 1e-10);
%WMER_FAC = 10*log10(   mean_energy_of_used_cells  *  ...
%   sum( squared_weight_sequence )/sum( FAC_squared_noise_sequence.*squared_weight_sequence )  + 1e-10);

WMER_FAC = -10*log10(   sum( FAC_squared_noise_sequence.*(squared_weight_sequence+1e-10) ) / ...
   ( mean_energy_of_used_cells  *  sum( squared_weight_sequence+1e-10 )  )  );

SNR_dB = WMER_FAC; 

message(PRINTTIME<=VERBOSE_LEVEL,sprintf('%5.0fms - equalization, ', toctic_equalization * 1000 )); 
message(1<=VERBOSE_LEVEL,sprintf('FAC MER: %2.1f dB, FAC WMER: %2.1f dB\n',MER_FAC,WMER_FAC));

if (SHOW_SIGNAL_INFO)
   show_signal_info( 2, 11, -freq_offset*12000/Tu/(2*pi) );
   show_signal_info( 2, 12, (1/(r_record*(1+delta_time_offset_I/Ts)) - 1)*1e6  );
   show_signal_info( 2, 13, MER_FAC );
   show_signal_info( 2, 14, WMER_FAC );
end


if (SNR_dB < SNR_MIN_DB)
   SNR_timeout_counter = SNR_timeout_counter - 1;
   if ( SNR_timeout_counter<=0 )
      do_synchronize = 1;
      SNR_timeout_counter = SNR_TIMEOUT;
   end
   transmission_frame_buffer_data_valid = 0;
   
   %because we mark buffer as not valid, we have to decrease also fac_not_valid_counter
   fac_not_valid_counter = fac_not_valid_counter - 1;
   if ( fac_not_valid_counter<=0 )
      do_synchronize = 1;
      fac_not_valid_counter = FAC_NOT_VALID_TIMEOUT;
   end
   
else
   SNR_timeout_counter = SNR_TIMEOUT;
   transmission_frame_buffer_data_valid = 1;
end


% set next request-data-size
if (do_synchronize==1)
   N_samples_needed = N_symbols_mode_detection*320 - rs_buffer_writeptr + 1;
else
	N_samples_needed = (symbols_per_frame+1)*Ts - (rs_buffer_writeptr-1);	%one additional symbol because of delta_time_offset_integer correction
end   

if ( N_samples_needed > 0 )
  	input_samples_buffer_request = N_samples_needed*4;
   input_samples_buffer_writeptr = 1;
else
  	input_samples_buffer_request = 0;
end


