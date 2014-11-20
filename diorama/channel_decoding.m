%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  University of Kaiserslautern, Institute of Communications Engineering     %
%%  Copyright (C) 2005 Torsten Schorr                                         %
%%                                                                            %
%%  Author(s)    : Torsten Schorr (schorr@eit.uni-kl.de)                      %
%%  Project start: 27.05.2004                                                 %
%%  Last change  : 28.04.2005                                                 %
%%  Changes      : ||                                                         %
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
%%  Last change: 27.04.2005                                                   %
%%  By         : Torsten Schorr                                               %
%%  Description: included show_signal_info                                    %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Last change: 28.04.2005, 11:15                                             %
%% By         : Torsten Schorr                                                %
%% Description: handling of missing multiplex description                     %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  channel_decoding.m                                                        %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  Description:                                                              %
%%  Super-frame-synchronization, OFDM demapping, deinterleaving               %
%%  and channel decoding of FAC, SDC and MSC frames                           %
%%                                                                            %
%%  To be used after demodulation_and_equalization.m                          %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function channel_decoding()

%*******************************************************************************
%* constant global variables                                                   *
%*******************************************************************************

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

global robustness_mode;	% -1 or 0 = unknown, 1 = mode A, 2 = mode B ...
global spectrum_occupancy;	% -1 = unknown, 0..5
global fac_valid;
global sdc_data_valid;

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

% channel_decoding --> source_decoding
global channel_decoded_data_buffer;
global channel_decoded_data_buffer_data_valid;

% channel_decoding --> source_decoding --> output_data_writing
global multiplex_description
global application_information
global audio_information
global stream_information
global time_and_date

global gui_parameters
global nongui_parameters

%*******************************************************************************
%* static variables                                                            *
%*******************************************************************************
persistent MSC_mode_strings SDC_mode_strings bandwith_numbers interleaving_numbers FAC_Deinterleaver ...
           RX RY RYlcmSM16 RYlcmSM64 RYlcmHMsym64 RYlcmHMmix64 RatesSM16 RatesSM64 RatesHMsym64 ...
           RatesHMmixR RatesHMmixI Length_SDC_data_field_matrix frame_index enough_frames frame_count ...
           msc_parameters_valid robustness_mode_old sdc_mode msc_mode interleaver_depth z_SDC ...
           Tu_list FAC_Demapper SDC_Demapper MSC_Demapper ...
           L_SDC SDC_Deinterleaver SDC_PL SDC_data_length SNR_estimation_valid N1 L Lvspp ...
           Deinterleaver PL squared_noise_signal_buffer MSC_carriers_usage MSC_used_carriers ...
           signal_to_noise_ratio noise_power_density MSC_Demapper_symbolwise min_index_equal_samples ...
           max_index_equal_samples
           
CHANNELDECODING = gui_parameters{17};           
SHOW_SIGNAL_INFO = gui_parameters{20};
PRINTTIME = nongui_parameters{10};
VERBOSE_LEVEL = 3 - gui_parameters{14};
PLOT_SNR_SPECTRUM = gui_parameters{5};
PLOT_CONSTELLATIONS = gui_parameters{3};
ENABLE_SERVICE1 = gui_parameters{9};
ENABLE_SERVICE2 = gui_parameters{10};
ENABLE_SERVICE3 = gui_parameters{11};
ENABLE_SERVICE4 = gui_parameters{12};
SOFT_MSD = nongui_parameters{3};
MSD_ITER = 5 - gui_parameters{15};
audio_services = gui_parameters{31};

if (run_state == RUN_STATE_POWERON)
    
    MSC_mode_strings = {'64-QAM SM','64-QAM HMmix','64-QAM HM','16-QAM SM'};
    SDC_mode_strings = {'16-QAM','4-QAM'};
    Tu_list = [288, 256, 176, 112];
    bandwith_numbers = [4.5,5,9,10,18,20];
    interleaving_numbers = [2, 0.4];
    FAC_Deinterleaver = int32(deinterleaver(0,1,130,21));
    RX = [1 3 1 4 1 4 3 2 8 3 4 7 8];
    RY = [4 10 3 11 2 7 5 3 11 4 5 8 9];
    RYlcmSM16 = [3 4];
    RYlcmSM64 = [4 15 8 45];
    RYlcmHMsym64 = [10 11 56 9];
    RYlcmHMmix64 = [20 165 56 45];   
    RatesSM16 = [3 8;5 10];
    RatesSM64 = [1 5 10;3 8 11;5 10 12;8 11 13];
    RatesHMsym64 = [5 2 7;6 4 9;7 6 12;8 8 13];
    RatesHMmixR = [5 2 7;6 4 9;7 6 12;8 8 13];
    RatesHMmixI = [1 5 10;3 8 11;5 10 12;8 11 13];
    Length_SDC_data_field_matrix = [37 43 85 97 184 207;17 20 41 47 91 102;...
                                    28 33 66 76 143 161;13 15 32 37 70 79;...
                                    0 0 0 68 0 147;0 0 0 32 0 72;0 0 0 33 0 78;...
                                    0 0 0 15 0 38];
    return;
    
