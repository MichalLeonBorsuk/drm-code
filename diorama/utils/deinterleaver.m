%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  University of Kaiserslautern, Institute of Communications Engineering     %
%%  Copyright (C) 2005 Torsten Schorr                                         %
%%                                                                            %
%%  Author(s)    : Torsten Schorr (schorr@eit.uni-kl.de)                      %
%%  Project start: 01.06.2004                                                 %
%%  Last change  : 28.02.2005                                                 %
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
%%  deinterleaver.m                                                           %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  Description:                                                              %
%%  Deinterleaver/Interleaver generation for DRM frames                       %
%%  Usage:                                                                    %
%%                                                                            %
%%  [deinterleaver, interleaver] = deinterleaver(xinA,tA,xinB,tB);            %
%%                                                                            %
%%  deinterleaver and interleaver according to ETSI ES 201 980 7.3.3          %
%%  as double vectors. Overall interleaver for part A and part B              %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


function varargout = deinterleaver(xinA,tA,xinB,tB)

deinterl = zeros(xinA + xinB,1);

if (xinA < 0)
    error('deinterleaver: xinA must be >= 0!');
end
if (tA < 1)
    error('deinterleaver: tA must be a natural number!');
end
if (xinB < 6)
    error('deinterleaver: xinB must be >= 6!');
end
if (tB < 1)
    error('deinterleaver: tB must be a natural number!');
end

if (xinA == 0)
    sA = 0;
else
    sA = 2^(ceil(log2(xinA)));
end
qA = sA/4 - 1;
if (xinB == 0)
    sB = 0;
else
    sB = 2^(ceil(log2(xinB)));
end
qB = sB/4 - 1;

deinterl(1) = 0;
PIofi = 0;
for i = 1:xinA-1 
    PIofi = rem((tA * PIofi + qA),sA);
    while (PIofi >= xinA)
        PIofi = rem((tA * PIofi + qA),sA);
    end
    deinterl(PIofi + 1) = i;
end
deinterl(xinA + 1) = xinA;
PIofi = 0;
for i = 1:xinB-1
    PIofi = rem((tB * PIofi + qB),sB);
    while (PIofi >= xinB)
        PIofi = rem((tB * PIofi + qB),sB);
    end
    deinterl(PIofi + xinA + 1) = i + xinA;
end

if (nargout == 1)
    varargout = {deinterl};
else
    interl = zeros(xinA + xinB,1);
    interl(deinterl + 1) = [0:xinA + xinB - 1];
    varargout = {deinterl, interl};
end