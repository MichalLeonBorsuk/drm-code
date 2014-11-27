%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  University of Kaiserslautern, Institute of Communications Engineering     %
%%  Copyright (C) 2005 Torsten Schorr                                         %
%%                                                                            %
%%  Author(s)    : Torsten Schorr (schorr@eit.uni-kl.de)                      %
%%  Project start: 30.07.2004                                                 %
%%  Last change  : 19.01.2005                                                 %
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
%%  MOT_object_parsing.m                                                      %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  Description:                                                              %
%%  Parsing of MOT objects: header interpretation and structure filling       %
%%                                                                            %
%%  Usage:                                                                    %
%%  MOT_object = MOT_object_parsing(MOT_object);                              %
%%  MOT_object: MOT object structure to be filled                             %
%%                                                                            %
%%  Invoked by source_decoding.m                                              %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function MOT_object = MOT_object_parsing(MOT_object)
%   ETSI EN 301234 / 5

matlab_version=[sscanf(version,'%f')];
        
% header parsing:

header_core = bytes2bits(MOT_object.header(1:7));
body_size = header_core(1:28) * (2.^[27:-1:0]');
header_size = header_core(29:41) * (2.^[12:-1:0]');
MOT_object.content_type = header_core(42:47) * (2.^[5:-1:0]');
MOT_object.content_subtype = header_core(48:56) * (2.^[8:-1:0]');

% header extension parsing:
header_extension = bytes2bits(MOT_object.header(8:end));
extension_index = 1;
while (extension_index+7 <= length(header_extension))
    PLI = header_extension(extension_index + [0:1]) * (2.^[1:-1:0]');
    Param_ID = header_extension(extension_index + [2:7]) * (2.^[5:-1:0]');
    if (PLI == 0)       % reserved for future use
        extension_index = extension_index + 8;
        continue;
    elseif (PLI == 1)
        data_field = header_extension(extension_index + [8:15]);
        extension_index = extension_index + 16;
    elseif (PLI == 2)
        data_field = header_extension(extension_index + [8:39]);
        extension_index = extension_index + 40;
    else % (PLI == 3)
        if (header_extension(extension_index + 8) == 0)
            data_field_length = header_extension(extension_index + [9:15]) * (2.^[6:-1:0]');
            data_field = header_extension(extension_index + 16 + [0:data_field_length*8-1]);
            extension_index = extension_index + 16 + data_field_length*8;
        else
            data_field_length = header_extension(extension_index + [9:23]) * (2.^[14:-1:0]');
            data_field = header_extension(extension_index + 24 + [0:data_field_length*8-1]);
            extension_index = extension_index + 24 + data_field_length*8;
        end
    end

    if (Param_ID == 2)          % Creation Time
        MOT_object.creation_time = data_field;  % to be extended
    elseif (Param_ID == 3)      % Start Validity
        MOT_object.start_validity = data_field; % to be extended
    elseif (Param_ID == 4)      % Expire Time
        MOT_object.expire_time = data_field;    % to be extended
    elseif (Param_ID == 5)      % Trigger Time
        MOT_object.trigger_time = data_field;   % to be extended
    elseif (Param_ID == 6)      % Version Number
        MOT_object.version_number = bits2bytes(data_field(1:8));        % to be extended
    elseif (Param_ID == 7)      % Re-transmission Distance
        MOT_object.repetition_distance = bits2bytes (data_field(9:32)); % to be extended
    elseif (Param_ID == 8)      % Group Reference
        MOT_object.group_reference = data_field;                        % to be extended
    elseif (Param_ID == 10)     % Priority
        MOT_object.priority = bits2bytes(data_field(1:8));              % to be extended
    elseif (Param_ID == 11)     % Label
        character_set_indicator = data_field(1:4) * (2.^[3:-1:0]');
        character_field = bits2bytes (data_field(9:136));
        character_flag_field = data_field(137:152);
        if (matlab_version(1) >= 5.3)
            MOT_object.label = char(character_field);
        else 
            MOT_object.label = char(double(character_field));
        end                                                             % to be extended
    elseif (Param_ID == 12)     % Content Name
        character_set_indicator = data_field(1:4) * (2.^[3:-1:0]');
        character_field = bits2bytes (data_field(9:end));
        if (matlab_version(1) >= 5.3)
            MOT_object.content_name = char(character_field);
        else 
            MOT_object.content_name = char(double(character_field));
        end                                                             % to be extended
    elseif (Param_ID == 15)     % Content Description
        character_set_indicator = data_field(1:4) * (2.^[3:-1:0]');
        character_field = bits2bytes (data_field(9:end));
        if (matlab_version(1) >= 5.3)
            MOT_object.content_description = char(character_field);
        else 
            MOT_object.content_description = char(double(character_field));
        end                                                             % to be extended
    elseif (Param_ID == 16)     % Mime Type
        if (matlab_version(1) >= 5.3)
            MOT_object.mime_type = char(bits2bytes (data_field));
        else 
            MOT_object.mime_type = char(double(bits2bytes (data_field)));
        end                                                             % to be extended
    elseif (Param_ID == 17)     % Compression Type
        MOT_object.compression_type = bits2bytes(data_field);                    % to be extended    
    elseif (Param_ID == 63)
        % Application Specific
    else
        % reserved for future use
    end
    
end