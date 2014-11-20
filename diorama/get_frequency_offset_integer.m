%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  University of Kaiserslautern, Institute of Communications Engineering     %
%%  Copyright (C) 2004 Andreas Dittrich                                       %
%%                                                                            %
%%  Author(s)    : Andreas Dittrich (dittrich@eit.uni-kl.de)                  %
%%  Project start: 27.05.2004                                                 %
%%  Last change  : 29.06.2004                                                 %
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
%%  get_frequency_offset_integer.m                                            %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  Description:                                                              %
%%  Estimation of the integer-part of the frequency offset, i.e. search of    %
%%  frequency pilots                                                          %
%%                                                                            %
%%  Invoked by demodulation_and_equalization.m                                %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function freq_offset_integer = get_frequency_offset_integer( symbol_buffer, N_symbols, K_dc, K_modulo, Tu );

S = reshape( symbol_buffer(1:N_symbols*K_modulo), K_modulo, N_symbols );
S = S(1:Tu,:);

% accumulate weighted phase differences for all carrier
dS_sum = zeros(Tu,1);
for (t=2:N_symbols)
   %dS_sum = dS_sum + S(:,t-1).*conj(S(:,t));
   dS_sum = dS_sum + exp( j*angle(S(:,t-1).*conj(S(:,t))) );
end

%detect carrier position of pilots
k_pilot1 = 18*Tu/288; 
k_pilot2 = 54*Tu/288; 
k_pilot3 = 72*Tu/288;
         
abs_dS_sum = abs(dS_sum);
pilot_indicator = [abs_dS_sum((k_pilot1+1):end);abs_dS_sum(1:k_pilot1)] + ...
            [abs_dS_sum((k_pilot2+1):end);abs_dS_sum(1:k_pilot2)] + ...
				[abs_dS_sum((k_pilot3+1):end);abs_dS_sum(1:k_pilot3)];
         
[dummy, K_dc_offset] = max( pilot_indicator );
% todo: check if detection is reliable

K_dc_offset = rem( K_dc_offset - K_dc + Tu/2 + Tu, Tu ) - Tu/2;	% shift index of dc-carrier into interval [-Tu/2,Tu/2) 
freq_offset_integer = 2*pi*K_dc_offset;


if (0)	%for debugging only
	%plot pilot-indicator-signal
	figure(234);
	plot( abs(dS_sum), '.-' );
	title('Frequency Pilot Detection');
	xlabel('frequency (Hz)');
	ylabel('abs( ... )');
end
      	
