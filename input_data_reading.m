%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  University of Kaiserslautern, Institute of Communications Engineering     %
%%  Copyright (C) 2005 Andreas Dittrich, Torsten Schorr                       %
%%                                                                            %
%%  Author(s)    : Andreas Dittrich (dittrich@eit.uni-kl.de)                  %
%%                 Torsten Schorr (schorr@eit.uni-kl.de)                      %
%%  Project start: 27.05.2004                                                 %
%%  Last change: 02.05.2005, 20:00                                            %
%%  Changes      : |||                                                        %
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
%% Last change: 28.04.2005, 16:00                                             %
%% By         : Torsten Schorr                                                %
%% Description: message output corrected                                      %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Last change: 28.04.2005, 16:00                                             %
%% By         : Torsten Schorr                                                %
%% Description: included input_data_valid                                     %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  Last change: 02.05.2005, 20:00                                            %
%%  By         : Andreas Dittrich                                             %
%%  Description: info message because of wrong inputfile-sample-rate,         %
%%               default sample rate offset defined in settings.m             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  input_data_writing.m                                                      %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  Description:                                                              %
%%  Recording of DRM signal                                                   %
%%                                                                            %
%%  To be used at first                                                       %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function input_data_writing()

%*******************************************************************************
%* constant global variables                                                   *
%*******************************************************************************
INPUT_SOURCE_SOUNDCARD=0; 
INPUT_SOURCE_FILE=1;
INPUT_SOURCE_TESTSIGNAL=2;

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

global is_time_sync;
global samplerate_offset_estimation;
global r_record;
global do_synchronize;

% input_data_reading --> demodulation_and_equalization
global input_samples_buffer;	% twice the number of samples for a period of 400ms @ 48kHz sample-rate
global input_samples_buffer_writeptr; % has to be set by demodulation_and_equalization
global input_samples_buffer_request;	% has to be set by demodulation_and_equalization 

global gui_parameters
global nongui_parameters

global smp_rate_conv_fft_phase_diff
global smp_rate_conv_fft_phase_offset
global smp_rate_conv_in_out_delay

% input_data_reading --> output_data_writin
global smp_rate_conv_rec_phase_offset

%*******************************************************************************
%* static variables                                                            *
%*******************************************************************************
persistent actual_input_source MIN_R_RECORD MAX_R_RECORD Fs Ch ...
           wave_channels ...
           wave_precision wave_bps wave_offset wave_type wave_dc ...
           readtime_samples wav_file_read_pointer input_samples_buffer_received ...
           start_recordtime wave_samples_per_second RECORD_INPUT_last ...
           record_input_RIFF_chunk_size record_input_data_chunk_size ...
           filename_record_input record_input_wavheader record_input_bytes_per_second ...
           input_file_progress ...
           parameters_testsignal state_testsignal


input_source = gui_parameters{13};
input_filename = gui_parameters{27};
offset_smp = nongui_parameters{11};
PRINTTIME = nongui_parameters{10};
VERBOSE_LEVEL = 3 - gui_parameters{14};
DATA_STORAGE_PATH = gui_parameters{28};

if ( run_state == RUN_STATE_POWERON)
   r_record = 1;
   actual_input_source = input_source;
	Fs = 48000;
   Ch = 1;
   
   MIN_R_RECORD = 1/(1+1.5e-3); % +-1500 ppm
   MAX_R_RECORD = 1/(1-1.5e-3);
   
   return;
end

if ( run_state == RUN_STATE_INIT)
   
   SAMPLERATE_OFFSET_INIT_PPM = settings_handler(6,2);
   
   r_record = 1/(1 + SAMPLERATE_OFFSET_INIT_PPM/1e6);
   smp_rate_conv_in_out_delay = 0;
   smp_rate_conv_rec_phase_offset = 0;
   wave_channels = 1;
   wave_precision = 'int16';
   wave_bps = 16;
   wave_offset = 0;
   wave_type ='';
   wave_dc = 0;
   readtime_samples = 0;
   smp_rate_conv_rec_phase_offset = 0;
   wav_file_read_pointer = offset_smp;
   actual_input_source = input_source;
   RECORD_INPUT_last = 0;
   input_file_progress = 0;
   gui_parameters{29} = input_file_progress;
   settings_handler(7,29,input_file_progress);

end
  
input_data_valid = 0;

