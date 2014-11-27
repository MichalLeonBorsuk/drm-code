%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  University of Kaiserslautern, Institute of Communications Engineering     %
%%  Copyright (C) 2004 Torsten Schorr                                         %
%%                                                                            %
%%  Author(s)    : Torsten Schorr (schorr@eit.uni-kl.de)                      %
%%  Project start: 26.05.2004                                                 %
%%  Last change  : 22.07.2004                                                 %
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
%%  print_SDC_data.m                                                          %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  Description:                                                              %
%%  SDC data presentation                                                     %
%%                                                                            %
%%  Usage:                                                                    %
%%  printSDCdata (sdc_data,msc_mode);                                         %
%%                                                                            %
%%  Can be used like get_SDC_data                                             %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function print_SDC_data (sdc_data,msc_mode)


audio_coding = ['AAC ';'CELP';'HVXC';'----'];
audio_sample_rate = [8, 12, 16, 24, -1, -1, -1, -1];

% information extraction:
AFS = sdc_data(1:4)*[8;4;2;1];
fprintf('\n\nService Description Channel:\n\nAFS-Index: %d\n\n',AFS);
SDCi = 5;

while (SDCi + 12 <= length(sdc_data))
    length_of_body = sdc_data(SDCi:SDCi+6)*(2.^[6:-1:0]');
    SDCi = SDCi + 7;
    version_flag = sdc_data(SDCi);
    SDCi = SDCi + 1;
    data_entity_type = sdc_data(SDCi:SDCi+3)*(2.^[3:-1:0]');
    SDCi = SDCi + 4;
    
    switch data_entity_type
        
    case 0
       			 if length_of_body==0
                    break;
                end
                fprintf('Multiplex description data entity - type 0:\n');
                fprintf('\tprotection level for part A: %d\n\tprotection level for part B: %d\n',sdc_data(SDCi:SDCi + 1)*(2.^[1:-1:0]'),sdc_data(SDCi + 2:SDCi + 3)*(2.^[1:-1:0]'));
                SDCi = SDCi + 4;
                fprintf('\tthere are %d stream(s)\n',length_of_body / 3);
                N_partA = sdc_data(SDCi:SDCi + 11)*(2.^[11:-1:0]');
                N_partB = sdc_data(SDCi+12:SDCi + 23)*(2.^[11:-1:0]');
                if (msc_mode == 0 | msc_mode == 3)
                   fprintf('\tNo hierarchical modulation used\n');
                   fprintf('\tdata length for part A of stream 0: %d bytes\n',N_partA);
                   fprintf('\tdata length for part B of stream 0: %d bytes\n',N_partB);
                else
                   fprintf('\tHierarchical modulation is used\n');
                   fprintf('\tprotection level of hierarchical: %d\n',N_partA);
                   fprintf('\tdata length for hierarchical: %d\n',N_partB);
                end
                
                for i = [1:length_of_body/3 - 1]
                   fprintf('\tdata length for part A of stream %d: %d bytes\n', i, sdc_data(SDCi + 24 * i:SDCi + 24 * i + 11)*(2.^[11:-1:0]'));
                   fprintf('\tdata length for part B of stream %d: %d bytes\n', i, sdc_data(SDCi + 24 * i + 12:SDCi + 24 * i + 23)*(2.^[11:-1:0]'));
                end
                fprintf('\n');
                SDCi = SDCi + 8 * length_of_body;
    case 1
                fprintf('Label data entity - type 1:\n');
                fprintf('\tShort ID: %d, rfu: %d\n',sdc_data(SDCi:SDCi + 1)*(2.^[1:-1:0]'),sdc_data(SDCi + 2:SDCi + 3)*(2.^[1:-1:0]'));
                SDCi = SDCi + 4;
                fprintf('\tLabel: %s\n\n',char((reshape(sdc_data(SDCi:SDCi + 8 * length_of_body - 1),8,length_of_body)'*(2.^[7:-1:0]'))'));
                SDCi = SDCi + 8 * length_of_body;
    case 2
                fprintf('Conditional access parameters data entity - type 2:\n');
                fprintf('\tShort ID: %d, rfu: %d\n',sdc_data(SDCi:SDCi + 1)*(2.^[1:-1:0]'),sdc_data(SDCi + 2:SDCi + 3)*(2.^[1:-1:0]'));
                SDCi = SDCi + 4;
                fprintf('\tCA system identifier: %d. %d bytes of CA system specific information\n\n',sdc_data(SDCi:SDCi+7)*(2.^[7:-1:0]'),length_of_body - 1);
                SDCi = SDCi + 8 * length_of_body;
    case 5
                fprintf('Application information data entity - type 5:\n');
                fprintf('\tShort ID: %d, Stream ID: %d\n',sdc_data(SDCi:SDCi + 1)*(2.^[1:-1:0]'),sdc_data(SDCi + 2:SDCi + 3)*(2.^[1:-1:0]'));
                SDCi = SDCi + 4;
                packet_mode_indicator = sdc_data(SDCi);
                if (packet_mode_indicator)
                    fprintf('\tPacket mode\n\tdata unit indicator: %d\n\tpacket ID: %d\n\tenhancement flag: %d\n\tapplication domain: %d\n\tpacket length: %d\n\n',sdc_data(SDCi+1),sdc_data(SDCi+2:SDCi+3)*(2.^[1:-1:0]'),sdc_data(SDCi+4),sdc_data(SDCi+5:SDCi+7)*(2.^[2:-1:0]'),sdc_data(SDCi+8:SDCi+15)*(2.^[7:-1:0]'));
                else
                    fprintf('\tSynchronous stream mode\n\n');
                end                
                SDCi = SDCi + 8 * length_of_body;                        
    case 8                
                fprintf('Time and date information data entity - type 8:\n');
                if (SDCi + 28 > length(sdc_data)) | (length_of_body ~= 3)
                    break;
                end

                [time_and_date.day,time_and_date.month,time_and_date.year] = MJD2Gregorian(sdc_data(SDCi:SDCi + 16)*(2.^[16:-1:0]'));
                time_and_date.hours = sdc_data(SDCi+17:SDCi + 21)*(2.^[4:-1:0]');
                time_and_date.minutes = sdc_data(SDCi+22:SDCi + 27)*(2.^[5:-1:0]');
                fprintf('\tDate: %04d-%02d-%02d    Time (UTC): %02d:%02d\n\n', time_and_date.year, time_and_date.month, time_and_date.day, time_and_date.hours, time_and_date.minutes);
                SDCi = SDCi + 4 + 8 * length_of_body;            
    case 9
                fprintf('Audio information data entity - type 9:\n');
                fprintf('\tShort ID: %d, Stream ID: %d\n',sdc_data(SDCi:SDCi + 1)*(2.^[1:-1:0]'),sdc_data(SDCi + 2:SDCi + 3)*(2.^[1:-1:0]'));
                SDCi = SDCi + 4;
                fprintf('\tAudio Coding: %s\n\tSBR-flag: %d\n\tAudio mode: %d\n\tAudio sampling rate: %dkHz\n',audio_coding(sdc_data(SDCi:SDCi + 1)*(2.^[1:-1:0]')+1,:),sdc_data(SDCi + 2),sdc_data(SDCi + 3:SDCi + 4)*(2.^[1:-1:0]'),audio_sample_rate(sdc_data(SDCi + 5:SDCi + 7)*(2.^[2:-1:0]')+1));
                if (sdc_data(SDCi + 8)==1)
                   fprintf('\ta text message is carried\n');
                else
                   fprintf('\tno text message is carried\n');
                end
                if (sdc_data(SDCi + 9)==1)
                   fprintf('\tenhancement available\n\n');
                else
                   fprintf('\tno enhancement available\n');
                end
                fprintf('\tcoder field: %d, rfa = %d\n\n',sdc_data(SDCi + 10:SDCi + 14)*(2.^[4:-1:0]'),sdc_data(SDCi + 15));
                SDCi = SDCi + 8 * length_of_body;             
     case 12 
                fprintf('Language and country data entity - type 12:\n');
                fprintf('\tShort ID: %d, rfu: %d\n',sdc_data(SDCi:SDCi + 1)*(2.^[1:-1:0]'),sdc_data(SDCi + 2:SDCi + 3)*(2.^[1:-1:0]'));
                SDCi = SDCi + 4;
                fprintf('\tLanguage code of target audience (ISO 639-2): %s\n',char((reshape(sdc_data(SDCi:SDCi + 8 * 3 - 1),8,3)'*(2.^[7:-1:0]'))'));
                fprintf('\tCountry code of service origin (ISO 3166): %s\n\n',char((reshape(sdc_data(SDCi + 8 * 3:SDCi + 8 * 5 - 1),8,2)'*(2.^[7:-1:0]'))'));
                SDCi = SDCi + 8 * length_of_body;
               
     otherwise
                fprintf('Data entity - type %d:\n', data_entity_type);
                SDCi = SDCi + 4;
                fprintf('\tLength of body: %d\n\n',length_of_body);
                SDCi = SDCi + 8 * length_of_body;          
     end
 end
  
 fprintf ('\n\n');
 