%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  University of Kaiserslautern, Institute of Communications Engineering     %
%%  Copyright (C) 2004 Andreas Dittrich, Torsten Schorr                       %
%%                                                                            %
%%  Author(s)    : Andreas Dittrich (dittrich@eit.uni-kl.de)                  %
%%                 Torsten Schorr (schorr@eit.uni-kl.de)                      %
%%  Project start: 17.06.2004                                                 %
%%  Last change  : 17.06.2004                                                 %
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
%%  message.m                                                                 %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  Description:                                                              %
%%  Formatted conditional text output                                         %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function message( varargin )

persistent MESSAGE;
arg_ptr = 1;
print_flag = logical(1);

for arg_ptr = 1:nargin
    if ( isnumeric(varargin{arg_ptr})||islogical(varargin{arg_ptr}) )
        print_flag = varargin{arg_ptr};
    elseif (isequal(varargin{arg_ptr},'flush'))
        fprintf(1,'%s',char(MESSAGE));
        MESSAGE='';
    elseif (isequal(varargin{arg_ptr},'clear'))
        MESSAGE='';    
    elseif (ischar(varargin{arg_ptr})) 
        if (print_flag)
            MESSAGE= [MESSAGE,varargin{arg_ptr}];
        end
    else
        error('wrong data format');
    end
    
end

