%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  University of Kaiserslautern, Institute of Communications Engineering     %
%%  Copyright (C) 2004 Andreas Dittrich                                       %
%%                                                                            %
%%  Author(s)    : Andreas Dittrich (dittrich@eit.uni-kl.de)                  %
%%  Project start: 27.05.2004                                                 %
%%  Last change: 27.04.2005, 17:30                                            %
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
%%  Last change: 27.04.2005, 17:30                                            %
%%  By         : Andreas Dittrich                                             %
%%  Description: modified timing sync algorithm                               %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  get_ofdm_symbol.m                                                         %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  Description:                                                              %
%%  Outputs one OFDM-Symbol, which is time- and frequency synchronized.       %
%%  This module has an internal state for doing the time-/frequency           %
%%  synchronization in a control-loop. Fractional time-synchronization is     %
%%  made in the frequency domain.                                             %
%%                                                                            %
%%  Invoked by demodulation_and_equalization.m                                %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function [out, delta_time_offset_integer, time_offset_fractional, freq_offset, delta_time_offset_I, phi_freq_correction_last, symbol_position_offset] = ...
   get_ofdm_symbol( in, Ts, Tu, H, delta_freq_offset, time_offset_fractional, freq_offset, delta_time_offset_I, phi_freq_correction_last)

global TIME_SYNC_ENABLE FREQ_SYNC_ENABLE

%some constants
Tg = Ts - Tu;
Tgh = floor( Tg/2 + 0.5 );
Tc = 2^ceil( log2( length(H) ) );
Tch = ceil(Tc/2);
Tgs = floor(Tg/Tu*Tc);
Tghs = floor(Tg/Tu/2*Tc);

% some fixed parameters
kP_small_timing_controller = 0.005;	%todo: calculate some better constants
kP_large_timing_controller = 0.01;
threshold_timing_small_large = Tgh;
kI_timing_controller = 0.000005;
max_delta_theta = Tgh;
max_delta_time_offset_integer = 3;
max_symbol_position_offset = Tgh-max_delta_time_offset_integer;
kP_freq_controller = 0.01;	%todo: calculate some better constants

if (TIME_SYNC_ENABLE == 1)
	%estimate time offset
	h_absq = abs( fft( [transpose(H),zeros(1,Tc-length(H))] ) ).^2;	%note thate h_absq is mirrored in time direction because we use fft instead of ifft
	[dummy, delta_theta] = max(   filter( sin([1:Tgs]/(Tgs+1)*pi).^(0.001), 1, [h_absq,h_absq] ) );	%find a window with maximum energy inside
	delta_theta =  ( rem( Tc + Tghs + 1 - delta_theta + Tc*1.5, Tc ) - Tc/2 )*Tu/Tc;
	delta_theta = max(min(delta_theta,max_delta_theta),-max_delta_theta);

	% filter theta: P-I-controler
	time_offset_ctrl = delta_theta - time_offset_fractional;	% integer part is corrected from outside this routine (=delay of input samples)
	delta_time_offset_I = delta_time_offset_I + kI_timing_controller*time_offset_ctrl;
	delta_time_offset_P = kP_large_timing_controller*time_offset_ctrl + ...
	   threshold_timing_small_large*(kP_small_timing_controller-kP_large_timing_controller)*tanh(time_offset_ctrl/threshold_timing_small_large); 
	delta_time_offset = delta_time_offset_P + delta_time_offset_I + time_offset_fractional;
	delta_time_offset_integer = floor( delta_time_offset + 0.5 );
	delta_time_offset_integer = min( max( delta_time_offset_integer, -max_delta_time_offset_integer ), max_delta_time_offset_integer );	%only +/- one symbol 
	time_offset_fractional = delta_time_offset - delta_time_offset_integer;

	%get best time window (without affecting channelestimation)
	symbol_position_offset = floor( delta_theta - delta_time_offset_integer + 0.5 );
	symbol_position_offset = min(max( symbol_position_offset, -max_symbol_position_offset ),max_symbol_position_offset);
   
	if(0)	% for debugging only!
	figure(99); plot([-Tc+1:Tc], 10*log10( max( (fliplr([h_absq,h_absq])/max(h_absq)), 10^-3 ) ) ); 
	axis([-Tc,Tc,-30,0]); 
	hold on; 
	plot( [symbol_position_offset/Tu*Tc-Tghs,symbol_position_offset/Tu*Tc-Tghs],[-30,0],'r', [symbol_position_offset/Tu*Tc+Tghs,symbol_position_offset/Tu*Tc+Tghs],[-30,0],'r' );
	hold off;
	end

	%do integer time offset correction and compensate phase shift
	phi_freq_correction_last = phi_freq_correction_last +  delta_time_offset_integer/Tu*freq_offset;	%because we delay/skip samples, we have to correct the phase

else
   delta_time_offset_integer = 0;
   time_offset_fractional = 0;
   symbol_position_offset = 0;
   %phi_freq_correction_last = 0;
end


if (FREQ_SYNC_ENABLE == 1)
	%frequency offset estimation
	freq_offset_ctrl = delta_freq_offset;
	freq_offset = freq_offset + kP_freq_controller*freq_offset_ctrl;
else
   %freq_offset = 0;
end

   
%get symbol and do frequency correction
exp_temp = exp(   j * ( (freq_offset/Tu)*[symbol_position_offset:(symbol_position_offset+Tu-1)] + phi_freq_correction_last )   ) ;

s = in((2+delta_time_offset_integer+Tgh+symbol_position_offset):(1+delta_time_offset_integer+Tgh+Tu+symbol_position_offset))  .*  exp_temp;
phi_freq_correction_last = rem( phi_freq_correction_last + (Ts)/Tu*freq_offset, 2*pi );

%do fft
S = fft(s);

%half Tg integer time offset correction (because of Tgh delay - could also be done by cycling s) and ...
%fractional time offset correction (because of sample-rate/OFDM-symbol-rate difference)
exp_temp = exp(  j * ( (2*pi/Tu*(Tgh+time_offset_fractional-symbol_position_offset)) * [0:(Tu/2)] ) );
out = [ fliplr( S(end:-1:(Tu/2+1)).*conj( exp_temp(2:end) ) ), S(1:Tu/2).*exp_temp(1:(end-1)) ];

