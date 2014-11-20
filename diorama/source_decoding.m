%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  University of Kaiserslautern, Institute of Communications Engineering     %
%%  Copyright (C) 2005 Torsten Schorr, Andreas Dittrich                       %
%%                                                                            %
%%  Author(s)    : Torsten Schorr (schorr@eit.uni-kl.de)                      %
%%                 Andreas Dittrich (dittrich@eit.uni-kl.de)                  %
%%  Project start: 27.05.2004                                                 %
%%  Last change: 12.05.2005, 10:15                                            %
%%  Changes      : |||||                                                      %
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
%%  Last change: 22.04.2005, 13:20                                            %
%%  By         : Andreas Dittrich                                             %
%%  Description: new: convenience noise if no valid audio data available      %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  Last change: 28.04.2005                                                   %
%%  By         : Torsten Schorr                                               %
%%  Description: included show_signal_info, CRC error count                   %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  Last change: 29.04.2005                                                   %
%%  By         : Torsten Schorr                                               %
%%  Description: Unequal Error Protection                                     %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  Last change: 10.05.2005                                                   %
%%  By         : Torsten Schorr                                               %
%%  Description: Fixed problems in web site reception                         %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  Last change: 12.05.2005, 10:15                                            %
%%  By         : Andreas Dittrich                                             %
%%  Description: Fixed Bug in convienience-noice generation                   %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  source_decoding.m                                                         %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  Description:                                                              %
%%  AAC decoding for audio services                                           %
%%  Data sorting and decompression for data services                          %
%%                                                                            %
%%  To be used after channel_decoding.m                                       %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function source_decoding()

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
global sdc_data_valid;

global WMER_FAC

% channel_decoding --> source_decoding
global channel_decoded_data_buffer;
global channel_decoded_data_buffer_data_valid;

% channel_decoding --> source_decoding --> output_data_writing
global multiplex_description
global application_information
global audio_information
global stream_information
global text_message

% source_decoding --> output_data_writing
global output_samples_buffer;	% fixed size data block
global output_samples_buffer_data_valid;
global output_samples_buffer_data_start;
global output_sampling_rate
global channels
global num_frames
global samples_per_audioframe

global gui_parameters
global nongui_parameters

%*******************************************************************************
%* static variables                                                            *
%*******************************************************************************
persistent convenience_noise_wav convenience_noise_gain convenience_noise_wav_ptr ...
           channel_mode zeros_3840x1_int16 zeros_1x3840_int16 sampling_rate_list ...
           channel_list num_frames_list default_text_message default_data_unit_assembly ...
           default_MOT_directory_assembly default_MOT_object_assembly ...
           default_MOT_object_assembly_information default_MOT_object frame_errors_total ...
           initial_source_decoding audio_fer data_units_assembly ...
           MOT_objects_assembly_information MOT_objects_assembly MOT_directories_assembly ...
           last_temp


SOURCEDECODING = gui_parameters{18};           
SHOW_SIGNAL_INFO = gui_parameters{20};
PRINTTIME = nongui_parameters{10};
VERBOSE_LEVEL = 3 - gui_parameters{14};
DATA_STORAGE_PATH = gui_parameters{28};
audio_service_index = gui_parameters{30};

if (run_state == RUN_STATE_POWERON)
    
    CONVENIENCE_NOISE_FILENAME = settings_handler(6,8); 
     
    convenience_noise_wav = wavread(CONVENIENCE_NOISE_FILENAME);
    convenience_noise_gain = 0;
    convenience_noise_wav_ptr = 0;
    
    channel_mode = 1;
    zeros_3840x1_int16 = int16( zeros(3840,1) );
    zeros_1x3840_int16 = int16( zeros(1,3840) );
    sampling_rate_list = [8000, 12000, 16000, 24000, 0, 0, 0, 0];
    % DRM channel_mode definitions (libfaad: faad.h)
    % define DRMCH_MONO          1
    % define DRMCH_STEREO        2
    % define DRMCH_SBR_MONO      3
    % define DRMCH_SBR_STEREO    4
    % define DRMCH_SBR_PS_STEREO 5
    channel_list = [1 0 2 0;3 5 4 0];
    num_frames_list = [0, 5, 0, 10, 0, 0, 0, 0];
    
    default_text_message = struct('stream_no', -1, 'current_toggle', 0, 'first_last_flag', 0,...
                                  'command_flag', 0, 'field1', 0, 'field2', 0, 'segments', {{[],[],[],[],[],[],[],[]}},...
                                  'string', 'Receiving...', 'CRC_error', 0, 'first_seg_received', 0,...
                                  'current_segment', [], 'current_segment_no', 0, 'current_segment_length', 0);
    
    default_data_unit_assembly = struct('ID', 0, 'first_packet_received', 0,'packet_ID', 0,'continuity_index', -1,...
                                        'application_domain', 0, 'application_data', [], 'CRC_error', 0, 'data_unit', []);
    
    default_MOT_directory_assembly = struct('transport_ID', 0, 'continuity_index', -1, 'current_segment_no', -1, ...
                                            'body_complete', 0, 'body', []);
    
    default_MOT_object_assembly = struct('transport_ID', 0, 'header_continuity_index', -1, 'current_header_segment_no', -1,...
                                         'body_continuity_index', -1, 'current_body_segment_no', -1, 'header_complete', 0,...
                                         'body_complete', 0, 'delete', 0, 'header', [], 'body', []);
    
    default_MOT_object_assembly_information = struct('transport_ID', []);
    
    default_MOT_object = struct('ID', 0, 'content_type', 0, 'content_subtype', 0, 'creation_time', 0, 'start_validity', 0,...
                                'expire_time', 0, 'trigger_time', 0,...
                                'version_number', 0, 'repetition_distance', [], 'group_reference', [],...
                                'priority', 0, 'label', 'no label', 'content_name', 'unnamed content',...
                                'content_description', 'undefined_content', 'mime_type', '', 'compression_type', 0, ...
                                'header', [], 'body', [], 'error', 0);
    
    news_counter = 0;
    frame_errors_total = 0;     
    return;
    
