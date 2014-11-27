%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  University of Kaiserslautern, Institute of Communications Engineering     %
%%  Copyright (C) 2004 Torsten Schorr                                         %
%%                                                                            %
%%  Author(s)    : Torsten Schorr (schorr@eit.uni-kl.de)                      %
%%  Project start: 20.07.2004                                                 %
%%  Last change  : 23.07.2004                                                 %
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
%%  text_message_decoding.m                                                   %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  Description:                                                              %
%%  Assembly of audio stream text messages                                    %
%%                                                                            %
%%  Usage:                                                                    %
%%  text_message = text_message_decoding (text_message_bits, text_message);   %
%%  text_message: text_message structure to be assembled                      %
%%  text_message_bits: double bit stream at the end of audio stream           %
%%                                                                            %
%%  Invoked by source_decoding.m                                              %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function text_message = text_message_decoding (text_message_bits, text_message)

if (text_message.stream_no < 0)
    if (isequal(text_message_bits,ones(1,32)))
        text_message.stream_no = 0;
    end
elseif (text_message.stream_no == 0)
    text_message.current_toggle = text_message_bits(1);
    text_message.first_last_flag = text_message_bits(2:3)*(2.^[1:-1:0]');
    text_message.command_flag = text_message_bits(4);
    text_message.field1 = text_message_bits(5:8)*(2.^[3:-1:0]');
    text_message.field2 = text_message_bits(9:12)*(2.^[3:-1:0]');
    if (text_message.command_flag == 1)
        if  (text_message.field1 == 1)
            [text_message.segments{:}]= deal([]);
            text_message.string = '';
        end
        text_message.stream_no = -1;
    else
        text_message.current_segment_length = text_message.field1 + 1;
        text_message.stream_no = 1;
        if (text_message.first_last_flag <= 1)
            text_message.current_segment_no = text_message.field2;
        else
            text_message.current_segment_no = 0;
            [text_message.segments{:}]= deal([]);
            text_message.first_seg_received = 1;
        end
        text_message.current_segment = text_message_bits;
    end
elseif (text_message.stream_no > 0)
    bytes_to_receive = text_message.current_segment_length + 2 - ((text_message.stream_no-1)*4 + 2);
    if (bytes_to_receive > 4)
        text_message.current_segment = [text_message.current_segment, text_message_bits];
        text_message.stream_no = text_message.stream_no + 1;
    else
        text_message.current_segment = [text_message.current_segment, text_message_bits(1:bytes_to_receive*8)];
        text_message.stream_no = -1;
        if (~crc16([text_message.current_segment(1:end-16),1-text_message.current_segment(end-15:end)]))
            text_message.segments{text_message.current_segment_no+1} = ...
                (reshape(text_message.current_segment(17:end-16),8,text_message.current_segment_length)'*(2.^[7:-1:0]'))';
        else
            text_message.CRC_error = 1;
        end
        if (((text_message.first_last_flag == 1)&&(text_message.first_seg_received == 1)) || (text_message.first_last_flag == 3))
            text_message.string = '';
            for seg_no=1:8
                text_message.string = [text_message.string,char(text_message.segments{seg_no})];
            end 
            text_message.first_seg_received = 0;
        end            
    end
    
elseif (text_message.stream_no > 4)
    text_message.stream_no = -1;
end       
