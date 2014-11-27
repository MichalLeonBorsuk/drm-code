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
%%  wavrecordex.m                                                             %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  Description:                                                              %
%%  wavrecordex allows continous sound recording                              %
%%                                                                            %
%%  Usage:                                                                    %
%%  (1) wavrecordex                                                           %
%%  (2) samples = wavrecordex( N, Fs, Ch [,r])                                %
%%  (3) [samples, delay_diff, delay_offset] =  wavplayex( in, Fs [, r] )      %
%%                                                                            %
%%  Records N samples at sampling frequency Fs from ch channels.              %
%%  The return argument samples is a Ch x N - matrix.                         %
%%  A call without any argument stops all current recording and resets the    %
%%  internal recording buffer (1).                                            %
%%  Using sample rate conversion, applying the factor r results in            %
%%  conversion of N*r soundcard samples into N output samples.                %
%%  By integrating (adding) only the output values 'delay_diff' and adding    %
%%  'delay_offset' to the integration result, you allways have the actual     %
%%  delay in samples between resampled input data stream and the output data  %
%%  stream.                                                                   %
%%  The first call starts soundcard-recording and                             %
%%  returns when the requested number of samples are available. Recording     %
%%  is then still going on and the samples are buffered until the next call.  %
%%  If the recording buffer reaches some upper level (about ten seconds)      %
%%  recording is stopped and is only restarted at the next call. So once      %
%%  started the user should request new data as soon as possible. Because a   %
%%  call returns when data is available, you are automatically synchronized   %
%%  with the soundcard-clock.                                                 %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function varargout = wavrecordex(varargin)

if (isequal(computer,'GLNX86')||isequal(computer,'LNX86')||isequal(computer,'x86_64-pc-linux-gnu'))
   if (nargin==0)
      wavio_linux(0);
   else
      [y, phase_diff, phase_offset] = wavio_linux(0,varargin{:});
   end
   
elseif(isequal(computer,'PCWIN'))
   if (nargin==0)
      wavrecord_directx;
   else
      [y, phase_diff, phase_offset] = wavrecord_directx(varargin{:});
   end
   
else
   error('computer system not supported');
   
end
   

if (nargin>0)
   if (nargout<=1)
      varargout = {y};
	else
   	varargout = {y, phase_diff, phase_offset};
   end
end

