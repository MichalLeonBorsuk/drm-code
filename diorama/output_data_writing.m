%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  University of Kaiserslautern, Institute of Communications Engineering     %
%%  Copyright (C) 2005  Andreas Dittrich, Torsten Schorr                      %
%%                                                                            %
%%  Author(s)    : Andreas Dittrich (dittrich@eit.uni-kl.de)                  %
%%                 Torsten Schorr (schorr@eit.uni-kl.de)                      %
%%  Project start: 27.05.2004                                                 %
%%  Last change: 11.10.2005, 10:30                                            %
%%  Changes      : ||||                                                       %
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
%%  Last change: 06.04.2005, 16:60                                            %
%%  By         : Torsten Schorr, Andreas Dittrich                             %
%%  Description: support of stereo playback and wavewrite                     %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  Last change: 22.04.2005, 09:15                                            %
%%  By         : Andreas Dittrich                                             %
%%  Description: restart of playthread before jingle and when restarting      %
%%              playing, floor() in jingle size adjustment                    %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  Last change: 27.04.2005                                                   %
%%  By         : Torsten Schorr                                               %
%%  Description: included show_signal_info                                    %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  Last change: 11.10.2005, 10:30                                            %
%%  By         : Andreas Dittrich                                             %
%%  Description: fixed bug: corrupt file output of mono audio                 %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  output_data_writing.m                                                     %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  Description:                                                              %
%%  Playback of audio data                                                    %
%%                                                                            %
%%  To be used after source_decoding.m                                        %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function output_data_writing()

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
global matlab_version;

global smp_rate_conv_in_out_delay;			% integer part of input/output delay in samples
global sdc_data_valid;

% channel_decoding --> source_decoding --> output_data_writing
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

% input_data_reading --> output_data_writing
global smp_rate_conv_rec_phase_offset

% demodulation_and_equalization --> output_data_writing
global smp_rate_conv_fft_phase_offset 
global smp_rate_conv_in_out_delay

%*******************************************************************************
%* static variables                                                            *
%*******************************************************************************
persistent MIN_PLAYBUFFER_MS jingle_begin FS_jingle_begin jingle_end FS_jingle_end ...
           language_codes programme_type_codes audio_coding_codes MIN_PITCH MAX_PITCH ...
           MEAN_PLAYBUFFER_MS LOCK_THRESHOLD_MS LOCK_COUNTDOWN_CYCLES LOCKED_STRINGS ...
           frames_between_audio_data_label_switch switch_frame_counter show_audio_label ...
           in_out_locked in_out_locked_countdown smp_rate_conv_play_phase_offset PLAYWAV_last ...
           WRITEWAV_last playtime_delay_ms valid_last waveformat_bytes_per_second ...
           filename_waveoutfile start_play_time ...
           RIFF_chunk_size data_chunk_size wavheader sampling_rate_codes ...
           audio_mode_codes SBR_codes


PLAYWAV = gui_parameters{8};
WRITEWAV = gui_parameters{7};
SHOW_SIGNAL_INFO = gui_parameters{20};
PRINTTIME = nongui_parameters{10};
VERBOSE_LEVEL = 3 - gui_parameters{14};
input_source = gui_parameters{13};
DATA_STORAGE_PATH = gui_parameters{28};