elseif (run_state == RUN_STATE_INIT)
    
    transmission_frame_buffer_writeptr = 1;
    frame_index = 1;
    enough_frames = 0;
    frame_count = 0;
    msc_parameters_valid = 0;
    robustness_mode_old = -1;
    sdc_mode = -1;
    msc_mode = -1;
    interleaver_depth = -1;
    z_SDC = [0 + 0.000000000001*j];
    fac_valid = -1;
    multiplex_description = struct ('stream_lengths', [], 'PL_PartA', [], 'PL_PartB', [], 'HM_length', 0, 'PL_HM',[]);
    plot_constellations(7);
    show_signal_info(2,17,0);
    show_signal_info(2,18,0);
    show_signal_info(2,19,0);
    show_signal_info(2,15,NaN);
    show_signal_info(2,16,NaN);
    return;    
    
end

channel_decoded_data_buffer_data_valid = 0;

if (transmission_frame_buffer_data_valid == 0) 
    frame_index = 1;
    enough_frames = 0;
    frame_count = 0;
    msc_parameters_valid = 0;
    transmission_frame_buffer_writeptr = 1;
    
    multiplex_description = struct ('stream_lengths', [], 'PL_PartA', [], 'PL_PartB', [], 'HM_length', 0, 'PL_HM',[]);
    
    audio_information = struct ('ID',[0 0 0 0], 'stream_ID', [-1 -1 -1 -1], 'audio_coding',[0 0 0 0],'SBR_flag',[0 0 0 0],'audio_mode',[0 0 0 0],...
                            'sampling_rate',[0 0 0 0],'text_flag',[0 0 0 0],'enhancement_flag',[0 0 0 0],'coder_field',[0 0 0 0], ...
                            'bytes_per_frame', [0 0 0 0]);
                        
    application_information = struct ('ID', [0 0 0 0], 'stream_ID', [-1 -1 -1 -1], 'packet_mode', [0 0 0 0], 'data_unit_indicator', [0 0 0 0],...
                                  'packet_ID', [0 0 0 0], 'enhancement_flag', [0 0 0 0], 'application_domain', [0 0 0 0], ...
                                  'packet_length', [0 0 0 0], 'application_data', [], 'user_application_type', [0 0 0 0], ...
                                  'user_application_identifier', [0 0 0 0], 'label', {{'','','',''}}, 'language', {{'','','',''}}, ...
                                  'country', {{'','','',''}}, 'language_code', [0, 0, 0, 0], 'programme_type_code', [0, 0, 0, 0], ...
                                  'bytes_per_frame', [0 0 0 0]);
    
    stream_information = struct ('number_of_audio_services', 0, 'number_of_data_services', 0, 'number_of_streams', 0, ...
                                 'number_of_audio_streams', 0, 'number_of_data_streams', 0, 'audio_streams', [], ...
                                 'data_streams', [], 'audio_services', [], 'data_services', []);                             

    time_and_date = struct('day', -1,'month', -1,'year', -1,'hours', -1,'minutes', -1);
    
    sdc_mode = -1;
    msc_mode = -1;
    robustness_mode_old = -1;
    interleaver_depth = -1;
    squared_noise_signal_buffer = zeros( (110 + 350 + 1)*15*2*3, 1 );
    noise_power_density = zeros((110 + 350 + 1),1);
    SNR_estimation_valid = 0;
    fac_valid = -1;
    sdc_data_valid = 0;
    audio_service_index = 1;
    
    gui_parameters{31} = 'N/A';
    if (~isequal(settings_handler(4, 31),gui_parameters{31}))
       settings_handler(7, 31, gui_parameters{31});
    end
    
    gui_parameters{30} = audio_service_index;
    settings_handler(7, 30, gui_parameters{30});

    show_signal_info(2,17,0);
    show_signal_info(2,18,0);
    show_signal_info(2,19,0);
    show_signal_info(2,15,NaN);
    show_signal_info(2,16,NaN);
    return;
