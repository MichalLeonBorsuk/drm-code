%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  University of Kaiserslautern, Institute of Communications Engineering     %
%%  Copyright (C) 2005 Andreas Dittrich, Torsten Schorr                       %
%%                                                                            %
%%  Author(s)    : Andreas Dittrich (dittrich@eit.uni-kl.de)                  %
%%                 Torsten Schorr (schorr@eit.uni-kl.de)                      %
%%  Project start: 12.03.2004                                                 %
%%  Last change  : 16.03.2005                                                 %
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
%%  wavplayex.m                                                               %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  Description:                                                              %
%%  wavplayex allows continous sound playback                                 %
%%                                                                            %
%%  Usage:                                                                    %
%%  (1) wavplayex                                                             %
%%  (2) wavplayex( in, Fs [, r] )                                             %
%%  (3) playtime_delay_ms = wavplayex( in, Fs [, r] )                         %
%%  (4) [buffer_ms, delay_diff, delay_offset] =  wavplayex( in, Fs [, r] )    %
%%                                                                            %
%%  in = matrix with input data. Number of channel rows and as many cols as   %
%%       samples                                                              %
%%  Fs = sampling rate in Hz                                                  %
%%  r = resampling factor (optional argument)                                 %
%%  buffer_ms = time in ms, buffer was actually filled just before the call   %
%%              This value is zero at the first call. At consecutive calls,   %
%%              value is nonzero as long as there is data to play in the      %
%%              buffer.                                                       %
%%  delay_diff = no of samples which were added due to resampling             %
%%  delay_offset = fractional part of delay in samples due to resampling      %
%%                                                                            %
%%  A call without any argument stops all current playing (1).                %
%%  Use "in" as input data vector of either double or int16 type. "in" should %
%%  have two rows for stereo and one row for mono and as many columns as you  %
%%  have sound-samples to play. Calling wavplayex when sound is still         %
%%  playing adds the data to the play buffer and allows continous playing.    %
%%  Using sample rate conversion, applying the factor r results in conversion %
%%  of N input samples into N*r soundcard samples.                            %
%%  By integrating (adding) only the output values 'delay_diff' and adding    %
%%  'delay_offset' to the integration result, you allways have the actual     %
%%  delay in samples between input data stream and the resampled data stream. %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function varargout = wavplayex(varargin)

if (nargin>0)
	if ( (size(varargin{1},1)>2) & (size(varargin{1},2)==2) )
   	warning('input matrix has been transposed - wavplayex expects that audio data is a 2-by-N matrix for stereo');
   	varargin{1} = transpose(varargin{1});
	end
end

if (isequal(computer,'GLNX86')|isequal(computer,'LNX86'))
   if (nargin==0)
      wavio_linux(1);
   else
      [delay_ms, phase_diff, phase_offset] = wavio_linux(1, varargin{:});
   end
   
elseif(isequal(computer,'PCWIN'))
   if (nargin==0)
      wavplay_directx;
   else
      [delay_ms, phase_diff, phase_offset] = wavplay_directx(varargin{:});
   end
   
else
   error('computer system not supported');
   
end

if (nargin>0)
	if (nargout<=1)
	   varargout = {delay_ms};
	else
   	varargout = {delay_ms, phase_diff, phase_offset};
	end
end