if (run_state == RUN_STATE_POWERON)
   MIN_PLAYBUFFER_MS = 300;	%should be large enough so that we have time for decoding a 400ms-frame and do the display etc. in worst case
   JINGLE_BEGIN_FILENAME = settings_handler(6,6);
   JINGLE_END_FILENAME = settings_handler(6,7);
   [jingle_begin, FS_jingle_begin, dummy] = wavread(JINGLE_BEGIN_FILENAME);
   [jingle_end, FS_jingle_end, dummy] = wavread(JINGLE_END_FILENAME);
   jingle_begin = [jingle_begin',zeros(size(jingle_begin,2), max(0,floor(MIN_PLAYBUFFER_MS/1000*FS_jingle_begin)-size(jingle_begin,1)))];
   jingle_end = [jingle_end',zeros(size(jingle_end,2), max(0,floor(MIN_PLAYBUFFER_MS/1000*FS_jingle_end)-size(jingle_end,1)))];
   
   language_codes = {'', 'Arabic', 'Bengali', 'Chinese (Mandarin)', 'Dutch', 'English', 'French', 'German', ...
                     'Hindi', 'Japanese', 'Javanese', 'Korean', 'Portuguese', 'Russian', 'Spanish', 'unknown language'};
               
   programme_type_codes = {'', 'News', 'Current Affairs', 'Information', 'Sport', 'Education', 'Drama', 'Culture', ...
                           'Science', 'Varied', 'Pop Music', 'Rock Music', 'Easy Listening Music', 'Light Classical', 'Serious Classical', 'Other music',  ...
                           'Weather/meteorolgy', 'Finance/Business', 'Children''s programmes', 'Social Affairs', 'Religion', 'Phone in', 'Travel', 'Leisure', ...
                           'Jazz music', 'Country music', 'National music', 'Oldies music', 'Folk music', 'Documentary', '', '' };
   audio_coding_codes = {'AAC', 'CELP', 'HVXC', 'unknown'};
   sampling_rate_codes = [8,12,16,24,0,0,0,0];
   audio_mode_codes = {'mono', 'parametric stereo', 'stereo', ''};
   SBR_codes ={'', 'SBR'};
   MIN_PITCH = 1/(1+0.01);
   MAX_PITCH = 1/(1-0.01);
   MEAN_PLAYBUFFER_MS = max(size(jingle_begin))/FS_jingle_begin*1000;
   LOCK_THRESHOLD_MS = 500;
   LOCK_COUNTDOWN_CYCLES = 50;
   LOCKED_STRINGS = {'0','1','-'};
   
   frames_between_audio_data_label_switch = 10;
   switch_frame_counter = 0;
   show_audio_label = 1;
   
   return;
    
elseif (run_state == RUN_STATE_INIT)
    in_out_locked = 0;
    in_out_locked_countdown = 0;
    smp_rate_conv_play_phase_offset = 0;
    PLAYWAV_last = PLAYWAV;
    WRITEWAV_last = WRITEWAV;
	 playtime_delay_ms = MEAN_PLAYBUFFER_MS;
    show_signal_info(2,47,-1);
    valid_last = 0;
    switch_frame_counter = 0;
    show_audio_label = 1;
    return;    
    
elseif (run_state >= RUN_STATE_STOP)
	if (PLAYWAV_last==1)
      wavplayex; %stop immeadiatly playing
   end
   return;
end

    
if ( (output_samples_buffer_data_valid-valid_last)>0 ) 
   if (output_sampling_rate==12000)
      output_sampling_rate_code = 1;
   elseif (output_sampling_rate==24000)
      output_sampling_rate_code = 2;
   else 
      output_sampling_rate_code = 3;
   end
   start_playing = 1;
   stop_playing = 0;
   valid_last = 1;   
elseif ( (output_samples_buffer_data_valid-valid_last)<0 )
   start_playing = 0;
   stop_playing = 1;
   valid_last = 0;
else
   start_playing = 0;
   stop_playing = 0;
end

   
   
if (WRITEWAV) 

	if ((start_playing) || (~WRITEWAV_last))
      
      waveformat_bitsPerSample = 16;
   	waveformat_samplerate = output_sampling_rate;
   	waveformat_channels =  channels;
      waveformat_bytes_per_second = floor( waveformat_bitsPerSample/8*waveformat_channels*output_sampling_rate + 0.5);
      wavheader = uint8( [ ...
            'RIFF', ...		%RIFF
            36,0,0,0, ...	%RIFF_chunk_size = filesize - 8
            'WAVE', ...		%WAVE
            'fmt ', ...		%fmt
            waveformat_bitsPerSample,0,0,0, ...	%chunk size 
            1,0, ...			%format = PCM
            waveformat_channels,0, ...
            [rem( floor( waveformat_samplerate*[1,2^-8,2^-16,2^-24] ), 256 )], ...	%samplerate
            [rem( floor( waveformat_bytes_per_second*[1,2^-8,2^-16,2^-24] ), 256 )], ...	%bytes per second
            ceil(waveformat_bitsPerSample/8)*waveformat_channels,0, ...			%block align
            waveformat_bitsPerSample,0, ...		%bits per sample
            'data', ...
            0,0,0,0 ...		% data_chunk_size = 0
            ] );
         
      RIFF_chunk_size = length(wavheader)-8;
      data_chunk_size = 0;
      
      %filename_waveoutfile = [date,'_',num2str(loop_counter),'_',label{1},'.wav'];
      clock_vector = clock;
      timestr = sprintf('%02d%02d%02d',clock_vector(4),clock_vector(5),floor(clock_vector(6)));
      filename_waveoutfile_postfix = ['_',date,'_',timestr,'.wav'];
      if ( ~exist( 'output_wav_filename', 'var' ) ) 
         output_wav_filename = [DATA_STORAGE_PATH,filesep,'waveout'];
         if ( ~exist( DATA_STORAGE_PATH, 'dir' ) ) mkdirp( DATA_STORAGE_PATH ); end
      end
      filename_waveoutfile = [output_wav_filename, filename_waveoutfile_postfix];
     	fid_wavoutfile = fopen( filename_waveoutfile, 'w');
      if ( matlab_version(1) < 5.3 ) wavheader = double(wavheader ); end
      fwrite(fid_wavoutfile,wavheader,'uint8');
      fclose(fid_wavoutfile);
   end
   
   if ( output_samples_buffer_data_valid==1 ) 
      no_of_bytes_to_add = 2*num_frames*samples_per_audioframe*channels;
      RIFF_chunk_size = RIFF_chunk_size + no_of_bytes_to_add;
      data_chunk_size = data_chunk_size + no_of_bytes_to_add;
      
      if (RIFF_chunk_size<(2^31-1))	% 2GByte
         fid_wavoutfile = fopen(filename_waveoutfile,'r+');
         
         fseek(fid_wavoutfile,0,'eof');
	      if ( matlab_version(1) < 5.3 )       
            fwrite(fid_wavoutfile, double(output_samples_buffer(1:channels, 1:num_frames*samples_per_audioframe)), 'int16' );
         else
            fwrite(fid_wavoutfile, output_samples_buffer(1:channels, 1:num_frames*samples_per_audioframe), 'int16' );
         end
      
         %we have to jump from the end to the beginning of the file to overwrite the header in order to allways 
         %get a correct wav-file also in the case when the user aborts the running program.
         %maybe we could accept a corrupt header and some software-player will play - but for now we will stress-test the harddisk
      	fseek(fid_wavoutfile,4,'bof');
      	fwrite(fid_wavoutfile, RIFF_chunk_size, 'uint32' );
      	fseek(fid_wavoutfile,length(wavheader)-4,'bof');
         fwrite(fid_wavoutfile, data_chunk_size, 'uint32' );
         
         writetime_seconds = floor( data_chunk_size/waveformat_bytes_per_second );
      	writetime_hours = floor( writetime_seconds/3600 ); 
      	writetime_seconds = writetime_seconds - writetime_hours*3600;
      	writetime_minutes = floor( writetime_seconds/60 ); 
      	writetime_seconds = writetime_seconds - writetime_minutes*60;
         
         message(PRINTTIME <= VERBOSE_LEVEL,sprintf('%02.0f:%02.0f:%02.0f  wavwrite "%s", filesize: %0.1f MByte\n', writetime_hours, writetime_minutes, writetime_seconds, filename_waveoutfile, (RIFF_chunk_size+8)/(2^20)));
         
         fclose(fid_wavoutfile);
      end
   end
   
end
WRITEWAV_last = WRITEWAV;

if (PLAYWAV)
   
   if (~PLAYWAV_last && (output_samples_buffer_data_valid==1) && (~start_playing) )
      start_play_time = clockex;
      wavplayex;	%restart play thread
      wavplayex(zeros(channels,floor(output_sampling_rate*MEAN_PLAYBUFFER_MS/1000)),output_sampling_rate,1); %delay to fill buffer
      playtime_delay_ms = MEAN_PLAYBUFFER_MS;
      
      %fade in!
		output_samples_buffer(:,1:num_frames*samples_per_audioframe) = ...
			 int16( double( output_samples_buffer(:,1:num_frames*samples_per_audioframe) ) .*...
			 10.^(( [1:num_frames*samples_per_audioframe;1:num_frames*samples_per_audioframe]/(num_frames*samples_per_audioframe)*40 - 40 )/20 ) );
   end
 
	if ( stop_playing ) 
      wavplayex(ones(channels,1) * jingle_end, output_sampling_rate, output_sampling_rate/FS_jingle_begin );
      playtime_delay_ms = MEAN_PLAYBUFFER_MS;
	elseif (start_playing)
      start_play_time = clockex;
      wavplayex;	%restart play thread      
      wavplayex(ones(channels,1) * jingle_begin, output_sampling_rate, output_sampling_rate/FS_jingle_end );
      playtime_delay_ms = MEAN_PLAYBUFFER_MS;
      
      %fade in!
		output_samples_buffer(:,1:num_frames*samples_per_audioframe) = ...
			 int16( double( output_samples_buffer(:,1:num_frames*samples_per_audioframe) ) .*...
			 10.^(( [1:num_frames*samples_per_audioframe;1:num_frames*samples_per_audioframe]/(num_frames*samples_per_audioframe)*40 - 40 )/20 ) );
	end

	if ( (output_samples_buffer_data_valid==1) && (~stop_playing) ) 
      
      if (input_source==INPUT_SOURCE_SOUNDCARD) 
         
         if ( abs(playtime_delay_ms - MEAN_PLAYBUFFER_MS + smp_rate_conv_in_out_delay/output_sampling_rate*1000)>LOCK_THRESHOLD_MS )
            in_out_locked = 0;
            in_out_locked_countdown = LOCK_COUNTDOWN_CYCLES;
         end
         
         if (in_out_locked_countdown>0)
            in_out_locked_countdown = in_out_locked_countdown - 1;           
            smp_rate_conv_in_out_delay = 0.9*smp_rate_conv_in_out_delay + 0.1*(MEAN_PLAYBUFFER_MS-playtime_delay_ms)*output_sampling_rate/1000;
         else
            in_out_locked = 1;
         end
            
       	%this is a P-controller to phase-lock the output-sample-stream to the incoming data
      	in_out_delay_ctrl = smp_rate_conv_in_out_delay + smp_rate_conv_fft_phase_offset + smp_rate_conv_rec_phase_offset - smp_rate_conv_play_phase_offset;
      	r_play = 1 + 0.05*(in_out_delay_ctrl)/(num_frames*samples_per_audioframe);
         r_play = min( max( MIN_PITCH, r_play ), MAX_PITCH );
         
         samplerate_offset_string = sprintf('%.0f ppm', (1/r_play-1)*1e6 );
      else
	      in_out_locked = 2;
         sleep( max(min((playtime_delay_ms-MEAN_PLAYBUFFER_MS)*1.75,MEAN_PLAYBUFFER_MS),0) ) ;
         r_play = 1;
         samplerate_offset_string = '-';
   	end
      
      [playtime_delay_ms, smp_rate_conv_play_phase_diff, smp_rate_conv_play_phase_offset] = wavplayex(output_samples_buffer(1:channels, 1:num_frames*samples_per_audioframe), output_sampling_rate, r_play);
		smp_rate_conv_in_out_delay = smp_rate_conv_in_out_delay - smp_rate_conv_play_phase_diff;
      
      playtime_seconds = etime( clockex, start_play_time );
      playtime_hours = floor( playtime_seconds/3600 ); 
      playtime_seconds = playtime_seconds - playtime_hours*3600;
      playtime_minutes = floor( playtime_seconds/60 ); 
      playtime_seconds = playtime_seconds - playtime_minutes*60;

		message(PRINTTIME <= VERBOSE_LEVEL,sprintf('%02.0f:%02.0f:%02.0f  wavplay, buffer-level: %0.3f s, play samplerate offset: %s, locked: %s\n', playtime_hours, playtime_minutes, playtime_seconds, playtime_delay_ms/1000, samplerate_offset_string, LOCKED_STRINGS{in_out_locked+1}) );
      
      if (SHOW_SIGNAL_INFO)
			if (playtime_delay_ms<50)
				show_signal_info(2,55,2);	%playbuffer indicator -> bad (red)
			elseif (playtime_delay_ms<500)
   		   show_signal_info(2,55,3);	%playbuffer indicator -> low, but OK (yellow)
		   else
   		   show_signal_info(2,55,1);	%playbuffer indicator -> OK (green)
		   end
		end
      
      % if you have choppy play and it does not stop without a break enable the following line
		if (0) %( (playtime_delay_ms==0) & (etime(clockex,start_play_time)>0.2) )  %buffer underrun => insert silence
         playtime_delay_ms = wavplayex(zeros(channels,output_sampling_rate/4),output_sampling_rate,1); %1/4 of a second delay to fill buffer
      end
         
   else
   	if (SHOW_SIGNAL_INFO)
			show_signal_info(2,55,0);	%playbuffer indicator -> N/A (transparent)
      end
   end
   
else
   
	if (PLAYWAV_last)
      wavplayex; %stop immeadiatly playing
      playtime_delay_ms = MEAN_PLAYBUFFER_MS;      
   end
   
   if (SHOW_SIGNAL_INFO)
		show_signal_info(2,55,0);	%playbuffer indicator -> N/A (transparent)
   end
   
end
PLAYWAV_last = PLAYWAV;



% visualization of SDC data and text messages

switch_frame_counter = switch_frame_counter + 1;
if (mod(switch_frame_counter,10) == 0)
    show_audio_label = 1 -show_audio_label;
end

if (sdc_data_valid)
    for service_ID = stream_information.audio_services
        audio_coding = audio_information.audio_coding(service_ID);
        audio_service_message = text_message.string;
        audio_service_message(find(audio_service_message < 32 | audio_service_message > 126)) = 32;
        
        SBR_flag = audio_information.SBR_flag(service_ID);
        audio_mode = audio_information.audio_mode(service_ID);
        sampling_rate_code = audio_information.sampling_rate(service_ID);

        message(2 <= VERBOSE_LEVEL,sprintf('          %d: %s - %s audio - ', service_ID, application_information.label{service_ID}, audio_coding_codes{audio_coding + 1}));
        message(2 <= VERBOSE_LEVEL,sprintf('%dkHz %s %s - ',sampling_rate_codes(sampling_rate_code + 1),SBR_codes{SBR_flag + 1}, audio_mode_codes{audio_mode + 1}));
        message(2 <= VERBOSE_LEVEL,sprintf('%s %s ',application_information.language{service_ID},application_information.country{service_ID}));
        message(2 <= VERBOSE_LEVEL,sprintf('%s %s - %4.2f kbit/s',language_codes{application_information.language_code(service_ID) + 1},programme_type_codes{application_information.programme_type_code(service_ID) + 1},audio_information.bytes_per_frame(service_ID) * 8 / 1000 / 0.4 ));
        
        if (audio_information.text_flag(service_ID))
            message(2 <= VERBOSE_LEVEL,sprintf('\n             %s', audio_service_message));
            show_signal_info(2,25 + (service_ID - 1) * 3,audio_service_message);
        end
        
        if (~ismember(service_ID,stream_information.data_services) || show_audio_label)
                show_signal_info(2,23 + (service_ID - 1) * 3, sprintf('%s - %s audio - %s %s %s %s - %4.2f kbit/s', ... 
                    application_information.label{service_ID}, audio_coding_codes{audio_coding + 1}, ...
                    application_information.language{service_ID},application_information.country{service_ID}, ...
                    language_codes{application_information.language_code(service_ID) + 1},...
                    programme_type_codes{application_information.programme_type_code(service_ID) + 1}, ...
                    audio_information.bytes_per_frame(service_ID) * 8 / 1000 / 0.4 ));
        end
        
        message(2 <= VERBOSE_LEVEL,sprintf('\n'));   
    end
    
    for service_ID = stream_information.data_services
        service_type = 'Unknown service';
        if (application_information.application_domain(service_ID) == 0)
            service_type = 'Unknown DRM service';
        elseif (application_information.application_domain(service_ID) == 1) 
            switch application_information.user_application_type(service_ID)
                
                case 2
                    service_type = 'MOT Slideshow';
                case 3      
                    service_type = 'MOT Broadcast Web Site';
                case 1098   
                    service_type = 'Journaline(R) news service';
                otherwise   
                    service_type = 'Unknown DAB service';
            end
            
        end
        message(2 <= VERBOSE_LEVEL,sprintf('          %d: %s - %s - %4.2f kbit/s\n', service_ID, application_information.label{service_ID}, service_type, ...
                application_information.bytes_per_frame(service_ID) * 8 / 1000 / 0.4 ));
        
        if (~ismember(service_ID,stream_information.audio_services) || ~show_audio_label)
            show_signal_info(2,23 + (service_ID - 1) * 3,sprintf('%s - %s - %4.2f kbit/s\n', application_information.label{service_ID}, service_type, ...
                application_information.bytes_per_frame(service_ID) * 8 / 1000 / 0.4 ));
        end
       
    end 
    
    for service_ID = setdiff([1:4], stream_information.data_services)
        if (~ismember(service_ID,stream_information.audio_services))
            show_signal_info(2,23 + (service_ID - 1) * 3,'');
            show_signal_info(2,25 + (service_ID - 1) * 3,'');
        end
    end
    
    
    if (time_and_date.day > 0)
        message(2 <= VERBOSE_LEVEL,sprintf('          Date: %04d-%02d-%02d    Time (UTC): %02d:%02d\n', time_and_date.year, time_and_date.month, time_and_date.day, time_and_date.hours, time_and_date.minutes));
    end
 
    show_signal_info(2,8,time_and_date);

else
    % clear text messages
    show_signal_info(2,25 + 0 * 3,'');
    show_signal_info(2,25 + 1 * 3,'');
    show_signal_info(2,25 + 2 * 3,'');
    show_signal_info(2,25 + 3 * 3,'');
end