elseif (run_state == RUN_STATE_INIT)
    convenience_noise_gain = 0;
    convenience_noise_wav_ptr = 0;
    initial_source_decoding = 1;
    audio_fer = 0;
    show_signal_info(2,56,NaN);
    return;    
    
elseif (run_state == RUN_STATE_FIRSTRUN)
    text_message = default_text_message;
    data_units_assembly = {default_data_unit_assembly, default_data_unit_assembly, default_data_unit_assembly, default_data_unit_assembly};
    MOT_objects_assembly_information = {default_MOT_object_assembly_information, default_MOT_object_assembly_information, default_MOT_object_assembly_information, default_MOT_object_assembly_information};
    MOT_objects_assembly = {[],[],[],[]};
    MOT_directories_assembly = {default_MOT_directory_assembly, default_MOT_directory_assembly, default_MOT_directory_assembly, default_MOT_directory_assembly};
    output_samples_buffer_data_valid = 0;
    last_temp = zeros_1x3840_int16;
    journaline_decode(0);
    initial_source_decoding = 1;
    audio_fer = 0;
    show_signal_info(2,56,NaN);
end

output_samples_buffer_data_valid = 0;
frameinfo_errors = 0;

if (~SOURCEDECODING) return; end
   
if (~channel_decoded_data_buffer_data_valid | ~sdc_data_valid) 
    last_temp = zeros_1x3840_int16;
    drm_aacdecode([],24000,channel_mode);	%close decoder
    text_message = default_text_message;
    data_units_assembly = {default_data_unit_assembly, default_data_unit_assembly, default_data_unit_assembly, default_data_unit_assembly};
    MOT_objects_assembly_information = {default_MOT_object_assembly_information, default_MOT_object_assembly_information, default_MOT_object_assembly_information, default_MOT_object_assembly_information};
    MOT_objects_assembly = {[],[],[],[]};
    MOT_directories_assembly = {default_MOT_directory_assembly, default_MOT_directory_assembly, default_MOT_directory_assembly, default_MOT_directory_assembly};
    initial_source_decoding = 1;
    audio_fer = 0;
    show_signal_info(2,56,NaN);
    return; 
end


CRC_error_counter = 0;
CRC_OK_counter = 0;

% Decoding of audio streams:

for serviceID = stream_information.audio_services
    stream_ID = audio_information.stream_ID(serviceID);
    N_partA = multiplex_description.stream_lengths(1, stream_ID + 1);
    N_partB = multiplex_description.stream_lengths(2, stream_ID + 1);
    audio_information.bytes_per_frame(serviceID) = N_partA + N_partB;    
end