end

if (~CHANNELDECODING) 
    frame_index = mod(frame_index, 6) + 1;
    if (exist('symbol_period') & exist('symbols_per_frame'))
        transmission_frame_buffer_writeptr = rem (transmission_frame_buffer_writeptr - 1 + symbol_period * symbols_per_frame, symbol_period * symbols_per_frame * 6) + 1;
    end
    SNR_estimation_valid = 0;
    sdc_data_valid = 0;
    return; 
end

iterations = 0;
calc_variance = -0.05;
msc_parameters_changed = 0;
sdc_parameters_changed = 0;

if (robustness_mode ~= robustness_mode_old)
    if (robustness_mode <= 0)
        return;
    end
    symbol_period = Tu_list(robustness_mode);
    FAC_Demapper = Create_FAC_Demapper(robustness_mode,K_dc, K_modulo);
    
end
%channel_transfer_function_buffer = ones( (110 + 350 + 1)*15*2*3, 1 );	% is filled by demodulation_and_equalization


%%%%% FAC-Decoding: %%%%%%%%%%%%%

fac_valid = 1;

z_FAC = transmission_frame_buffer((frame_index-1) * symbol_period * symbols_per_frame + FAC_Demapper );
transfer_function_FAC = channel_transfer_function_buffer((frame_index-1) * symbol_period * symbols_per_frame + FAC_Demapper );
plot_constellations(4, z_FAC);
z_FAC(10)=0;
fac_data = msd_hard(z_FAC,abs(transfer_function_FAC), 0, [0 72], 0, FAC_Deinterleaver, [0 6], 2,0);
channel_parameters = fac_data(1:20);
service_parameters = fac_data(21:64);

if (crc8([fac_data(1:64),1-fac_data(65:72)])) 
    spectrum_occupancy = -1;
    fac_valid = 0;
    message(PRINTTIME <= VERBOSE_LEVEL,sprintf('%5.0fms - FAC decoding, FAC CRC error\n', toctic * 1000));
    show_signal_info(2,19,2);
    return;
 end
 show_signal_info(2,19,1);
 
identity = rem(channel_parameters(2:3)*[2;1],3);
% frame alignment:
if (identity ~= rem(frame_index - 1,3))
    old_ptr = transmission_frame_buffer_writeptr;
    transmission_frame_buffer_writeptr = identity * symbol_period * symbols_per_frame + 1;
    transmission_frame_buffer(transmission_frame_buffer_writeptr:transmission_frame_buffer_writeptr - 1 + symbol_period * symbols_per_frame) =...
        transmission_frame_buffer(old_ptr:old_ptr - 1 + symbol_period * symbols_per_frame);
    channel_transfer_function_buffer(transmission_frame_buffer_writeptr:transmission_frame_buffer_writeptr - 1 + symbol_period * symbols_per_frame) =...
        channel_transfer_function_buffer(old_ptr:old_ptr - 1 + symbol_period * symbols_per_frame);    
    frame_index = identity + 1;
end

interleaver_depth_new = channel_parameters(8);
msc_mode_new = channel_parameters(9:10)*[2;1];
spectrum_occupancy_new = channel_parameters(4:7)*[8;4;2;1];
sdc_mode_new = channel_parameters(11);      
number_of_services = channel_parameters(12:15)*[8;4;2;1];

if (spectrum_occupancy_new > 5 | number_of_services == 11 | number_of_services == 14) 
    spectrum_occupancy = -1;
    fac_valid = 0;
    message(PRINTTIME <= VERBOSE_LEVEL,sprintf('%5.0fms - FAC decoding, FAC consistence error\n', toctic * 1000));
    return;
end

