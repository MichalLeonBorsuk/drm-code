%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  University of Kaiserslautern, Institute of Communications Engineering     %
%%  Copyright (C) 2004 Andreas Dittrich                                       %
%%                                                                            %
%%  Author(s)    : Andreas Dittrich (dittrich@eit.uni-kl.de)                  %
%%  Project start: 27.05.2004                                                 %
%%  Last change  : 18.06.2004                                                 %
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
%%  get_ofdm_symbol.m                                                         %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  Description:                                                              %
%%  Outputs one OFDM-Symbol, which is time- and frequency synchronized.       %
%%  This module has an internal state for doing the time-/frequency           %
%%  synchronization in a control-loop. Fractional time-synchronization is     %
%%  made in the frequency domain.                                              %
%%                                                                            %
%%  Invoked by demodulation_and_equalization.m                                %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function [out, delta_time_offset_integer, time_offset_fractional, freq_offset, delta_time_offset_I, Zf] = get_ofdm_symbol( in, ...
   time_offset_fractional_init, freq_offset_init, delta_time_offset_I_init, Ts, Tu, Zi)

global TIME_SYNC_ENABLE FREQ_SYNC_ENABLE

% some fixed parameters
EPSILON = 1e-10;
%SNR_time_frequ_sync = 10^(18/10);	% assumed SNR for time and frequency sync, here: 18dB
%rho = SNR_time_frequ_sync/(SNR_time_frequ_sync+1);
max_theta = 5;
max_time_offset_ctrl = 10;
max_delta_time_offset = 2;
kP_small_timing_controller = 0.01;	%todo: calculate some better constants
kP_large_timing_controller = 0.01;
threshold_timing_small_large = 2;
kI_timing_controller = 0.00005;

kP_small_freq_controller = 0.05;	%todo: calculate some better constants
kP_large_freq_controller = 0.75;
threshold_freq_small_large = 0.5;
kI_freq_controller = 0.0008;

%some constants
Tg = Ts - Tu;
Tgh = floor( Tg/2 + 0.5 );

% set state
if (Zi(1)==0)
	phi_freq_correction_last = 0;
	delta_time_offset_I = delta_time_offset_I_init;
   dfreq_offset_I = 0;
  	freq_offset = freq_offset_init;
   time_offset_fractional = time_offset_fractional_init;
else
	phi_freq_correction_last = Zi(2);
	delta_time_offset_I =  Zi(3);
   dfreq_offset_I =  Zi(4);
  	freq_offset = Zi(5);
   time_offset_fractional = Zi(6);
end


