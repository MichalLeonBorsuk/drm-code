%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  University of Kaiserslautern, Institute of Communications Engineering     %
%%  Copyright (C) 2005 Torsten Schorr                                         %
%%                                                                            %
%%  Author(s)    : Torsten Schorr (schorr@eit.uni-kl.de)                      %
%%  Project start: 01.06.2004                                                 %
%%  Last change  : 28.04.2005                                                 %
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
%% Last change: 28.04.2005, 11:25                                             %
%% By         : Torsten Schorr                                                %
%% Description: handling of missing multiplex description                     %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  get_SDC_data.m                                                            %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  Description:                                                              %
%%  SDC data extraction                                                       %
%%                                                                            %
%%  Usage:                                                                    %
%%   [multiplex_description, application_information,                         %
%%    audio_information, time_and_date, updates] =                            %
%%            get_SDC_data (sdc_data,msc_mode, multiplex_description,         %
%%                          application_information, audio_information,       %
%%                          time_and_date);                                   %
%%                                                                            %
%%   multiplex_description: structure components:                             %
%%       stream_lengths: columns: different streams.                          %
%%                       row 1,2: length of part A, part B                    %
%%       PL_PartA: protection level of part A                                 %
%%       PL_PartB: protection level of part B                                 %
%%       HM_length: length of stream 0 in the case of HM, 0 otherwise         %
%%       PL_HM: protection level for hierarchical modulation                  %
%%   application_information: structure components:                           % 
%%       ID: service ID                                                       %
%%       stream_ID:                                                           %
%%       audio_coding:                                                        %
%%       SBR_flag:                                                            %
%%       audio_mode:                                                          %
%%       sampling_rate:                                                       %
%%       text_flag:                                                           %
%%       enhancement_flag:                                                    %
%%       coder_field:                                                         %
%%       ...                                                                  %
%%   audio_information: structure components:                                 %
%%       ID: service ID                                                       %
%%       stream_ID:                                                           %
%%       packet_mode:                                                         %
%%       data_unit_indicator:                                                 %
%%       packet_ID:                                                           %
%%       enhancement_flag:                                                    %
%%       application_domain:                                                  %
%%       packet_length:                                                       %
%%       application_data:                                                    %
%%       user_application_type:                                               %
%%       user_application_identifier:                                         %
%%   time_and_date: structure components: day, month, year, hours, minutes    %
%%                                                                            %
%%  Invoked by channel_decoding.m                                             %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function [multiplex_description, application_information, audio_information, time_and_date, updates] = ...
                get_SDC_data (sdc_data, msc_mode, multiplex_description, application_information, audio_information, time_and_date)
% information extraction:
AFS = sdc_data(1:4)*[8;4;2;1];
SDCi = 5;

updates = zeros (1,16);

