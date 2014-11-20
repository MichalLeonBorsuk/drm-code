%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  University of Kaiserslautern, Institute of Communications Engineering     %
%%  Copyright (C) 2004 Andreas Dittrich                                       %
%%                                                                            %
%%  Author(s)    : Andreas Dittrich (dittrich@eit.uni-kl.de)                  %
%%  Project start: 27.05.2004                                                 %
%%  Last change: 03.05.2005, 09:30                                            %
%%  Changes: |                                                                %
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
%%  Last change: 03.05.2005, 09:30                                            %
%%  By         : Andreas Dittrich                                             %
%%  Description: lowered reliability level of robustness mode detection       %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  get_robustness_mode.m                                                     %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  Description:                                                              %
%%  Detects the robustness-mode and also estimates symbol-time-position,      %
%%  samplerate-offset and fractional frequency-offset                         %
%%                                                                            %
%%  Invoked by demodulation_and_equalization.m                                %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


function [robustness_mode, time_offset, samplerate_offset, frequency_offset_fractional] = get_robustness_mode( in )

SAMPLERATE_OFFSET_MAX = 3500*1e-6;

SNR_mode_detection = 10^(15/10);	% assumed SNR for mode detection, here: 15dB
rho = SNR_mode_detection/(SNR_mode_detection+1);
EPSILON = 1e-10;

% mode A
Ts_A = 320;
Tu_A = 288;
% mode B
Ts_B = 320;
Tu_B = 256;
% mode C
Ts_C = 240;
Tu_C = 176;
% mode D
Ts_D = 200;
Tu_D = 112;

Ts_list = [Ts_A, Ts_B, Ts_C, Ts_D];
Tu_list = [Tu_A, Tu_B, Tu_C, Tu_D];
max_abs_gamma_rel_list = zeros(1,4);
theta_list = zeros(1,4);
epsilon_ML_list = zeros(1,4);
N_symbols_mode_detection = floor( (length(in)+1)/Ts_A ) - 1;	%i hope this calculation is correct to guarantee no overflow

for( robustness_mode = [1:4] )
   Ts = Ts_list(robustness_mode);
   Tu = Tu_list(robustness_mode);
   Tg = Ts - Tu;

	t_smp = 0;
	gamma = zeros(1,Ts);
   Phi = zeros(size(gamma));
   
   in_ = in(1:(end-Tu)).*conj(in((1+Tu):end));
   my_rect = ones(1,Tg);
   conv_in_ = filter( my_rect, 1, in_ );
   abs_in_ = abs( in ).^2;
   abs_in_in_ = abs_in_(1:(end-Tu)) + abs_in_((1+Tu):end);
   conv_abs_in_in_ = filter( my_rect, 1, abs_in_in_ );
   
   for( l=[1:N_symbols_mode_detection] )
      %for( m=[1:Ts] )	
  			%gamma(m) = gamma(m) + in(t_smp+m+[0:(Tg-1)]) * in(t_smp+m+Tu+[0:(Tg-1)])';
         %Phi(m) = Phi(m) + 0.5*( EPSILON + sum( abs( in(t_smp+m+[0:(Tg-1)]) ).^2 ) + sum( abs( in(t_smp+m+Tu+[0:(Tg-1)]) ).^2 ) );
      %end
      gamma = gamma + conv_in_(t_smp+Tg+[0:(Ts-1)]);
      Phi = Phi + 0.5*( EPSILON + conv_abs_in_in_(t_smp+Tg+[0:(Ts-1)]) );
      t_smp = t_smp + Ts;
   end
   [max_abs_gamma_rel, theta] = max( abs(gamma) - (rho*Phi) );	
   max_abs_gamma_rel = abs(gamma(theta))/(rho*Phi(theta));
   epsilon_ML = angle(gamma(theta));
   
   max_abs_gamma_rel_list(robustness_mode) = max_abs_gamma_rel;	
	theta_list(robustness_mode) = theta;
	epsilon_ML_list(robustness_mode) = epsilon_ML;

end