%     fprintf(1,'| Channel Parameters        |   |\n');
%     fprintf(1,'+---------------------------+---+\n');
%     fprintf(1,'| Base/Enhancement flag:    | %i |\n',channel_parameters(1));
%     fprintf(1,'| Identity:                 | %i |\n',channel_parameters(2:3)*[2;1]);
%     fprintf(1,'| Spectrum occupancy:       | %i |\n',channel_parameters(4:7)*[8;4;2;1]);
%     fprintf(1,'| Interleaver depth flag:   | %i |\n',channel_parameters(8));
%     fprintf(1,'| MSC mode:                 | %i |\n',channel_parameters(9:10)*[2;1]);
%     fprintf(1,'| SDC mode:                 | %i |\n',channel_parameters(11));
%     fprintf(1,'| Number of Services        | %i |\n',channel_parameters(12:15)*[8;4;2;1]);
%     fprintf(1,'| Reconfiguration index     | %i |\n',channel_parameters(16:18)*[4;2;1]);
%     fprintf(1,'| Rfu:                      | %i |\n',channel_parameters(19:20)*[2;1]);
%     fprintf(1,'+---------------------------+---+\n');

if ((spectrum_occupancy ~= spectrum_occupancy_new) | (robustness_mode_old ~= robustness_mode))
    spectrum_occupancy = spectrum_occupancy_new;
    SDC_Demapper = Create_SDC_Demapper(robustness_mode, spectrum_occupancy, K_dc, K_modulo);
    interleaver_depth = interleaver_depth_new;
    MSC_Demapper = Create_MSC_Demapper(robustness_mode, spectrum_occupancy, interleaver_depth, K_dc, K_modulo);
    MSC_Demapper_symbolwise = rem(MSC_Demapper-1,symbol_period) + 1;
    MSC_carriers_usage = hist(MSC_Demapper_symbolwise(1:end)-1, [0:symbol_period - 1]);
    MSC_used_carriers = find(MSC_carriers_usage ~= 0);
    msc_parameters_changed = 1;
    sdc_parameters_changed = 1;
    
    show_signal_info(2,1, char(64+robustness_mode));
    show_signal_info(2,2, bandwith_numbers(spectrum_occupancy + 1));
    show_signal_info(2,3, interleaver_depth);
    
elseif (interleaver_depth ~= interleaver_depth_new)
    interleaver_depth = interleaver_depth_new;
    MSC_Demapper = Create_MSC_Demapper(robustness_mode, spectrum_occupancy, interleaver_depth, K_dc, K_modulo);
    MSC_Demapper_symbolwise = rem(MSC_Demapper-1,symbol_period) + 1;
    MSC_carriers_usage = hist(MSC_Demapper_symbolwise(1:end)-1, [0:symbol_period - 1]);
    MSC_used_carriers = find(MSC_carriers_usage ~= 0);
    msc_parameters_changed = 1;
    sdc_parameters_changed = 1;
    
    show_signal_info(2,1, char(64+robustness_mode));
    show_signal_info(2,2, bandwith_numbers(spectrum_occupancy + 1));
    show_signal_info(2,3, interleaver_depth);

end

robustness_mode_old = robustness_mode;

interleaver_depth = interleaver_depth_new;
% frame count: deinterleaving possible after 2 received frames for short and after 6 received frames for long interleaving, respectively
frame_count = frame_count + 1;
if (frame_count >= 6 - 4 * interleaver_depth)
    enough_frames = 1;
elseif (frame_count == 1)
    min_index_equal_samples = transmission_frame_buffer_writeptr;
    max_index_equal_samples = transmission_frame_buffer_writeptr - 1 + symbol_period * symbols_per_frame;
else
    if (transmission_frame_buffer_writeptr < min_index_equal_samples)
        min_index_equal_samples = transmission_frame_buffer_writeptr;
    end
    if (transmission_frame_buffer_writeptr - 1 + symbol_period * symbols_per_frame > max_index_equal_samples)
        max_index_equal_samples = transmission_frame_buffer_writeptr - 1 + symbol_period * symbols_per_frame;
    end
end

if (msc_mode ~= msc_mode_new)
    msc_mode = msc_mode_new;
    msc_parameters_changed = 1;

    show_signal_info(2,5, MSC_mode_strings{msc_mode+1});
end

if (sdc_mode ~= sdc_mode_new)
    sdc_mode = sdc_mode_new;
    sdc_parameters_changed = 1;
    
    show_signal_info(2,4, SDC_mode_strings{sdc_mode+1});
end