if ((stream_information.number_of_audio_streams > 0) & (length(stream_information.audio_services) >= audio_service_index))
    
    AUDIO_SERVICE = stream_information.audio_services(audio_service_index);
    
    audio_coding = audio_information.audio_coding(AUDIO_SERVICE);
    SBR_flag = audio_information.SBR_flag(AUDIO_SERVICE);
    audio_mode = audio_information.audio_mode(AUDIO_SERVICE);
    sampling_rate_code = audio_information.sampling_rate(AUDIO_SERVICE);
    text_flag = audio_information.text_flag(AUDIO_SERVICE);
    
    stream_ID = audio_information.stream_ID(AUDIO_SERVICE);
    service_ID = AUDIO_SERVICE;
    N_partA = multiplex_description.stream_lengths(1, stream_ID + 1);
    N_partB = multiplex_description.stream_lengths(2, stream_ID + 1);
    Start_partA = sum(multiplex_description.stream_lengths(1, 1:stream_ID))*8 + 1;
    Start_partB = sum(multiplex_description.stream_lengths(1, :))*8 + 1 + sum(multiplex_description.stream_lengths(2, 1:stream_ID))*8;

    higher_protected_part = channel_decoded_data_buffer(Start_partA:Start_partA+N_partA*8-1);
    lower_protected_part = channel_decoded_data_buffer(Start_partB:Start_partB+N_partB*8-1);
    
    % Text message application:
    % ETSI ES 201980 / 6.5
    text_message.CRC_error = 0;
    if (text_flag ~= 0)
        text_message_bits = lower_protected_part((N_partB - 4)*8 + 1:N_partB*8,:)';
        text_message = text_message_decoding (text_message_bits, text_message);
    end
    
    if (audio_coding == 0)
        
        channel_mode = channel_list(SBR_flag+1,audio_mode+1);
        channels = 1 + double (audio_mode > 0);
        AAC_sampling_rate = sampling_rate_list(sampling_rate_code+1);
        num_frames = num_frames_list(sampling_rate_code+1);
        output_sampling_rate = AAC_sampling_rate * (SBR_flag+1);
        samples_per_audioframe = (0.4/num_frames)*AAC_sampling_rate*(SBR_flag+1)*(floor(audio_mode/2)+1);
        
        number_of_headerbytes = ceil((num_frames-1)*12/8);
        lower_protected_part_bytewise = bits2bytes(lower_protected_part(1:(N_partB - 4) * 8,:));
        higher_protected_part_bytewise = bits2bytes(higher_protected_part);
        
        if (N_partA == 0)
            frame_borders=reshape(lower_protected_part(1:12*(num_frames-1)),12,num_frames-1)'*(2.^[11:-1:0]');    
            crc=lower_protected_part_bytewise(number_of_headerbytes+1:number_of_headerbytes+num_frames);
            lower_protected_part_bytewise = lower_protected_part_bytewise(number_of_headerbytes+num_frames+1:end);
            num_higher_protected_bytes = 0;
        else
            frame_borders=reshape(higher_protected_part(1:12*(num_frames-1)),12,num_frames-1)'*(2.^[11:-1:0]');
            num_higher_protected_bytes = (N_partA - number_of_headerbytes - num_frames) / num_frames;
            higher_protected_part_sorted = reshape(higher_protected_part_bytewise(number_of_headerbytes+1:end),num_higher_protected_bytes + 1, num_frames);
            crc=higher_protected_part_sorted(end,:);
            higher_protected_part_sorted = higher_protected_part_sorted(1:end-1,:)';
        end
        
        frame_length = [frame_borders;(N_partA + N_partB - 4 - number_of_headerbytes - num_frames)] - [0;frame_borders] - num_higher_protected_bytes;                
        previousborder = 0;
        audio_error_counter = 0;
        for audio_subframe_no = 1:num_frames
            if ((previousborder >= 0) & (previousborder + frame_length(audio_subframe_no) <= length(lower_protected_part_bytewise)) & (frame_length(audio_subframe_no) > 0))
                if (N_partA == 0)
                    AAC_frame = [crc(audio_subframe_no),lower_protected_part_bytewise(previousborder + [1:frame_length(audio_subframe_no)])];
                else
                    AAC_frame = [crc(audio_subframe_no),higher_protected_part_sorted(audio_subframe_no,:),lower_protected_part_bytewise(previousborder + [1:frame_length(audio_subframe_no)])];
                end
            else
                AAC_frame = uint8(0);
            end
            previousborder = previousborder + frame_length(audio_subframe_no);
            
            [temp, faad_error] = drm_aacdecode(AAC_frame,AAC_sampling_rate,channel_mode);
            
            if (channels == 1 & size(temp,1) > 0)
                temp = int16(sum(double(temp),1)/size(temp,1));
            end
	    
            if (matlab_version(1) >= 5.3)
                if (~isempty(temp) & (length(find(temp == 32767)) + length(find(temp == -32768)) >= 2)) temp = []; end
            else 
                if (~isempty(temp) & (length(find(double(temp) == 32767)) + length(find(double(temp) == -32768)) >= 2)) temp = []; end
            end
            
            if ( size(temp,2) == samples_per_audioframe )
                output_samples_buffer(1:channels, (audio_subframe_no - 1) * samples_per_audioframe + 1:(audio_subframe_no) * samples_per_audioframe) = temp(1:channels, 1:samples_per_audioframe);
                last_temp = temp;
                convenience_noise_gain=0;
                convenience_noise_wav_ptr=0;
                CRC_OK_counter = CRC_OK_counter + 1;
                
                if (~output_samples_buffer_data_valid)
                    output_samples_buffer_data_valid = 1;   
                    output_samples_buffer_data_start = (audio_subframe_no - 1) * samples_per_audioframe + 1;
                end
                
            else 
               if ( (size(last_temp,2) ~= samples_per_audioframe) | (size(last_temp,1) ~= channels) )
                  last_temp = int16( zeros( channels, samples_per_audioframe ) );
               end
               
                convenience_noise_gain = convenience_noise_gain*0.90 + 0.1*(2^15-1)*max(min( 2^((14-WMER_FAC)/2),0.8),0.2);
                
                if ( (convenience_noise_wav_ptr + samples_per_audioframe) > length(convenience_noise_wav) )
                   convenience_noise_wav_ptr = convenience_noise_wav_ptr - length(convenience_noise_wav);
                end
                convenience_noise = reshape( ...
                   convenience_noise_wav([(convenience_noise_wav_ptr+1):(convenience_noise_wav_ptr+samples_per_audioframe)]), ...
                   1, samples_per_audioframe )*convenience_noise_gain;
                if (channels==2)
                   convenience_noise = [convenience_noise;convenience_noise];
                end
                convenience_noise_wav_ptr = convenience_noise_wav_ptr + samples_per_audioframe;
                
                last_temp = int16( double( (last_temp) )*10^(-0.5/20) );
                temp = int16( double(last_temp) + convenience_noise );
                output_samples_buffer(1:channels, (audio_subframe_no - 1) * samples_per_audioframe + 1:(audio_subframe_no) * samples_per_audioframe) = temp(1:channels, 1:samples_per_audioframe);
                if (~initial_source_decoding & (channel_decoded_data_buffer_data_valid < 2))
                    frameinfo_errors = frameinfo_errors+1;
                    CRC_error_counter = CRC_error_counter + 1;
                    audio_error_counter = audio_error_counter + 1;
                end
            end
            initial_source_decoding = 0;
        end
        
        audio_fer = audio_fer * (1 - 0.01) + 0.01 * audio_error_counter/num_frames;
        
        show_signal_info(2,56,audio_fer);
        
        frame_errors_total = frame_errors_total + frameinfo_errors;
        
        message(PRINTTIME <= VERBOSE_LEVEL,sprintf('%5.0fms - AAC decoding, frameinfo errors: %i, total errors: %i\n', toctic * 1000, frameinfo_errors, frame_errors_total));    
    else
       message(PRINTTIME <= VERBOSE_LEVEL,sprintf('%5.0fms - audio decoding, CELP and HVXC not supported yet!\n', toctic * 1000));
    end