% now decide for one mode
[max_abs_gamma_rel_list_sorted, robustness_mode_index] = sort( max_abs_gamma_rel_list );
% check consistence and set mode if reliable
if( (max_abs_gamma_rel_list_sorted(4)>0.6) & (max_abs_gamma_rel_list_sorted(3)<0.4) )
   robustness_mode = robustness_mode_index(4);
   
   % set the appropriate parameter
	Ts = Ts_list(robustness_mode); 	% symbol periode
	Tu = Tu_list(robustness_mode);	% fft periode
   Tg = Ts - Tu;

	time_offset_mean = theta_list(robustness_mode) - 1;	% time offset
   frequency_offset_fractional = epsilon_ML_list(robustness_mode);	% phase difference or fractional freq offset
   
   %quick and dirty calculation of initial time delay and samplerate offset
   %TODO: do it better!
   in_ = in(1:(end-Tu)).*conj(in((1+Tu):end));
   my_rect = [0.5,ones(1,Tg-2),0.5];
   conv_in_ = filter( my_rect, 1, in_ );
   abs_in_ = abs( in ).^2;
   abs_in_in_ = abs_in_(1:(end-Tu)) + abs_in_((1+Tu):end);
   conv_abs_in_in_ = filter( my_rect, 1, abs_in_in_ );   
   temp = reshape( abs( conv_in_(Tg+time_offset_mean+Ts/2+[0:(Ts*(N_symbols_mode_detection-2)-1)]) ) ...
      - (rho*0.5*(EPSILON + conv_abs_in_in_(Tg+time_offset_mean+Ts/2+[0:(Ts*(N_symbols_mode_detection-2)-1)]))), Ts, N_symbols_mode_detection-2 );
   [a,b]=max(temp);
   temp2 = polyfit([0:(N_symbols_mode_detection-3)],b,1);
   samplerate_offset = temp2(1)/Ts;
   time_offset = rem( temp2(2) + ( Ts/2 + time_offset_mean ) - 2, Ts ); 
   
   if(0) %maybe more precise estimation
   in_ = in(1:(end-Tu)).*conj(in((1+Tu):end));
   my_rect = [0.5,ones(1,Tg-2),0.5];
   conv_in_ = filter( my_rect, 1, in_ );
   abs_in_ = abs( in ).^2;
   abs_in_in_ = abs_in_(1:(end-Tu)) + abs_in_((1+Tu):end);
   conv_abs_in_in_ = filter( my_rect, 1, abs_in_in_ );   
   temp1 = abs( conv_in_(Tg+time_offset_mean+Ts/2+[0:(Ts*(N_symbols_mode_detection-2)-1)]) );
   temp2 = (rho*0.5*(EPSILON + conv_abs_in_in_(Tg+time_offset_mean+Ts/2+[0:(Ts*(N_symbols_mode_detection-2)-1)])));
   temp3 = reshape( temp1 - temp2, Ts, N_symbols_mode_detection-2 );
   [a,b]=max(temp3);
   temp4 = [temp1(1,:);temp1;temp1(end,:)];
   temp5 = ( temp4(b-1+[1:Ts:Ts*(N_symbols_mode_detection-2)])-temp4(b+1+[1:Ts:Ts*(N_symbols_mode_detection-2)]) );
   temp6 = ( temp1(b-1+[1:Ts:Ts*(N_symbols_mode_detection-2)]) );
   b2 = b - min(max(temp5./(EPSILON + temp6)*Tg/2,-1),1);
   temp7 = polyfit([0:(N_symbols_mode_detection-3)],b2,1);
   samplerate_offset = temp7(1)/Ts;
   time_offset = rem( temp7(2) + ( Ts/2 + time_offset_mean ) - 2, Ts ); 
	end

else
   robustness_mode = 0;
   time_offset = 0;
   samplerate_offset = 0;
	frequency_offset_fractional = 0;	% phase difference or fractional freq offset
end

%check reliability
if ( (abs(samplerate_offset)>SAMPLERATE_OFFSET_MAX) | (time_offset<0) )
   robustness_mode = 0;
   time_offset = 0;
   samplerate_offset = 0;
   frequency_offset_fractional = 0;	
end