application_information.language_code(service_parameters(25:26)*(2.^[1:-1:0]') + 1) = service_parameters(28:31)*(2.^[3:-1:0]');
if (service_parameters(32) == 0)
    application_information.programme_type_code(service_parameters(25:26)*(2.^[1:-1:0]') + 1) = service_parameters(33:37)*(2.^[4:-1:0]');
end


%%%%% SDC-Decoding: %%%%%%%%%%%%%

N_SDC = length (SDC_Demapper);
% MSC parameter assignment: sizes, interleavers, puncturing
if (sdc_parameters_changed)
    SDC_data_length = Length_SDC_data_field_matrix((robustness_mode-1)*2+sdc_mode+1,spectrum_occupancy+1);
    if (sdc_mode == 0) 
        SDC_rates = [3 8];
        L_SDC = [[0;0],(RX(SDC_rates) .* floor((2 * N_SDC - 12)./RY(SDC_rates)))'];
        SDC_Deinterleaver = int32 ([deinterleaver(0,1,2*N_SDC,13),deinterleaver(0,1,2*N_SDC,21)]);
        SDC_PL = [[1;1],SDC_rates.'] - 1;
    elseif (sdc_mode == 1) 
        SDC_rates = [5];
        L_SDC = [[0],(RX(SDC_rates) .* floor((2 * N_SDC - 12)./RY(SDC_rates)))'];
        SDC_Deinterleaver = int32 (deinterleaver(0,1,2*N_SDC,21));
        SDC_PL = [[1],SDC_rates.'] - 1;
    end

end

if (identity == 0) % SDC cells only in the first frame of a superframe
    z_SDC = transmission_frame_buffer((frame_index-1) * symbol_period * symbols_per_frame + SDC_Demapper );
    transfer_function_SDC = channel_transfer_function_buffer((frame_index-1) * symbol_period * symbols_per_frame + SDC_Demapper );
    plot_constellations(5, z_SDC, sdc_mode);
    
    sdc_data = msd_hard(z_SDC, abs(transfer_function_SDC), 0, L_SDC, 0, SDC_Deinterleaver, SDC_PL, 2, 1);
    
    % check crc: 4-bit AFS index is coded as 8-bit field with 4 msbs = 0; Robustness mode A, spectrum occupancy 2:
    % 85 Bytes = 680 bits data field, followed by crc-16
    % ETSI ES 201980 / 6.4.2
    if (~crc16([0,0,0,0,sdc_data(1:SDC_data_length * 8 + 4),1-sdc_data(SDC_data_length * 8 + 5:SDC_data_length * 8 + 20)]))
        show_signal_info(2,18,1);
        [multiplex_description_new, application_information, audio_information, time_and_date, sdc_updates] = ...
            get_SDC_data (sdc_data(1:SDC_data_length * 8 + 4), msc_mode, multiplex_description, application_information, audio_information, time_and_date);
        
        if (sdc_updates(1) & (~isequal(multiplex_description_new, multiplex_description)))
            msc_parameters_changed = 1;
            msc_parameters_valid = 1;
            multiplex_description = multiplex_description_new;
            if (sum(multiplex_description.stream_lengths(1,:)) == 0)
                show_signal_info(2,6,[-1, multiplex_description.PL_PartB] );
            else
                show_signal_info(2,6,[multiplex_description.PL_PartA, multiplex_description.PL_PartB] );
            end
        end
        sdc_data_valid = 1;
        %print_SDC_data (sdc_data,msc_mode)
    else
        show_signal_info(2,18,2);
        message(PRINTTIME <= VERBOSE_LEVEL,sprintf('%5.0fms - SDC decoding, SDC CRC error\n', toctic * 1000));
    end
    
end

if (sdc_data_valid)
    stream_information.number_of_audio_services = floor(number_of_services / 4);
    stream_information.number_of_data_services = number_of_services - 4*stream_information.number_of_audio_services;
    if (number_of_services == 0)
        stream_information.number_of_audio_services = 4;
        stream_information.number_of_data_services = 0;
    elseif (number_of_services == 15)
        stream_information.number_of_audio_services = 0;
        stream_information.number_of_data_services = 4;
    end
    
    
    stream_information.number_of_streams = size(multiplex_description.stream_lengths,2);
    stream_information.number_of_audio_streams = stream_information.number_of_audio_services;
    stream_information.number_of_data_streams = stream_information.number_of_streams - stream_information.number_of_audio_streams;
    stream_information.audio_streams = setdiff(audio_information.stream_ID, -1);
    stream_information.data_streams = setdiff(application_information.stream_ID, -1);
    stream_information.audio_services = find(audio_information.stream_ID ~= -1);
    stream_information.data_services = find(application_information.stream_ID ~= -1);
    
    show_signal_info(2,7,[stream_information.number_of_audio_services, stream_information.number_of_data_services] );
    
    [dummy, data_service_pos] = unique(application_information.packet_ID(stream_information.data_services));
    stream_information.data_services = stream_information.data_services(data_service_pos);
    DATA_SERVICES = find([ENABLE_SERVICE1, ENABLE_SERVICE2, ENABLE_SERVICE3, ENABLE_SERVICE4]);
    stream_information.data_services = intersect(DATA_SERVICES,stream_information.data_services);
    
    if (~isequal(gui_parameters{31}, stream_information.audio_services))
        if (~isempty(stream_information.audio_services))
            % choose first audio service withh AAC audio stream
            AAC_services = find(audio_information.audio_coding(stream_information.audio_services)==0);
            
            if (~isempty(AAC_services))
                audio_service_index = AAC_services(1);
            else
                audio_service_index = 1;
            end
            
            gui_parameters{31} = stream_information.audio_services;
            
        else
            audio_service_index = 1;
            gui_parameters{31} = 'N/A';
        end
        settings_handler(7, 31, gui_parameters{31});
        gui_parameters{30} = audio_service_index;
        settings_handler(7, 30, gui_parameters{30});
        
    end
end

message(PRINTTIME <= VERBOSE_LEVEL,sprintf('%5.0fms - FAC/SDC decoding, mode %s, %2.1fkHz, SDC: %s, MSC: %s, %1.1fs interleaving\n', toctic * 1000, ...
    char(64+robustness_mode), bandwith_numbers(spectrum_occupancy + 1), SDC_mode_strings{sdc_mode+1}, MSC_mode_strings{msc_mode+1},interleaving_numbers(interleaver_depth + 1)));

%%%% MSC Decoding: %%%%%%%%%%

N_MUX = size(MSC_Demapper, 2);
% MSC parameter assignment: sizes, interleavers, puncturing
if (msc_parameters_changed & msc_parameters_valid)
    if (msc_mode == 0)  % 64-QAM SM
        rylcm = RYlcmSM64(multiplex_description.PL_PartA + 1);
        ratesA = RatesSM64(multiplex_description.PL_PartA + 1,:);
        N1 = ceil(8 * sum(multiplex_description.stream_lengths(1,:))/(2 * rylcm * sum(RX(ratesA)./RY(ratesA)))) * rylcm;
        N2 = N_MUX - N1;
        ratesB = RatesSM64(multiplex_description.PL_PartB + 1,:);
        L = [2 * N1 * (RX(ratesA) ./ RY(ratesA))',(RX(ratesB) .* floor((2 * N2 - 12)./RY(ratesB)))'];
        Lvspp = 0;
        xin1 = 2 * N1;
        xin2 = 2 * N2;
        Deinterleaver = int32([[0:xin1+xin2-1].',deinterleaver(xin1,13,xin2,13),deinterleaver(xin1,21,xin2,21)]);
        PL = [ratesA.',ratesB.'] - 1;
    elseif (msc_mode == 1) % 64-QAM HMmix
        rylcm = RYlcmHMmix64(multiplex_description.PL_PartA + 1);
        ratesAR = RatesHMmixR(multiplex_description.PL_PartA + 1,:);
        ratesAI = RatesHMmixI(multiplex_description.PL_PartA + 1,:);
        N1 = ceil(8 * sum(multiplex_description.stream_lengths(1,2:end))/(rylcm * (RX(ratesAI(1))./RY(ratesAI(1)) + sum(RX(ratesAR(2:end))./RY(ratesAR(2:end)) + RX(ratesAI(2:end))./RY(ratesAI(2:end))) ) )) * rylcm;
        N2 = N_MUX - N1;
        ratesAR(1) = RatesHMmixR(multiplex_description.PL_HM + 1,1);
        ratesBR = RatesHMmixR(multiplex_description.PL_PartB + 1,:);
        ratesBI = RatesHMmixI(multiplex_description.PL_PartB + 1,:);
        ratesBR(1) = RatesHMmixR(multiplex_description.PL_HM + 1,1);
        L = [N1 * (RX(ratesAR) ./ RY(ratesAR))',(RX(ratesBR) .* floor((N2 - 12)./RY(ratesBR)))', N1 * (RX(ratesAI) ./ RY(ratesAI))',(RX(ratesBI) .* floor((N2 - 12)./RY(ratesBI)))'];
        Lvspp = RX(ratesBR(1)) .* floor((N_MUX - 12)./RY(ratesBR(1)));
        L(1,1) = 0;
        L(1,2) = Lvspp;
        xin1 = N1;
        xin2 = N2;
        Deinterleaver = int32([[0:xin1+xin2-1].',deinterleaver(xin1,13,xin2,13),deinterleaver(xin1,21,xin2,21)]);
        PL = [ratesAR.',ratesBR.',ratesAI.',ratesBI.'] - 1;
        
    elseif (msc_mode == 2) % 64-QAM HMsym
        rylcm = RYlcmHMsym64(multiplex_description.PL_PartA + 1);
        ratesA = RatesHMsym64(multiplex_description.PL_PartA + 1,:);
        N1 = ceil(8 * sum(multiplex_description.stream_lengths(1,2:end))/(2 * rylcm * sum(RX(ratesA(2:end))./RY(ratesA(2:end))))) * rylcm;
        N2 = N_MUX - N1;
        ratesA(1) = RatesHMsym64(multiplex_description.PL_HM + 1,1);
        ratesB = RatesHMsym64(multiplex_description.PL_PartB + 1,:);
        ratesB(1) = RatesHMsym64(multiplex_description.PL_HM + 1,1);
        L = [2 * N1 * (RX(ratesA) ./ RY(ratesA))',(RX(ratesB) .* floor((2 * N2 - 12)./RY(ratesB)))'];
        Lvspp = RX(ratesB(1)) .* floor((2 * N_MUX - 12)./RY(ratesB(1)));
        L(1,1) = 0;
        L(1,2) = Lvspp;
        xin1 = 2 * N1;
        xin2 = 2 * N2;
        Deinterleaver = int32([[0:xin1+xin2-1].',deinterleaver(xin1,13,xin2,13),deinterleaver(xin1,21,xin2,21)]);
        PL = [ratesA.',ratesB.'] - 1;
    elseif (msc_mode == 3) % 16-QAM SM
        rylcm = RYlcmSM16(multiplex_description.PL_PartA + 1);
        ratesA = RatesSM16(multiplex_description.PL_PartA + 1,:);
        N1 = ceil(8 * sum(multiplex_description.stream_lengths(1,:))/(2 * rylcm * sum(RX(ratesA)./RY(ratesA)))) * rylcm;
        N2 = N_MUX - N1;
        ratesB = RatesSM16(multiplex_description.PL_PartB + 1,:);
        L = [2 * N1 * (RX(ratesA) ./ RY(ratesA))',(RX(ratesB) .* floor((2 * N2 - 12)./RY(ratesB)))'];
        Lvspp = 0;
        xin1 = 2 * N1;
        xin2 = 2 * N2;
        Deinterleaver = int32([deinterleaver(xin1,13,xin2,13),deinterleaver(xin1,21,xin2,21)]);
        PL = [ratesA.',ratesB.'] - 1;
  
    end
end

if (msc_parameters_valid)
    
    z_MSC = transmission_frame_buffer(MSC_Demapper(frame_index,:));
    transfer_function_MSC = channel_transfer_function_buffer(MSC_Demapper(frame_index,:));
    
    if (~enough_frames)
        
               
        transfer_function_MSC(find(MSC_Demapper(frame_index,:) > max_index_equal_samples | MSC_Demapper(frame_index,:) < min_index_equal_samples)) = 0;
        
        
        SNR_estimation = abs(transfer_function_MSC);        % assuming white noise
        
        if (~SOFT_MSD)
            [SPPhard, VSPPhard, iterations, calc_variance, noise_signal] = msd_hard(z_MSC.', SNR_estimation, N1, L, Lvspp, Deinterleaver, PL, MSD_ITER, 1);
        else
            error ('Soft decoding not yet implemented!');
        end    
        
        channel_decoded_data_buffer_data_valid = 2;
        
    else
        
        plot_constellations(6, z_MSC, msc_mode);
        
            
        if (SNR_estimation_valid < 1)
            SNR_estimation = abs(transfer_function_MSC);        % assuming white noise
        else
            %SNR_estimation = abs(transfer_function_MSC);
            %SNR_estimation = sqrt(signal_to_noise_ratio(MSC_Demapper_symbolwise(frame_index,:)) / 100);
            %SNR_estimation = signal_to_noise_ratio(MSC_Demapper_symbolwise(frame_index,:)) / 100;
            noise_power_density(find(noise_power_density == 0)) = 1;
            SNR_estimation = abs(transfer_function_MSC) ./ sqrt(noise_power_density(MSC_Demapper_symbolwise(frame_index,:)));
        end
        
        if (~SOFT_MSD)
            [SPPhard, VSPPhard, iterations, calc_variance, noise_signal] = msd_hard(z_MSC.', SNR_estimation, N1, L, Lvspp, Deinterleaver, PL, MSD_ITER, 1);
        else
            error ('Soft decoding not yet implemented!');
        end
        
        squared_noise_signal = abs(noise_signal).^2;
        calc_weighted_variance = sum((abs(transfer_function_MSC).^2)' .* squared_noise_signal)/(sum(abs(transfer_function_MSC).^2) * mean_energy_of_used_cells_list((robustness_mode-1)*6 + spectrum_occupancy + 1));
        %noise_signal_buffer = zeros( (110 + 350 + 1)*15*2*3, 1 );
        squared_noise_signal_buffer(MSC_Demapper(frame_index,:)) = squared_noise_signal;
        weighted_noise_power_density = sum(reshape(squared_noise_signal_buffer(1:symbol_period * symbols_per_frame * 6),symbol_period, symbols_per_frame * 6),2)';
        noise_power_positions = find(weighted_noise_power_density ~= 0);
        signal_to_noise_ratio = zeros(1,symbol_period);
        signal_to_noise_ratio(noise_power_positions) = MSC_carriers_usage(noise_power_positions) ./ weighted_noise_power_density(noise_power_positions);
        
        samples_resorted = zeros (symbol_period, symbols_per_frame);
        samples_resorted(rem(MSC_Demapper(frame_index,:),symbol_period * symbols_per_frame)) = squared_noise_signal .* abs(transfer_function_MSC.').^2;
        noise_power_density(MSC_used_carriers) = noise_power_density(MSC_used_carriers) * (1 - 0.2) +  0.2 * sum(samples_resorted(MSC_used_carriers,:),2)./MSC_carriers_usage(MSC_used_carriers)';

        if (SNR_estimation_valid < 1)
            SNR_estimation_valid = SNR_estimation_valid + 1;
        end
        
        if (PLOT_SNR_SPECTRUM)
            plot_snr_spectrum(2, signal_to_noise_ratio, MSC_used_carriers, symbol_period, K_dc);
        end
        
        channel_decoded_data_buffer_data_valid = 1;
        
    end 
    
    if (length(VSPPhard) ~= 0)
        VSPPlength = sum(multiplex_description.stream_lengths(:,1)) * 8;
        HPPlength = sum(multiplex_description.stream_lengths(1,:)) * 8;
        channel_decoded_data_buffer(1:VSPPlength + length(SPPhard)) = [SPPhard(1:HPPlength), VSPPhard(1:VSPPlength), SPPhard(HPPlength + 1:end)];
    else
        channel_decoded_data_buffer(1:length(SPPhard)) = [SPPhard];
    end
end
 
frame_index = mod(frame_index, 6) + 1;    
transmission_frame_buffer_writeptr = rem (transmission_frame_buffer_writeptr - 1 + symbol_period * symbols_per_frame, symbol_period * symbols_per_frame * 6) + 1;

if (PLOT_CONSTELLATIONS)
   plot_constellations(2);
end

if (enough_frames & msc_parameters_valid)
    
    message(PRINTTIME <= VERBOSE_LEVEL,sprintf('%5.0fms - MSC decoding, ',toctic*1000));
    message(1 <= VERBOSE_LEVEL,sprintf('frame# %d - iter: %d - MSC MER: %2.1f dB - MSC WMER: %2.1f dB\n', frame_count, iterations, -10*log10(calc_variance), -10*log10(calc_weighted_variance)));
    show_signal_info(2,15,-10*log10(calc_variance));
    show_signal_info(2,16,-10*log10(calc_weighted_variance));
    
else
    
    message(PRINTTIME <= VERBOSE_LEVEL,sprintf('%5.0fms - MSC decoding, ', toctic*1000));
    message(1 <= VERBOSE_LEVEL,sprintf('interleaving delay - frame# %d\n', frame_count));
    show_signal_info(2,15,NaN);
    show_signal_info(2,16,NaN);
    
end