while (SDCi + 12 <= length(sdc_data))
    length_of_body = sdc_data(SDCi:SDCi+6)*(2.^[6:-1:0]');
    SDCi = SDCi + 7;
    version_flag = sdc_data(SDCi);
    SDCi = SDCi + 1;
    data_entity_type = sdc_data(SDCi:SDCi+3)*(2.^[3:-1:0]');
    SDCi = SDCi + 4;
    
    if (SDCi + 4 + 8 * length_of_body > length(sdc_data))
        break;
    end
    
    updates(data_entity_type + 1) = updates(data_entity_type + 1) + 1;
    
    switch data_entity_type
        
    case 0      % Multiplex description data entity - type 0: every SDC block
       			if (length_of_body==0) | (SDCi + 28 > length(sdc_data)) | (SDCi + 4 + 8 * length_of_body > length(sdc_data))
                    break;
                end
                
                multiplex_description.PL_PartA = sdc_data(SDCi:SDCi + 1)*(2.^[1:-1:0]');
                multiplex_description.PL_PartB = sdc_data(SDCi + 2:SDCi + 3)*(2.^[1:-1:0]');
                SDCi = SDCi + 4;
                % Number of streams: length_of_body/3
                if (msc_mode == 0 | msc_mode == 3)
                   % No hierarchical modulation
                   multiplex_description.stream_lengths = ...
                       [sdc_data(SDCi:SDCi + 11)*(2.^[11:-1:0]');sdc_data(SDCi+12:SDCi + 23)*(2.^[11:-1:0]')];
                   multiplex_description.HM_length = 0;
                   multiplex_description.PL_HM = 0;
                else
                   % Hierarchical modulation
                   multiplex_description.stream_lengths = ...
                       [0;sdc_data(SDCi+12:SDCi + 23)*(2.^[11:-1:0]')];
                   multiplex_description.PL_HM = sdc_data(SDCi:SDCi + 1)*(2.^[1:-1:0]');
                   multiplex_description.HM_length = sdc_data(SDCi+12:SDCi + 23)*(2.^[11:-1:0]');
                end
                for i = [1:length_of_body/3 - 1]
                   multiplex_description.stream_lengths = [multiplex_description.stream_lengths,...
                           [sdc_data(SDCi + 24 * i:SDCi + 24 * i + 11)*(2.^[11:-1:0]');sdc_data(SDCi + 24 * i + 12:SDCi + 24 * i + 23)*(2.^[11:-1:0]')]];
                end
                SDCi = SDCi + 8 * length_of_body;
    case 1      % Label data entity - type 1: every SDC block
                if (SDCi + 4 > length(sdc_data)) | (SDCi + 4 + 8 * length_of_body > length(sdc_data))
                    break;
                end
                ID = sdc_data(SDCi:SDCi + 1)*(2.^[1:-1:0]');
                SDCi = SDCi + 4;
                application_information.label{ID+1} = char((reshape(sdc_data(SDCi:SDCi + 8 * length_of_body - 1),8,length_of_body)'*(2.^[7:-1:0]'))');
                SDCi = SDCi + 8 * length_of_body;
    case 5      % Application information data entity - type 5: as required
                if (SDCi + 12 > length(sdc_data)) | (SDCi + 4 + 8 * length_of_body > length(sdc_data))
                    break;
                end
                ID = sdc_data(SDCi:SDCi + 1)*(2.^[1:-1:0]');
                application_information.ID(ID+1) = ID;
                application_information.stream_ID(ID+1) =  sdc_data(SDCi + 2:SDCi + 3)*(2.^[1:-1:0]');
                SDCi = SDCi + 4;
                application_information.packet_mode(ID+1) = sdc_data(SDCi);
                if (application_information.packet_mode(ID+1) == 1) % packet mode
                    if (SDCi + 16 > length(sdc_data))
                        break;
                    end
                    application_information.data_unit_indicator(ID+1) = sdc_data(SDCi+1);
                    application_information.packet_ID(ID+1) = sdc_data(SDCi+2:SDCi+3)*(2.^[1:-1:0]');
                    application_information.enhancement_flag(ID+1) = sdc_data(SDCi+4);
                    application_information.application_domain(ID+1) = sdc_data(SDCi+5:SDCi+7)*(2.^[2:-1:0]');
                    application_information.packet_length(ID+1) = sdc_data(SDCi+8:SDCi+15)*(2.^[7:-1:0]');
                    application_information.application_data{ID+1} = sdc_data(SDCi+16:SDCi + 8 * length_of_body - 1);
                else                                          % synchrounous stream mode
                    application_information.enhancement_flag(ID+1) = sdc_data(SDCi+4);
                    application_information.application_domain(ID+1) = sdc_data(SDCi+5:SDCi+7)*(2.^[2:-1:0]');
                    application_information.application_data{ID+1} = sdc_data(SDCi+8:SDCi + 8 * length_of_body - 1);
                end                
                SDCi = SDCi + 8 * length_of_body;  
    case 8      % Time and date information data entity - type 8: once per minute
                if (SDCi + 28 > length(sdc_data)) | (length_of_body ~= 3)
                    break;
                end
                [time_and_date.day,time_and_date.month,time_and_date.year] = MJD2Gregorian(sdc_data(SDCi:SDCi + 16)*(2.^[16:-1:0]'));
                time_and_date.hours = sdc_data(SDCi+17:SDCi + 21)*(2.^[4:-1:0]');
                time_and_date.minutes = sdc_data(SDCi+22:SDCi + 27)*(2.^[5:-1:0]');
                SDCi = SDCi + 4 + 8 * length_of_body;            
    case 9      % Audio information data entity - type 9: every SDC block
                if (SDCi + 20 > length(sdc_data)) | (length_of_body ~= 2)
                    break;
                end
                ID = sdc_data(SDCi:SDCi + 1)*(2.^[1:-1:0]');
                audio_information.ID(ID+1) = ID; % service ID        
                audio_information.stream_ID(ID+1) = sdc_data(SDCi + 2:SDCi + 3)*(2.^[1:-1:0]'); % stream ID
                SDCi = SDCi + 4;
                audio_information.audio_coding(ID+1) = sdc_data(SDCi:SDCi + 1)*(2.^[1:-1:0]');
                audio_information.SBR_flag(ID+1) = sdc_data(SDCi + 2);
                audio_information.audio_mode(ID+1) = sdc_data(SDCi + 3:SDCi + 4)*(2.^[1:-1:0]');
                audio_information.sampling_rate(ID+1) = sdc_data(SDCi + 5:SDCi + 7)*(2.^[2:-1:0]');
                audio_information.text_flag(ID+1) = sdc_data(SDCi + 8);
                audio_information.enhancement_flag(ID+1) = sdc_data(SDCi + 9);
                audio_information.coder_field(ID+1) = sdc_data(SDCi + 10:SDCi + 14)*(2.^[4:-1:0]');
                SDCi = SDCi + 8 * length_of_body;             
    case 12     % Language and country data entity - type 12: standard
                if (SDCi + 44 > length(sdc_data)) | (length_of_body ~= 5)
                    break;
                end
                ID = sdc_data(SDCi:SDCi + 1)*(2.^[1:-1:0]');
                SDCi = SDCi + 4;
                application_information.language{ID+1} = char((reshape(sdc_data(SDCi:SDCi + 23),8,3)'*(2.^[7:-1:0]'))');
                application_information.country{ID+1} = char((reshape(sdc_data(SDCi + 24:SDCi + 39),8,2)'*(2.^[7:-1:0]'))');
                SDCi = SDCi + 8 * length_of_body;
              
     otherwise
                SDCi = SDCi + 4 + 8 * length_of_body;          
     end
 end

 