if ( actual_input_source == INPUT_SOURCE_SOUNDCARD )	%soundcard
   
   if ( run_state == RUN_STATE_INIT)
      wavrecordex; %stop all pending recording threads
      start_recordtime = clockex;
   end
   
   if ( run_state > RUN_STATE_INIT )
      if ( input_samples_buffer_request > 0 )
         if (is_time_sync==1)
            k_P_rec = 0.005; 
            rec_delay_ctrl = (smp_rate_conv_fft_phase_diff+smp_rate_conv_fft_phase_offset); 
           	r_record = r_record + k_P_rec*rec_delay_ctrl/input_samples_buffer_request;
         else 
            r_record = r_record + 0.75*min(max(samplerate_offset_estimation,-1e-3),1e-3); 
         end
         
         r_record = min(max(r_record, MIN_R_RECORD), MAX_R_RECORD ); 
         
         [temp, smp_rate_conv_rec_phase_diff, smp_rate_conv_rec_phase_offset] = ...
               wavrecordex(input_samples_buffer_request, Fs, Ch, r_record );
         smp_rate_conv_in_out_delay = smp_rate_conv_in_out_delay + smp_rate_conv_rec_phase_diff;
         input_data_valid = 1;
         input_samples_buffer_received = prod(size(temp)); 
         input_samples_buffer(input_samples_buffer_writeptr-1+[1:input_samples_buffer_received]) = temp;
         input_samples_buffer_writeptr = input_samples_buffer_writeptr + input_samples_buffer_received;
         input_samples_buffer_request = input_samples_buffer_request - input_samples_buffer_received;
         
      end
      
     	recordtime_seconds = etime( clockex, start_recordtime  );
     	recordtime_hours = floor( recordtime_seconds/3600 ); 
     	recordtime_seconds = recordtime_seconds - recordtime_hours*3600;
     	recordtime_minutes = floor( recordtime_seconds/60 ); 
     	recordtime_seconds = recordtime_seconds - recordtime_minutes*60;

		message(PRINTTIME <= VERBOSE_LEVEL,sprintf('--------------------------------------------------------------------------------\n%02.0f:%02.0f:%02.0f  wavrecord, no of samples: %i, record samplerate offset: %.0f ppm, \n', recordtime_hours, recordtime_minutes, recordtime_seconds, input_samples_buffer_received, 1e6*(1/r_record-1) ) );
		toctic;
   end
     
elseif ( actual_input_source == INPUT_SOURCE_FILE )	%file input
   
   if ( run_state == RUN_STATE_INIT)
      
	   r_record = 1;
      