end



% Decoding of data streams:

application_information.bytes_per_frame = [0 0 0 0];

for service_ID=stream_information.data_services
    application_domain = application_information.application_domain(service_ID);
    application_data = application_information.application_data{service_ID};
    if (application_domain == 0)        % DRM application domain: no application registered at the moment
        if (length(application_data) >= 16)
            application_information.user_application_identifier(service_ID) = application_data(1:16)*(2.^[15:-1:0]');
        else
            application_information.user_application_identifier(service_ID) = 0;
        end
    elseif (application_domain == 1)    % DAB application domain
        application_information.user_application_type(service_ID) = application_data(6:16)*(2.^[10:-1:0]');
    else    
        % reserved for future definition
    end                         % if (application_domain == xx)      
end


for data_stream = stream_information.data_streams
   
    services_in_this_stream = application_information.ID(find(application_information.stream_ID == data_stream));
    N_partA = multiplex_description.stream_lengths(1, data_stream + 1);
    N_partB = multiplex_description.stream_lengths(2, data_stream + 1);
    Start_partA = sum(multiplex_description.stream_lengths(1, 1:data_stream))*8 + 1;
    Start_partB = Start_partA + sum(multiplex_description.stream_lengths(2, 1:data_stream))*8;
    
    data_unit_counter = 0;
    
    % synchronous stream mode occupies a full data stream => all services
    % of a stream must be either asynchronous or synchronous
    packet_mode = application_information.packet_mode(services_in_this_stream(1)+1);
    if (packet_mode == 1)               % packet mode
        % assuming equal length packets (minimizing error propagation (ETSI ES 201980 / 6.6.4)
        packet_length = application_information.packet_length(services_in_this_stream(1)+1);
        packet_buffer = bits2bytes(channel_decoded_data_buffer(Start_partB:Start_partB+N_partB*8-1));
        packet_buffer_length = N_partB;
        
        data_unit_counter = 0;
        data_units_assignment = [];
        data_units = [];
        
        packet_buffer_index = 1;    % assuming equal length packets (minimizing error propagation (ETSI ES 201980 / 6.6.4) 
        while (packet_buffer_index <= length(packet_buffer))
            header = bytes2bits(packet_buffer(packet_buffer_index));
            packet_ID = header(3:4)*(2.^[1:-1:0]');
            corresponding_service = intersect(find(application_information.packet_ID == packet_ID), stream_information.data_services);
            if (~isempty(corresponding_service))                      
                if (packet_buffer_index + packet_length+3-1 > length(packet_buffer))
                    break;
                end
                packet = packet_buffer(packet_buffer_index+[0:packet_length+3-1]);
                application_information.bytes_per_frame(corresponding_service) = application_information.bytes_per_frame(corresponding_service) + packet_length;
                
                if (crc16_bytewise(packet) == 0)
                    CRC_OK_counter = CRC_OK_counter + 1;
                    data_unit_indicator = application_information.data_unit_indicator(corresponding_service);
                    % assembly of data unit:
                    if (data_unit_indicator == 1)   % data unit transfer mode, e.g. for MOT, Journaline(R) news service
                        
                        first_last_flag = header(1:2)*(2.^[1:-1:0]');
                        PPI = header(5);
                        continuity_index = header(6:8)*(2.^[2:-1:0]');
                        if (first_last_flag < 2)
                            if ((data_units_assembly{corresponding_service}.continuity_index ~= -1) & ...
                                    (rem(data_units_assembly{corresponding_service}.continuity_index+1,8) == continuity_index) & ...
                                    (data_units_assembly{corresponding_service}.first_packet_received == 1))
                                if (PPI == 0)
                                    data_units_assembly{corresponding_service}.data_unit = ...
                                        [data_units_assembly{corresponding_service}.data_unit, packet(2:end-2)];
                                else
                                    useful_bytes = double(packet(2));
                                    if ~((useful_bytes == 1) & (packet(3) == 0))
                                        data_units_assembly{corresponding_service}.data_unit = ...
                                            [data_units_assembly{corresponding_service}.data_unit, packet(3:2+useful_bytes)];
                                    end
                                end
                                data_units_assembly{corresponding_service}.continuity_index = data_units_assembly{corresponding_service}.continuity_index + 1;
                                if (first_last_flag == 1)
                                    data_unit_counter = data_unit_counter + 1;
                                    data_units_assignment(data_unit_counter) = corresponding_service;
                                    data_units{data_unit_counter} = data_units_assembly{corresponding_service}.data_unit;
                                    data_units_assembly{corresponding_service} = default_data_unit_assembly;
                                end
                            else 
                                data_units_assembly{corresponding_service} = default_data_unit_assembly;
                            end
                            
                        else
                            if (data_units_assembly{corresponding_service}.continuity_index ~= -1)
                                data_units_assembly{corresponding_service} = default_data_unit_assembly;
                            end
                            data_units_assembly{corresponding_service}.continuity_index = continuity_index;
                            data_units_assembly{corresponding_service}.first_packet_received = 1;
                            if (PPI == 0)
                                data_units_assembly{corresponding_service}.data_unit = ...
                                    [data_units_assembly{corresponding_service}.data_unit, packet(2:end-2)];
                            else
                                useful_bytes = double(packet(2));
                                if ~((useful_bytes == 1) & (packet(3) == 0))
                                    data_units_assembly{corresponding_service}.data_unit =...
                                        [data_units_assembly{corresponding_service}.data_unit, packet(3:2+useful_bytes)];
                                end
                            end
                            
                            if (first_last_flag == 3)
                                data_unit_counter = data_unit_counter + 1;
                                data_units_assignment(data_unit_counter) = corresponding_service;
                                data_units{data_unit_counter} = data_units_assembly{corresponding_service}.data_unit;
                                data_units_assembly{corresponding_service} = default_data_unit_assembly;
                            end
                        end
                    end         % if (data_unit_indicator == 1)
                else            % (crc16_bytewise(packet) == 0)
                    CRC_error_counter = CRC_error_counter + 1;
                end             % (crc16_bytewise(packet) == 0)
            end                 % if (~isempty(corresponding_service))
            packet_buffer_index = packet_buffer_index + packet_length+3;
        end                     % while (packet_buffer_index <= length(packet_buffer))
        % data unit assembly finished (if there were data units)
        
    end                             % if (packet_mode == 1)
    
    MOT_object_counter = 0;
    MOT_objects = [];

    for data_unit_index = 1:data_unit_counter
        corresponding_service = data_units_assignment(data_unit_index);
        enhancement_flag = application_information.enhancement_flag(corresponding_service);
        application_domain = application_information.application_domain(corresponding_service);
        application_data = application_information.application_data{corresponding_service};
        user_application_type = application_information.user_application_type(corresponding_service);
        user_application_identifier = application_information.user_application_identifier(corresponding_service);
        
        if (application_domain == 0)        % DRM application domain: no application registered at the moment
            if (length(application_data) > 16)
                application_data = application_data(17:end);
            end
        elseif (application_domain == 1)    % DAB application domain
            application_data = application_data(17:end);
            % ETSI TS 101756 / 5.10
            if ((user_application_type == 2)|(user_application_type == 3))     % MOT Slideshow or Broadcast Web Site
                
                MSC_data_group_header = bytes2bits(data_units{data_unit_index}(1:4));
                next_data = 3 + 2*MSC_data_group_header(1);     % extension field?
                CRC_OK = 1;
                if (MSC_data_group_header(2))      % ETSI EN 300401 / 5.3.3.4
                    CRC_OK = double(crc16_bytewise(data_units{data_unit_index}) == 0);
                    CRC_OK_counter = CRC_OK_counter + CRC_OK;
                    CRC_error_counter = CRC_error_counter + 1 - CRC_OK;
                end
                    
                if (CRC_OK)
                    % user access field is recommended, including Transport ID: ETSI EN 301234 / 6.1
                    if (MSC_data_group_header(3) == 1)
                        segment_field = bytes2bits(data_units{data_unit_index}(next_data + [0:1]));
                        last_segment = segment_field(1);
                        segment_number = segment_field(2:16) * (2.^[14:-1:0])';
                        next_data = next_data + 2;
                    else
                        last_segment = 1;
                        segment_number = 0;
                    end
                    data_group_type = MSC_data_group_header(5:8) * (2.^[3:-1:0]');
                    continuity_index = MSC_data_group_header(9:12) * (2.^[3:-1:0]');
                    
                    % no consideration of repetition index yet
                    if (MSC_data_group_header(4) == 1) 
                        user_access_field = bytes2bits(data_units{data_unit_index}(next_data + [0:2]));
                        length_indicator = user_access_field(5:8) * (2.^[3:-1:0]');     % assume it's not different from 2 => no end user address field
                        next_data = next_data + 1 + length_indicator;
                        if ((length_indicator >= 2)&(user_access_field(4) == 1))
                            transport_ID = user_access_field(9:24) * (2.^[15:-1:0]');
                        else
                            transport_ID = -1;
                        end
                    else
                        transport_ID = -1;
                    end
                    
%                     if (transport_ID == 56817)
%                         keyboard
%                     end
                    
                    if ((data_group_type == 3)|(data_group_type == 4))
                        if (isempty(MOT_objects_assembly_information{corresponding_service}.transport_ID))
                            MOT_object_index = [];
                        else
                            MOT_object_index = find(MOT_objects_assembly_information{corresponding_service}.transport_ID == transport_ID);
                        end
                        if (isempty(MOT_object_index))
                            if (segment_number == 0)
                                number_of_MOT_objects = length(MOT_objects_assembly_information{corresponding_service}.transport_ID) + 1;
                                MOT_object_index = number_of_MOT_objects;
                                MOT_objects_assembly_information{corresponding_service}.transport_ID(MOT_object_index) = transport_ID;
                                MOT_objects_assembly{corresponding_service}{MOT_object_index} = default_MOT_object_assembly;
                                MOT_objects_assembly{corresponding_service}{MOT_object_index}.transport_ID = transport_ID;
                            else
                                MOT_object_index = 0;
                            end
                        else
                        end
                        
                        
                    end                        
                    
                    if (data_group_type == 3)               % MOT header data group
                        % assuming in order transmission:
                        % MOT object assembly:
                        % Trying to take ETSI 301234 / 6.3 into account
                        if (MOT_object_index > 0)
                            MOT_object_assembly = MOT_objects_assembly{corresponding_service}{MOT_object_index};
                            if (segment_number == 0)            % todo: check if this segment is already received
                                MOT_object_assembly.header_continuity_index = continuity_index;     
                                MOT_object_assembly.current_header_segment_no = 0;
                                segmentation_header = data_units{data_unit_index}(next_data + [0:1]); % don't care about that yet
                                MOT_object_assembly.header = data_units{data_unit_index}(next_data+2:end-2*MSC_data_group_header(2));
                                if (last_segment == 1)
                                    MOT_object_assembly.header_complete = 1;
                                end
                                MOT_objects_assembly{corresponding_service}{MOT_object_index} = MOT_object_assembly;
                            elseif ((MOT_object_assembly.current_header_segment_no + 1 == segment_number) &...
                                    (rem(MOT_object_assembly.header_continuity_index+1,16) == continuity_index))
                                MOT_object_assembly.header_continuity_index = MOT_object_assembly.header_continuity_index + 1;
                                MOT_object_assembly.current_header_segment_no = segment_number;
                                segmentation_header = data_units{data_unit_index}(next_data + [0:1]);
                                MOT_object_assembly.header = [MOT_object_assembly.header, data_units{data_unit_index}(next_data+2:end-2*MSC_data_group_header(2))];
                                if (last_segment == 1)
                                    MOT_object_assembly.header_complete = 1;
                                end
                                MOT_objects_assembly{corresponding_service}{MOT_object_index} = MOT_object_assembly;
                            end
                            
                        end
                        
                    elseif (data_group_type == 4)           % MOT body data group
                        if (MOT_object_index > 0)
                            MOT_object_assembly = MOT_objects_assembly{corresponding_service}{MOT_object_index};
                            if (segment_number == 0)        % todo: check if this segment is already received
                                MOT_object_assembly.body_continuity_index = continuity_index;   
                                MOT_object_assembly.current_body_segment_no = 0;
                                segmentation_header = data_units{data_unit_index}(next_data + [0:1]); % don't care about that yet
                                MOT_object_assembly.body = data_units{data_unit_index}(next_data+2:end-2*MSC_data_group_header(2));
                                if (last_segment == 1)
                                    MOT_object_assembly.body_complete = 1;
                                end
                                MOT_objects_assembly{corresponding_service}{MOT_object_index} = MOT_object_assembly;
                            elseif ((MOT_object_assembly.current_body_segment_no + 1 == segment_number))% &...
%                                    (rem(MOT_object_assembly.body_continuity_index+1,16) == continuity_index))
                                MOT_object_assembly.body_continuity_index = MOT_object_assembly.body_continuity_index + 1;
                                MOT_object_assembly.current_body_segment_no = segment_number;
                                segmentation_header = data_units{data_unit_index}(next_data + [0:1]);
                                MOT_object_assembly.body = [MOT_object_assembly.body, data_units{data_unit_index}(next_data+2:end-2*MSC_data_group_header(2))];
                                if (last_segment == 1)
                                    MOT_object_assembly.body_complete = 1;
                                end
                                MOT_objects_assembly{corresponding_service}{MOT_object_index} = MOT_object_assembly;
                            end
                            
                        end
                        
                    elseif (data_group_type == 5)           % MOT body data group, CA parameters
                        % not implemented
                    elseif (data_group_type == 6)           % MOT directory  
                        % only one directory/carousel per service: ETSI 301234 / 8.2.3
                        MOT_directory_assembly = MOT_directories_assembly{corresponding_service};
                        if (MOT_directory_assembly.transport_ID ~= transport_ID)
                            MOT_directory_assembly = default_MOT_directory_assembly;
                        end
                        if (segment_number == 0)        % todo: check if this segment is already received
                            MOT_directory_assembly.transport_ID = transport_ID;
                            MOT_directory_assembly.continuity_index = continuity_index;   
                            MOT_directory_assembly.current_segment_no = 0;
                            segmentation_header = data_units{data_unit_index}(next_data + [0:1]); % don't care about that yet
                            MOT_directory_assembly.body = data_units{data_unit_index}(next_data+2:end-2*MSC_data_group_header(2));
                            if (last_segment == 1)
                                MOT_directory_assembly.body_complete = 1;
                            end
                            MOT_directories_assembly{corresponding_service} = MOT_directory_assembly;
                        elseif ((MOT_directory_assembly.current_segment_no + 1 == segment_number) &...
                                (rem(MOT_directory_assembly.continuity_index+1,16) == continuity_index))
                            MOT_directory_assembly.continuity_index = MOT_directory_assembly.continuity_index + 1;
                            MOT_directory_assembly.current_segment_no = segment_number;
                            segmentation_header = data_units{data_unit_index}(next_data + [0:1]);
                            MOT_directory_assembly.body = [MOT_directory_assembly.body, data_units{data_unit_index}(next_data+2:end-2*MSC_data_group_header(2))];
                            if (last_segment == 1)
                                MOT_directory_assembly.body_complete = 1;
                            end
                            MOT_directories_assembly{corresponding_service} = MOT_directory_assembly;
                        end
                        
                        if (MOT_directory_assembly.body_complete == 1)
                            directory_header = bytes2bits(MOT_directory_assembly.body(1:13));
                            directory_size = directory_header(3:32) * (2.^[29:-1:0]');
                            number_of_objects = directory_header(33:48) * (2.^[15:-1:0]');
                            carousel_period = directory_header(49:72) * (2.^[23:-1:0]'); 
                            segment_size = directory_header(76:88) * (2.^[12:-1:0]');
                            directory_extension_length = directory_header(89:104) * (2.^[15:-1:0]');
                            directory_extension = MOT_directory_assembly.body(14 + [0:directory_extension_length-1]);
                            directory_entry_index = 14 + directory_extension_length;
                            entry_transport_IDs = [];
                            entry_number = 1;
                            while (directory_entry_index + 9 <= directory_size)
                                entry_transport_IDs(entry_number) = bytes2bits(MOT_directory_assembly.body(directory_entry_index + [0:1])) * (2.^[15:-1:0]');
                                MOT_object_index = find(MOT_objects_assembly_information{corresponding_service}.transport_ID == entry_transport_IDs(entry_number));
                                if (isempty(MOT_object_index))
                                    number_of_MOT_objects = length(MOT_objects_assembly_information{corresponding_service}.transport_ID) + 1;
                                    MOT_object_index = number_of_MOT_objects;
                                    MOT_objects_assembly_information{corresponding_service}.transport_ID(MOT_object_index) = entry_transport_IDs(entry_number);
                                    MOT_objects_assembly{corresponding_service}{MOT_object_index} = default_MOT_object_assembly;
                                    MOT_objects_assembly{corresponding_service}{MOT_object_index}.transport_ID = entry_transport_IDs(entry_number);
                                end
                                MOT_header_part = bytes2bits(MOT_directory_assembly.body(directory_entry_index + [2:7]));
                                header_size = MOT_header_part(29:41) * (2.^[12:-1:0]');
                                MOT_objects_assembly{corresponding_service}{MOT_object_index}.header = ...
                                    [MOT_directory_assembly.body(directory_entry_index + [2:1+header_size]), directory_extension];
                                MOT_objects_assembly{corresponding_service}{MOT_object_index}.header_complete = 1;
                                directory_entry_index = directory_entry_index + 2 + header_size;
                                entry_number = entry_number + 1;
                            end
                            % delete objects not contained in this
                            % directory because these objects are not
                            % complete and will not be completed
                            deprecated_objects = setdiff(MOT_objects_assembly_information{corresponding_service}.transport_ID, entry_transport_IDs);
                            for MOT_object_index = 1:length(deprecated_objects)
                                MOT_objects_assembly{corresponding_service}{MOT_object_index}.delete = 1;
                            end
                            
                            MOT_directories_assembly{corresponding_service}.body_complete = 0;
                        end
                    end
                    
                    % sort out completed or for deletion marked MOT objects
                    MOT_object_index = 1;
                    while MOT_object_index <=length(MOT_objects_assembly_information{corresponding_service}.transport_ID)
                        last_object = length(MOT_objects_assembly_information{corresponding_service}.transport_ID);
                        MOT_object_assembly = MOT_objects_assembly{corresponding_service}{MOT_object_index};
                        if ((MOT_object_assembly.header_complete == 1) & (MOT_object_assembly.body_complete == 1))
                            if (MOT_object_assembly.delete ~= 1)                % throw away objects not within directory
                                MOT_object_counter = MOT_object_counter + 1;
                                MOT_objects{MOT_object_counter} = default_MOT_object;
                                MOT_objects{MOT_object_counter}.header = MOT_object_assembly.header;
                                MOT_objects{MOT_object_counter}.body = MOT_object_assembly.body;
                                MOT_objects{MOT_object_counter}.ID = corresponding_service;
                            end
                            MOT_objects_assembly{corresponding_service} = ...
                                {MOT_objects_assembly{corresponding_service}{setdiff([1:last_object],MOT_object_index)}};
                            MOT_objects_assembly_information{corresponding_service}.transport_ID = ...
                                [MOT_objects_assembly_information{corresponding_service}.transport_ID(setdiff([1:last_object],MOT_object_index))];
                        else 
                            MOT_object_index = MOT_object_index + 1;
                        end
                        
                    end
                    
                end                         % if (CRC_OK)
                % MOT object assembly finished
            elseif (user_application_type == 1098)                              % Journaline(R) news service
                % DRM data unit corresponds to a DAB MSC data group with Journaline(R) news service
               
                application_data = application_information.application_data{corresponding_service};
                news_version = application_data(17:24)*(2.^[7:-1:0]');
                length_of_extended_header = application_data(25:32)*(2.^[7:-1:0]');
                
                news_objects = journaline_decode(data_units{data_unit_index}, length_of_extended_header);
                                
                
                
                service_name = sprintf('Service%i',corresponding_service);
                if (~exist (fullfile(DATA_STORAGE_PATH,service_name),'dir'))
                    mkdirp (fullfile(DATA_STORAGE_PATH,service_name));
                end
                for (news_index = 1:length(news_objects))
                    if (news_objects{news_index}.update == -1)
                        if (exist(news_objects{news_index}.filename))
                            delete (news_objects{news_index}.filename);
                        end
                    else
                        news_fid = fopen(fullfile(DATA_STORAGE_PATH,service_name,news_objects{news_index}.filename), 'wb');
                        if (news_fid >= 0)
                            if (matlab_version(1) >= 5.3)
                                fwrite (news_fid, news_objects{news_index}.filebody,'uchar');
                            else
                                fwrite (news_fid, double(news_objects{news_index}.filebody),'uchar');
                            end
                            fclose (news_fid); 
                            if (isequal(news_objects{news_index}.filename,'0000.html'))
                                news_fid = fopen(fullfile(DATA_STORAGE_PATH,service_name,'index.html'), 'wb');
                                if (matlab_version(1) >= 5.3)
                                    fwrite (news_fid, news_objects{news_index}.filebody,'uchar');
                                else
                                    fwrite (news_fid, double(news_objects{news_index}.filebody),'uchar');
                                end
                                fclose (news_fid); 
                            end
                        end
                    end
                end

            end                     % if ((user_application_type == 2)|(user_application_type == 3))      
        else    
            % reserved for future definition
        end                         % if (application_domain == xx)        
        
    end                 % for data_unit_index = 1:data_unit_counter
    
    % Saving complete MOT objects
    for MOT_object_index = 1:MOT_object_counter
        MOT_object = MOT_object_parsing(MOT_objects{MOT_object_index});
        service_name = ['Service',sprintf('%i',MOT_object.ID)];
        filename = MOT_object.content_name;

        
        % uncompress if compressed data
        if ((MOT_object.compression_type == 1)&(isequal(MOT_object.body(1:3),[31 139 8]))) % gzip deflate compression
            [uncompressed_data, alt_name, error] = deflate_uncompress(MOT_object.body);
            if (~isequal(alt_name,''))
                filename = alt_name;
            end
            if (length(uncompressed_data) == 1)
                uncompressed_data = MOT_object.body
            end
        else
            uncompressed_data = MOT_object.body;
         end
         
         if (filesep == '\')
             filename(find(filename == '/')) = '\';
         else
             filename(find(filename == '\')) = '/';
         end
         
        [path_to_MOT_file,MOT_file_name,MOT_file_ext,MOT_version]=fileparts(filename);
        
        if (~exist (fullfile(DATA_STORAGE_PATH,service_name,path_to_MOT_file),'dir'))
                mkdirp (fullfile(DATA_STORAGE_PATH,service_name,path_to_MOT_file));
        end
        
        MOT_fid = fopen(fullfile(DATA_STORAGE_PATH,service_name,path_to_MOT_file,[MOT_file_name,MOT_file_ext,MOT_version]), 'wb');
        if (MOT_fid >= 0)
            if (matlab_version(1) >= 5.3)
                fwrite (MOT_fid, uncompressed_data,'uchar');
            else 
                fwrite (MOT_fid, double(uncompressed_data),'uchar');
            end
            fclose (MOT_fid);
        end
    end
end                                 % for data_stream = stream_information.data_streams


% decoding error visualization 

if (CRC_error_counter == 0)
    show_signal_info(2,17,1);
elseif(CRC_OK_counter == 0)
    show_signal_info(2,17,2);
else
    show_signal_info(2,17,3);
end