if (TIME_SYNC_ENABLE == 1)
	temp1 = in(1:(Tg+2)) * in((Tu+1):(Tu+Tg+2))';
	temp2 = in(1:(Tg+2))* in(1:(Tg+2))';
	temp3 = in((Tu+1):(Tu+Tg+2))* in((Tu+1):(Tu+Tg+2))';
	temp4 = ( EPSILON + 0.5*( abs( temp2 ) + abs( temp3 ) ) );

	%get time offset measurement signal: theta
	in__Tu_plus_1_to_Tu_plus_5 = in((Tu+1):(Tu+5))';
	in__1_to_5 = in(1:5);
	theta_plus = abs( temp1 - in__1_to_5*in__Tu_plus_1_to_Tu_plus_5 ) - ...
	   ( 0.5*( - in__1_to_5*in__1_to_5' - in__Tu_plus_1_to_Tu_plus_5'*in__Tu_plus_1_to_Tu_plus_5 ) );

	in__Tg_minus_2_to_Tg_plus_2 = in((Tg-2):(Tg+2));
	int_Tg_plus_Tu_minus_2_to_Tg_plus_Tu_plus_2 = in((Tg+Tu-2):(Tg+Tu+2))';
	theta_minus = abs( temp1 - in__Tg_minus_2_to_Tg_plus_2*int_Tg_plus_Tu_minus_2_to_Tg_plus_Tu_plus_2 ) - ...
	   ( 0.5*( - in__Tg_minus_2_to_Tg_plus_2*in__Tg_minus_2_to_Tg_plus_2' - int_Tg_plus_Tu_minus_2_to_Tg_plus_Tu_plus_2'*int_Tg_plus_Tu_minus_2_to_Tg_plus_Tu_plus_2 ) );

	delta_theta = real( theta_plus - theta_minus )*Tgh / temp4;		%estimated delay in samples
	delta_theta = min( max( delta_theta, -max_theta ), max_theta );   %limit value

	% filter theta: P-I-controler
	time_offset_ctrl = delta_theta - time_offset_fractional;	% integer part is corrected from outside this routine (=delay of input samples)
	delta_time_offset_I = delta_time_offset_I + kI_timing_controller*time_offset_ctrl;
	delta_time_offset_P = kP_large_timing_controller*time_offset_ctrl + ...
	   threshold_timing_small_large*(kP_small_timing_controller-kP_large_timing_controller)*tanh(time_offset_ctrl/threshold_timing_small_large); 
	delta_time_offset = delta_time_offset_P + delta_time_offset_I + time_offset_fractional;
	delta_time_offset_integer = floor( delta_time_offset + 0.5 );
	delta_time_offset_integer = min( max( delta_time_offset_integer, -1 ), 1 );	%only +/- one symbol 
	time_offset_fractional = delta_time_offset - delta_time_offset_integer;   
   
	%do integer time offset correction and compensate phase shift
	phi_freq_correction_last = phi_freq_correction_last +  delta_time_offset_integer/Tu*freq_offset;	%because we delay/skip samples, we have to correct the phase

else
   delta_time_offset_integer = 0;
   time_offset_fractional = 0;
   %phi_freq_correction_last = 0;
end


if (FREQ_SYNC_ENABLE == 1)
	%get fractional frequency offset measurement signal: epsilon_ML
	temp = in((2+delta_time_offset_integer):(1+delta_time_offset_integer+Tg)) * in((2+delta_time_offset_integer+Tu):(1+delta_time_offset_integer+Tu+Tg))';
	epsilon_ML = atan2(imag(temp), real(temp));

	% filter epsilon_ML: P-I-controler, todo: combine P-I-ctrl-filter with FIR-filter
	freq_offset_ctrl = rem( epsilon_ML - freq_offset + pi + 100*pi, 2*pi ) - pi;
	dfreq_offset_I = dfreq_offset_I + kI_freq_controller*freq_offset_ctrl;
	dfreq_offset_P = kP_large_freq_controller*freq_offset_ctrl + ...
	   threshold_freq_small_large*(kP_small_freq_controller-kP_large_freq_controller)*tanh(freq_offset_ctrl/threshold_freq_small_large); 
	dfreq_offset = dfreq_offset_P + dfreq_offset_I;
	freq_offset = dfreq_offset + freq_offset;
   
else
   %freq_offset = 0;
end



%get symbol and do frequency correction
exp_temp = exp(   j * ( (freq_offset/Tu)*[0:(Tu-1)] + phi_freq_correction_last )   ) ;

s = in((2+delta_time_offset_integer+Tgh):(1+delta_time_offset_integer+Tgh+Tu))  .*  exp_temp;
phi_freq_correction_last = rem( phi_freq_correction_last + (Ts)/Tu*freq_offset, 2*pi );

%do fft
S = fft(s);

%half Tg integer time offset correction (because of Tgh delay - could also be done by cycling s) and ...
%fractional time offset correction (because of sample-rate/OFDM-symbol-rate difference)
exp_temp = exp(  j * ( (2*pi/Tu*(Tgh+time_offset_fractional)) * [0:(Tu/2)] ) );

out = [ fliplr( S(end:-1:(Tu/2+1)).*conj( exp_temp(2:end) ) ), S(1:Tu/2).*exp_temp(1:(end-1)) ];

Zf = [1; ...
	phi_freq_correction_last; ...
	delta_time_offset_I; ...
   dfreq_offset_I; ...
  	freq_offset; ...
   time_offset_fractional ...
];