%       commented for rough wav read
%       wav_file_size = wavread(input_filename, 'size');
%       no_of_samples_available = wav_file_size(1);
%       no_of_channels = wav_file_size(2);
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%     inserted for rough wav read
      wave_channels = 1;
      wave_precision = 'int16';
      wave_bps = 16;
      wave_offset = 0;
      wave_type ='';
      wave_dc = 0;
      wave_samples_per_second = 48000;
      
      wav_fid=fopen (input_filename,'r');
      if (wav_fid < 0)
        message(0<=VERBOSE_LEVEL, sprintf('Unable to open file %s!\n', input_filename));
        input_filename = '';
        input_source = INPUT_SOURCE_SOUNDCARD;
        run_command = RUN_COMMAND_RESTART;
        return;
      end
      temp = fread(wav_fid,44,'uint8');
      if length(temp == 44) & isequal(temp([1:4,9:16])','RIFFWAVEfmt ')
          if (temp(23) == 1) | (temp(23) == 2)
              wave_channels = temp(23);
              wave_offset = 44;
              wave_type = 'WAV file format';
              if temp(35) == 8
                  wave_precision = 'uint8';
                  wave_bps = 8;
                  wave_dc = 128;
              end
              
              wave_samples_per_second = temp(25) + 256*temp(26) + 65536*temp(27) + 16777216*temp(28);
              if ( wave_samples_per_second ~= 48000)
			        message(0<=VERBOSE_LEVEL, sprintf('file sample rate other than 48kHz not supported yet!\n') );
              end 
          end
       end
       
       readtime_samples = 0;
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

      smp_rate_conv_rec_phase_offset = 0;
      
%       commented for rough wav read
%       if ( no_of_channels>1 )
%          error('only mono wav-files supported');
%       end
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

      wav_file_read_pointer = offset_smp;

%       commented for rough wav read
%       % check samplerate and resolution
%       if (no_of_samples_available>0) 
%          [dummy,fs_Hz,bits] = wavread(input_filename, 1);
%          if ( (fs_Hz~=48000) | (bits~=16) )
%             error('wrong input format, only 16 bit, 48 kHz samplingrate is supported');
%          end
%       end   
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%      
		return;
	end
   
   if ( run_state > RUN_STATE_INIT ) 
      if ( input_samples_buffer_request > 0 )
	%     inserted for rough wav read
    wav_fid=fopen (input_filename,'r');
    if (wav_fid < 0)
%         error (['Unable to open file ', input_filename, '!']);
        input_filename = '';
        input_source = INPUT_SOURCE_SOUNDCARD;
        run_command = RUN_COMMAND_RESTART;
        return;
    end

    fseek(wav_fid,0,'eof');
    input_file_size = ftell(wav_fid);
    
    
    if (gui_parameters{29} ~= input_file_progress)
        input_file_progress = gui_parameters{29};
        wav_file_read_pointer = floor(input_file_progress * (input_file_size - wave_offset) / (wave_channels * (wave_bps/8)));
        do_synchronize = 1;
    end

    fseek(wav_fid,wave_offset + wave_channels * (wave_bps/8) * wav_file_read_pointer,'bof');
      	temp = (fread(wav_fid, wave_channels * input_samples_buffer_request,wave_precision) - wave_dc)/(wave_channels * 2^(wave_bps - 1));
	      fclose(wav_fid);
	   else
         temp = [];
      end
      input_data_valid = 1;
      readtime_samples = readtime_samples + length(temp);
      readtime_seconds = floor( readtime_samples/wave_samples_per_second );
      readtime_hours = floor( readtime_seconds/3600 ); 
      readtime_seconds = readtime_seconds - readtime_hours*3600;
      readtime_minutes = floor( readtime_seconds/60 ); 
      readtime_seconds = readtime_seconds - readtime_minutes*60;
   
      if ( length(temp) == wave_channels * input_samples_buffer_request )
         temp = sum(reshape(temp, wave_channels, input_samples_buffer_request),1);
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%       commented for rough wav read
%       if ( (wav_file_read_pointer+input_samples_buffer_request) <= no_of_samples_available );
%          [temp,fs_Hz,bits] = wavread(input_filename, [wav_file_read_pointer, wav_file_read_pointer-1+input_samples_buffer_request]);
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

         input_samples_buffer_received = prod(size(temp)); 
         input_samples_buffer(input_samples_buffer_writeptr-1+[1:input_samples_buffer_received]) = temp;
         input_samples_buffer_writeptr = input_samples_buffer_writeptr + input_samples_buffer_received;
         input_samples_buffer_request = input_samples_buffer_request - input_samples_buffer_received;
         
         wav_file_read_pointer = wav_file_read_pointer + input_samples_buffer_received;

         input_file_progress = wave_channels * (wave_bps/8) * wav_file_read_pointer / (input_file_size - wave_offset);
         gui_parameters{29} = input_file_progress;
         settings_handler(7,29,input_file_progress);

	      message(PRINTTIME <= VERBOSE_LEVEL,sprintf('--------------------------------------------------------------------------------\n%02.0f:%02.0f:%02.0f  wavread, %s - no of samples: %i - %0.0f kBytes read.\n', readtime_hours, readtime_minutes, readtime_seconds, wave_type, input_samples_buffer_received, (wave_offset + wave_channels * (wave_bps/8) * (wav_file_read_pointer + input_samples_buffer_received)) / 1024 ));
         toctic;
      else
         input_samples_buffer(input_samples_buffer_writeptr-1+[1:input_samples_buffer_request]) = 0;
         input_samples_buffer_writeptr = input_samples_buffer_writeptr + input_samples_buffer_request;
         input_samples_buffer_request = 0;
         message(PRINTTIME <= VERBOSE_LEVEL,sprintf('--------------------------------------------------------------------------------\n%02.0f:%02.0f:%02.0f  wavread, no more input data available - generating silence, sending quit-command\n', readtime_hours, readtime_minutes, readtime_seconds ));
         toctic;
         set(findobj('Tag','stop_pushbutton'),'String','Restart');  
         set(findobj('Tag','stop_pushbutton'),'Callback','diorama');
         run_command = RUN_COMMAND_QUIT;
      end
      
   end

elseif ( actual_input_source == INPUT_SOURCE_TESTSIGNAL )	
   
   if ( run_state == RUN_STATE_INIT)
      readtime_samples = 0;
      smp_rate_conv_rec_phase_offset = 0;
      
     	parameters_testsignal = testsignal_init( { ...
			'robustness_mode', 1, ...
         'spectrum_occupancy', 3, ...
      	'SDC_mode', 1, ...
         'MSC_mode', 3 ...
      } );
    
      state_testsignal = 0;
      
		return;
	end
   
   if ( run_state > RUN_STATE_INIT ) 
      if ( input_samples_buffer_request > 0 )
         
         [x_testsignal, state_testsignal] = testsignal( input_samples_buffer_request, parameters_testsignal, state_testsignal );

         input_samples_buffer_received = input_samples_buffer_request; 
         input_samples_buffer(input_samples_buffer_writeptr-1+[1:input_samples_buffer_received]) = x_testsignal;
         input_samples_buffer_writeptr = input_samples_buffer_writeptr + input_samples_buffer_received;
         input_samples_buffer_request = input_samples_buffer_request - input_samples_buffer_received;
         
      	input_data_valid = 1;
      	readtime_samples = readtime_samples + input_samples_buffer_received;
      	readtime_seconds = floor( readtime_samples/48000 );
      	readtime_hours = floor( readtime_seconds/3600 ); 
      	readtime_seconds = readtime_seconds - readtime_hours*3600;
      	readtime_minutes = floor( readtime_seconds/60 ); 

      	readtime_seconds = readtime_seconds - readtime_minutes*60;
   
	      message(PRINTTIME <= VERBOSE_LEVEL,sprintf('--------------------------------------------------------------------------------\n%02.0f:%02.0f:%02.0f  testsignal, no of samples: %i\n', readtime_hours, readtime_minutes, readtime_seconds, input_samples_buffer_received) );
         toctic;
      
      end
   end
   
else
   error('selected input-source not supported yet');
end

RECORD_INPUT = settings_handler(4,19);

if (RECORD_INPUT & input_data_valid) 
    
    if (~RECORD_INPUT_last)
        
        record_input_samplerate = Fs;
        record_input_channels = 1;
        record_input_bytes_per_second = 2*record_input_channels*record_input_samplerate;
        record_input_wavheader = uint8( [ ...
                'RIFF', ...		%RIFF
                36,0,0,0, ...	%RIFF_chunk_size = filesize - 8
                'WAVE', ...		%WAVE
                'fmt ', ...		%fmt
                16,0,0,0, ...	%chunk size 
                1,0, ...			%format = PCM
                record_input_channels,0, ...			%channels = 1
                [rem( floor( record_input_samplerate*[1,2^-8,2^-16,2^-24] ), 256 )], ...	%samplerate
                [rem( floor( record_input_bytes_per_second*[1,2^-8,2^-16,2^-24] ), 256 )], ...	%bytes per second
                2,0, ...			%block align
                16,0, ...		%bits per sample
                'data', ...
                0,0,0,0 ...		% data_chunk_size = 0
            ] );
        
        record_input_RIFF_chunk_size = length(record_input_wavheader)-8;
        record_input_data_chunk_size = 0;
        
        %filename_waveoutfile = [date,'_',num2str(loop_counter),'_',label{1},'.wav'];
        clock_vector = clock;
        timestr = sprintf('%02d%02d%02d',clock_vector(4),clock_vector(5),floor(clock_vector(6)));
        filename_record_input_postfix = ['_',date,'_',timestr,'.wav'];
        if ( ~exist( 'record_input_filename', 'var' ) ) 
            record_input_filename = [DATA_STORAGE_PATH,filesep,'record_input'];
            if ( ~exist( DATA_STORAGE_PATH, 'dir' ) ) mkdirp( DATA_STORAGE_PATH ); end
        end
        filename_record_input = [record_input_filename, filename_record_input_postfix];
        fid_record_input = fopen( filename_record_input, 'w');
        if ( matlab_version(1) < 5.3 ) record_input_wavheader = double(record_input_wavheader ); end
        fwrite(fid_record_input,record_input_wavheader,'uint8');
        fclose(fid_record_input);
    end
    
    if ( input_samples_buffer_received ) 
        record_input_RIFF_chunk_size = record_input_RIFF_chunk_size + length(temp)*2;
        record_input_data_chunk_size = record_input_data_chunk_size + length(temp)*2;
        
        if (record_input_RIFF_chunk_size<(2^31-1))	% 2GByte
            fid_record_input = fopen(filename_record_input,'r+');
            fseek(fid_record_input,0,'eof');
            fwrite(fid_record_input, 32768 * temp, 'int16' );
            %we have to jump from the end to the beginning of the file to overwrite the header in order to allways 
            %get a correct wav-file also in the case when the user aborts the running program.
            %maybe we could accept a corrupt header and some software-player will play - but for now we will stress-test the harddisk
            fseek(fid_record_input,4,'bof');
            fwrite(fid_record_input, record_input_RIFF_chunk_size, 'uint32' );
            fseek(fid_record_input,length(record_input_wavheader)-4,'bof');
            fwrite(fid_record_input, record_input_data_chunk_size, 'uint32' );
            fclose(fid_record_input);
        end
        
        writetime_seconds = floor( record_input_data_chunk_size/record_input_bytes_per_second );
      	writetime_hours = floor( writetime_seconds/3600 ); 
      	writetime_seconds = writetime_seconds - writetime_hours*3600;
      	writetime_minutes = floor( writetime_seconds/60 ); 
      	writetime_seconds = writetime_seconds - writetime_minutes*60;
         
        message(PRINTTIME <= VERBOSE_LEVEL,sprintf('%02.0f:%02.0f:%02.0f  record input "%s", filesize: %0.1f MByte\n', writetime_hours, writetime_minutes, writetime_seconds, filename_record_input, (record_input_RIFF_chunk_size+8)/(2^20)));
    end
    
end
RECORD_INPUT_last = RECORD_INPUT;

