%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  University of Kaiserslautern, Institute of Communications Engineering     %
%%  Copyright (C) 2004 Andreas Dittrich                                       %
%%                                                                            %
%%  Author(s)    : Andreas Dittrich (dittrich@eit.uni-kl.de)                  %
%%  Project start: 27.05.2004                                                 %
%%  Last change: 19.04.2005, 11:50                                            %
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
%% Last change: 19.04.2005, 11:50                                             %
%% By         : Andreas Dittrich                                              %
%% Description: small parameter adjustments of                                %
%% sigmaq_noise_list, f_cut_t, f_cut_k and disabled filter renormalization    %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  equalization_init.m                                                       %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  Description:                                                              %
%%  Initialization and calculation of fixed parameters, e.g. cell positions,  %
%%  equalization matrix etc. It will create the file MY_INIT_FILENAME if it   %
%%  is not there.                                                             %
%%                                                                            %
%%  Invoked by demodulation_and_equalization.m                                %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


function values = equalization_init( varargin )

if (nargin>0)
   commandflag = varargin{1}(1);
else
   commandflag = ' ';
end


create_new_file = isequal( commandflag, 'c' );
MY_INIT_FILENAME = 'equalization_init.mat';


VERBOSE_LEVEL = settings_handler(4,14);


if ( ~exist(MY_INIT_FILENAME,'file') | create_new_file )
   
	if ( ~create_new_file )
		message_(0<=VERBOSE_LEVEL, sprintf('file not found: "%s" - rendering new data', MY_INIT_FILENAME)); 
   else
		message_(0<=VERBOSE_LEVEL, sprintf('creating file "%s"', MY_INIT_FILENAME)); 
   end
   message_('flush');
  
      
   values_to_save = {};
   values = {};
   
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
Tg_list = Ts_list - Tu_list;
values_to_save = { values_to_save{:}, 'Ts_list', 'Tu_list', 'Tg_list' };
values = { values{:}, Ts_list, Tu_list, Tg_list };

sigmaq_noise_list = [ 10^(-16/10); 10^(-14/10);  10^(-14/10);  10^(-12/10) ];


%ETSI ES 201980 / 8.2 / Table 80
symbols_per_frame_list = [15,15,20,24];	%index = (robustness_mode-1) + 1
frames_per_superframe = 3;
values_to_save = { values_to_save{:}, 'symbols_per_frame_list', 'frames_per_superframe' };
values = { values{:}, symbols_per_frame_list, frames_per_superframe };

%ETSI ES 201980 / 8.3.1 / Table 82
%index = spectrum_occupancy*2 + (robustness_mode-1)*2*6 + 1
K_min_K_max_list = [ [  2;102], [  2;114], [-102;102], [-114;114], [ -98;314], [-110;350], ...
						   [  1; 91], [  1;103], [ -91; 91], [-103;103], [ -87;279], [ -99;311], ...
						   [  0;  0], [  0;  0], [   0;  0], [ -69; 69], [   0;  0], [ -67;213], ...
						   [  0;  0], [  0;  0], [   0;  0], [ -44; 44], [   0;  0], [ -43;135] ];
values_to_save = { values_to_save{:}, 'K_min_K_max_list' };
values = { values{:}, K_min_K_max_list };

%index = spectrum_occupancy + (robustness_mode-1)*6 + 1
no_of_used_cells_per_frame_list = ...	%(K_max - K_min + 1 - no_of_unused_carriers)*symbols_per_frame
   [ 98*15, 110*15, 202*15, 226*15, 410*15, 458*15, ...
     90*15, 102*15, 182*15, 206*15, 366*15, 410*15, ...
         0,      0,      0, 138*20,      0, 280*20, ...
         0,      0,      0,  88*24,      0, 178*24 ];
   
%ETSI ES 201980 / 8.4.2.1 / Table 84
%index = robustness_mode , pilot_no
freq_ref_cells_k_list = { [18;54;72], [16;48;64], [11;33;44], [7;21;28] };
freq_ref_cells_theta_1024_list = { [205;836;215], [331;651;555], [214;392;242], [788;1014;332] };

%ETSI ES 201980 / 8.4.3.1 / Table 86-89
time_ref_cells_k_list = { ...
   [17,18,19,21,28,29,32,33,39,40,41,53,54,55,56,60,61,63,71,72,73]', ...
	[14,16,18,20,24,26,32,36,42,44,48,49,50,54,56,62,64,66,68]', ...
	[08,10,11,12,14,16,18,22,24,28,30,32,33,36,38,42,44,45,46]', ...
   [05,06,07,08,09,11,12,14,15,17,18,20,21,23,24,26,27,28,29,30,32]' };
time_ref_cells_theta_1024_list = { ...
   [973,205,717,264,357,357,952,440,856,88,88,068,836,836,836,1008,1008,752,215,215,727]', ...
	[304,331,108,620,192,704,044,432,588,844,651,651,651,460,460,944,555,940,428]', ...
	[722,466,214,214,479,516,260,577,662,003,771,392,392,037,037,474,242,242,754]', ...
	[636,124,788,788,200,688,152,920,920,644,388,652,1014,176,176,752,496,332,432,964,452]' };
values_to_save = { values_to_save{:}, 'time_ref_cells_k_list', 'time_ref_cells_theta_1024_list', 'freq_ref_cells_theta_1024_list' };
values = { values{:}, time_ref_cells_k_list, time_ref_cells_theta_1024_list, freq_ref_cells_theta_1024_list };

   
%ETSI ES 201980 / 8.4.4.3.1 / Table 92
%index = mode (A=1, B=2 ...)
x_list = [4,2,2,1];
y_list = [5,3,2,3];
k0_list = [2,1,1,1];

%equalizer settings
symbols_per_2D_window_list = 2*y_list;
symbols_to_delay_list = floor(symbols_per_2D_window_list/2); % mid of window

values_to_save = { values_to_save{:}, 'y_list', 'symbols_per_2D_window_list', 'symbols_to_delay_list' };
values = { values{:}, y_list, symbols_per_2D_window_list, symbols_to_delay_list };


%ETSI ES 201980 / 8.4.4.3.2
W_1024_list = { [228,341,455;   455,569,683;   683,796,910;   910,  0,114;   114,228,341], ...
                [512,  0,512,  0,512;     0,512,  0,512,  0;   512,  0,512,  0,512], ...
                [465,372,279,186, 93,  0,931,838,745,652;   931,838,745,652,559,465,372,279,186, 93], ...
                [366,439,512,585,658,731,805,878;   731,805,878,951,  0, 73,146,219;    73,146,219,293,366,439,512,585] };
             
Z_256_list =  { [  0, 81,248;    18,106,106;   122,116, 31;   129,129, 39;    33, 32,111], ...
                [  0, 57,164, 64, 12;   168,255,161,106,118;    25,232,132,233, 38], ...
                [  0, 76, 29, 76,  9,190,161,248, 33,108;   179,178, 83,253,127,105,101,198,250,145], ...
                [  0,240, 17, 60,220, 38,151,101;   110,  7, 78, 82,175,150,106, 25;   165,  7,252,124,253,177,197,142] };
          
Q_1024_list = [  36,  12,  12,  14];

%power_boost = power_boost_list(spectrum_occupancy*4 + (robustness_mode-1)*4*6 + [1:4]);
power_boost_list = [ [  2;  6; 98;102], [  2;  6;110;114], [-102;-98; 98;102], [-114;-110;110;114], [-98;-94;310;314], [-110;-106;346;350], ...
						   [  1;  3; 89; 91], [  1;  3;101;103], [ -91;-89; 89; 91], [-103;-101;101;103], [-87;-85;277;279], [ -99; -97;309;311], ...
						   [  0;  0;  0;  0], [  0;  0;  0;  0], [   0;  0;  0;  0], [ -69; -67; 67; 69], [  0;  0;  0;  0], [ -67; -65;211;213], ...
						   [  0;  0;  0;  0], [  0;  0;  0;  0], [   0;  0;  0;  0], [ -44; -43; 43; 44], [  0;  0;  0;  0], [ -43; -42;134;135] ];



% index of this is mode_and_occupancy_code
ref_cells_subset_list = cell(16,1);
gain_ref_cells_k_list = cell(16,1);
gain_ref_cells_theta_1024_list = cell(16,1);
gain_ref_cells_a_list = cell(16,1);
W_symbol_int16_list = cell(16,1);
W_pilots_int16_list = cell(16,1);

values_to_save = { values_to_save{:}, 'gain_ref_cells_k_list', 'gain_ref_cells_theta_1024_list', 'gain_ref_cells_a_list', ...
      'W_symbol_int16_list', 'W_pilots_int16_list' };
values = { values{:}, gain_ref_cells_k_list, gain_ref_cells_theta_1024_list, gain_ref_cells_a_list, ...
      W_symbol_int16_list, W_pilots_int16_list };

%index = spectrum_occupancy + (robustness_mode-1)*6 + 1
mean_energy_of_used_cells_list = zeros(24,1);
values_to_save = { values_to_save{:}, 'mean_energy_of_used_cells_list' };
values = { values{:}, mean_energy_of_used_cells_list };

%mode_and_occupancy_code = mode_and_occupancy_code_table( (robustness_mode-1)*6 + spectrum_occupancy + 1);
mode_and_occupancy_code_table = [0,1,2,3,4,5,   6,7,8,9,10,11,   -1,-1,-1,12,-1,13,   -1,-1,-1,14,-1,15];
values_to_save = { values_to_save{:}, 'mode_and_occupancy_code_table' };
values = { values{:}, mode_and_occupancy_code_table };

for (robustness_mode=1:4)
 for (spectrum_occupancy=0:3)  
	mode_and_occupancy_code = mode_and_occupancy_code_table( (robustness_mode-1)*6 + spectrum_occupancy + 1);   
   if (mode_and_occupancy_code>=0) 
   
	Tu = Tu_list(robustness_mode);	% fft periode
	Ts = Ts_list(robustness_mode);	% symbol periode
	Tg = Tg_list(robustness_mode);	% guard periode
	
	sigmaq_noise = sigmaq_noise_list(robustness_mode);
       
	symbols_per_frame = symbols_per_frame_list(robustness_mode);
	freq_ref_cells_k = freq_ref_cells_k_list{robustness_mode};
	freq_ref_cells_theta_1024 = freq_ref_cells_theta_1024_list{robustness_mode};
	time_ref_cells_k = time_ref_cells_k_list{robustness_mode};
	time_ref_cells_theta_1024 = time_ref_cells_theta_1024_list{robustness_mode};
	
	K_min = K_min_K_max_list(spectrum_occupancy*2 + (robustness_mode-1)*2*6 + 1);
	K_max = K_min_K_max_list(spectrum_occupancy*2 + (robustness_mode-1)*2*6 + 2);
	
	carrier_per_symbol = K_max - K_min + 1;
	carrier_per_frame = symbols_per_frame*carrier_per_symbol;
	power_boost = power_boost_list(spectrum_occupancy*4 + (robustness_mode-1)*4*6 + [1:4]);
	
	x = x_list(robustness_mode);
	y = y_list(robustness_mode);
	k0 = k0_list(robustness_mode);
	
	W_1024 = W_1024_list{robustness_mode};
	Z_256 = Z_256_list{robustness_mode};	
	Q_1024 = Q_1024_list(robustness_mode);
	
	gain_ref_cells_k = [];
	gain_ref_cells_theta_1024 = [];
   gain_ref_cells_a = [];
   
   mean_energy_of_used_cells = ...
      no_of_used_cells_per_frame_list(spectrum_occupancy + (robustness_mode-1)*6 + 1) + ...
      length(freq_ref_cells_k) + ...		%this pilots have double energy, so we increase the energy by 1
      length(time_ref_cells_k);
   
   for s=[0:(symbols_per_frame-1)]
       n = mod(s,y);
       m = floor( s/y );
       p_min = ceil( ( K_min - k0 - x*n )/(x*y) );
       p_max = floor( ( K_max - k0 - x*n )/(x*y) );
       
       for p=[p_min:p_max]
          %ETSI ES 201980 / 8.4.4.1 / Table 90
          k = k0 + x*n + x*y*p;
          theta_1024 = rem( 4*Z_256(n+1,m+1) + p*W_1024(n+1,m+1) + p^2*(1+s)*Q_1024, 1024 );
          a = sqrt(2);
	
          % power boost?
          if (ismember( k,power_boost)) a = 2; end
          
          % is time reference-cell?
          if (s==0)
             if( ismember( k, time_ref_cells_k ) )
                indx = find( k == time_ref_cells_k );
                theta_1024 = time_ref_cells_theta_1024(indx);
                a = sqrt(2);
                
                mean_energy_of_used_cells = mean_energy_of_used_cells - 1;
             end
          end
          
          % is frequency reference-cell?
          if ( ismember( k, freq_ref_cells_k ) )
             indx = find( k == freq_ref_cells_k );
             theta_1024 = freq_ref_cells_theta_1024(indx);
             if ( robustness_mode == 4 )
                theta_1024 = rem( theta_1024 + 512*s, 1024 );
             end
                
             a = sqrt(2);
             
             mean_energy_of_used_cells = mean_energy_of_used_cells - 1;
          end
	
          gain_ref_cells_k = [ gain_ref_cells_k, k + s*carrier_per_symbol ];
          gain_ref_cells_theta_1024 = [ gain_ref_cells_theta_1024, theta_1024 ];
          gain_ref_cells_a = [ gain_ref_cells_a, a ];
          
          mean_energy_of_used_cells =  mean_energy_of_used_cells - 1 + a^2;
          
       end
	end
	
   mean_energy_of_used_cells_list(spectrum_occupancy + (robustness_mode-1)*6 + 1) = ...
      mean_energy_of_used_cells / no_of_used_cells_per_frame_list(spectrum_occupancy + (robustness_mode-1)*6 + 1);
   
   % precompute 2-D-Wiener filter-matrix w.r.t. power boost
   % Reference: Peter Hoeher, Stefan Kaiser, Patrick Robertson: 
   %            "Two-Dimensional Pilot-Symbol-Aided Channel Estimation By Wiener Filtering", 
   %            ISIT 1997, Ulm, Germany, June 29 - July 4
   
   % PHI = auto-covariance-matrix
   f_cut_t = 0.0675*1/y; % two-sided maximum doppler frequency (normalized w.r.t symbol duration Ts) 
   f_cut_k = 1.75*Tg/Tu; % two-sided maximum echo delay (normalized w.r.t useful symbol duration Tu)
   
   %only informative: (can someone confirm this?)
   f_D_max = f_cut_t*12000/Ts/2; % one-sided maximum doppler frequency in Hz 
   tau_max = f_cut_k*Tu/12000/2; % one-sided maximum echo delay in seconds
   
   % we only take a subset of training cells in an frame
   W_symbol_int16 = cell( y, 1 );
   W_pilots_int16 = cell( y, 1 );

   gain_ref_cells_subset = cell( y, 1 );
   
   symbols_per_2D_window = symbols_per_2D_window_list(robustness_mode);
   symbols_to_delay = symbols_to_delay_list(robustness_mode);
   
   for(n=0:(y-1))
      message_(0<=VERBOSE_LEVEL,'.');
      message_('flush');
      
		training_cells_k = mod( gain_ref_cells_k -K_min + (symbols_per_frame-n)*carrier_per_symbol, symbols_per_frame*carrier_per_symbol) + K_min;
      gain_ref_cells_subset{n+1} = find( (training_cells_k-K_min)<(carrier_per_symbol*symbols_per_2D_window) );
      
      next_pilot_cells_k_index = find( ((training_cells_k-K_min)>=(carrier_per_symbol*(symbols_per_2D_window))) & ...
         ((training_cells_k-K_min)<(carrier_per_symbol*(symbols_per_2D_window+1))) );
      next_pilot_cells_k = rem( training_cells_k(next_pilot_cells_k_index) - K_min, carrier_per_symbol ) + K_min;
      
      [training_cells_k, sort_index] = sort( training_cells_k(gain_ref_cells_subset{n+1}) );
      gain_ref_cells_subset{n+1} = gain_ref_cells_subset{n+1}( sort_index );
      
      
      gain_ref_cells_per_window = length(training_cells_k);
      
      W_symbol = zeros(gain_ref_cells_per_window, carrier_per_symbol);
      W_symbol_int16{n+1} = int16( W_symbol );
      
      W_pilots = zeros(gain_ref_cells_per_window, length(next_pilot_cells_k) );
      W_pilots_int16{n+1} = int16( W_pilots );
      
      PHI = zeros(gain_ref_cells_per_window);
		for (k_index1=1:gain_ref_cells_per_window)
           k_index2 = [1:gain_ref_cells_per_window];
           
           k1_pos = mod( training_cells_k(k_index1)-K_min, carrier_per_symbol ) + K_min;
           t1_pos = floor( (training_cells_k(k_index1)-K_min)/carrier_per_symbol );
           k2_pos = mod( training_cells_k(k_index2)-K_min, carrier_per_symbol ) + K_min;
           t2_pos = floor( (training_cells_k(k_index2)-K_min)/carrier_per_symbol );
              
           PHI(k_index1,k_index2) = sinc_( (k1_pos-k2_pos)*f_cut_k ).*sinc_( (t1_pos-t2_pos)*f_cut_t );
		end
		
		PHI = PHI + sigmaq_noise*diag( 2./( gain_ref_cells_a(gain_ref_cells_subset{n+1}).^2 ) );
		
		%matrix inversion
		PHI_inv = inv(PHI);
		
		THETA = zeros(1,gain_ref_cells_per_window);
		for ( k_index1=[1:(K_max-K_min+1)] )
           k_index2 = [1:gain_ref_cells_per_window];
           
           k1_pos = k_index1+K_min-1;
           t1_pos = symbols_to_delay;	
           k2_pos = mod( training_cells_k(k_index2)-K_min, K_max-K_min+1 ) + K_min;
           t2_pos = floor( (training_cells_k(k_index2)-K_min)/(K_max-K_min+1) );
              
			  % THETA = cross-covariance-vector
           THETA(k_index2) = sinc_( (k1_pos-k2_pos)*f_cut_k ).*sinc_( (t1_pos-t2_pos)*f_cut_t );
         
           W_symbol = transpose( THETA*PHI_inv );
%           W_symbol = W_symbol ./ ( ones( size(W_symbol,1), 1 )*sum(W_symbol) ); %hmmm, renormalize that a constant channel transfer function is estimated right!
           W_symbol_int16{n+1}(:,k_index1) = int16( floor(2^15*( W_symbol ) + 0.5) );
      end
        
		THETA = zeros(1,gain_ref_cells_per_window);
		for ( k_index1=[1:length(next_pilot_cells_k)] )
           k_index2 = [1:gain_ref_cells_per_window];
           
           k1_pos = next_pilot_cells_k(k_index1);
           t1_pos = symbols_per_2D_window-1;	
           k2_pos = mod( training_cells_k(k_index2)-K_min, K_max-K_min+1 ) + K_min;
           t2_pos = floor( (training_cells_k(k_index2)-K_min)/(K_max-K_min+1) );
              
			  % THETA = cross-covariance-vector
           THETA(k_index2) = sinc_( (k1_pos-k2_pos)*f_cut_k ).*sinc_( (t1_pos-t2_pos)*f_cut_t );
           
           W_pilots = transpose( THETA*PHI_inv );
%           W_pilots = W_pilots ./ ( ones( size(W_pilots ,1), 1 )*sum(W_pilots ) ); %hmmm, renormalize that a constant channel transfer function is estimated right!
           W_pilots_int16{n+1}(:,k_index1) = transpose( int16( floor(2^15*( W_pilots ) + 0.5) ) );
      end
        
        
        
   end
   
   
	gain_ref_cells_subset_list{mode_and_occupancy_code+1} = gain_ref_cells_subset;
   gain_ref_cells_k_list{mode_and_occupancy_code+1} = gain_ref_cells_k;
   gain_ref_cells_theta_1024_list{mode_and_occupancy_code+1} = gain_ref_cells_theta_1024;
   gain_ref_cells_a_list{mode_and_occupancy_code+1} = gain_ref_cells_a;
   W_symbol_int16_list{mode_and_occupancy_code+1} = W_symbol_int16;
   W_pilots_int16_list{mode_and_occupancy_code+1} = W_pilots_int16;
   
   end
 end
end

values_to_save = { values_to_save{:}, 'gain_ref_cells_subset_list' };
values = { values{:}, gain_ref_cells_subset_list };

equalization_init_ok = 1;
values_to_save = { values_to_save{:}, 'equalization_init_ok' };
values = { values{:}, equalization_init_ok };

% save( MY_INIT_FILENAME, values_to_save{:} );

save( MY_INIT_FILENAME, 'Ts_list','Tu_list','Tg_list','symbols_per_frame_list','frames_per_superframe','K_min_K_max_list', ...
                      'time_ref_cells_k_list','time_ref_cells_theta_1024_list','freq_ref_cells_theta_1024_list','y_list', ...
                      'symbols_per_2D_window_list','symbols_to_delay_list','gain_ref_cells_k_list','gain_ref_cells_theta_1024_list', ...
                      'gain_ref_cells_a_list','W_symbol_int16_list','W_pilots_int16_list','mean_energy_of_used_cells_list', ...
                      'mode_and_occupancy_code_table','gain_ref_cells_subset_list','equalization_init_ok');


message_(0<=VERBOSE_LEVEL, sprintf(' ok\n'));

else
	message_(3<=VERBOSE_LEVEL, sprintf('loading initialization file "%s"...', MY_INIT_FILENAME)); 
    message_('flush');
    
    values_struct = load(MY_INIT_FILENAME);
    
    values_to_save = {'Ts_list','Tu_list','Tg_list','symbols_per_frame_list','frames_per_superframe','K_min_K_max_list', ...
                      'time_ref_cells_k_list','time_ref_cells_theta_1024_list','freq_ref_cells_theta_1024_list','y_list', ...
                      'symbols_per_2D_window_list','symbols_to_delay_list','gain_ref_cells_k_list','gain_ref_cells_theta_1024_list', ...
                      'gain_ref_cells_a_list','W_symbol_int16_list','W_pilots_int16_list','mean_energy_of_used_cells_list', ...
                      'mode_and_occupancy_code_table','gain_ref_cells_subset_list','equalization_init_ok'};
    
    
    values = {};
        
    for var_index = 1:length(values_to_save)
        
        values = { values{:}, getfield(values_struct, values_to_save{var_index}) };
                
    end
    
   
    message_(3<=VERBOSE_LEVEL, sprintf(' ok\n')); 
end

message_('flush');




function message_(varargin)

if exist( 'message.m', 'file' );
   eval('message(varargin{:})');
else
   if ~isequal( varargin{1}, 'flush' )
      fprintf(1, varargin{2:end});
   end
end



%so we dont need signal-proc-toolbox
function y=sinc_(x)
TOO_SMALL = 1e-10;
y=ones(size(x));
i=find(abs(x)>TOO_SMALL);
y(i)=sin(pi*x(i))./(pi*x(i));



