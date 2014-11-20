%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  University of Kaiserslautern, Institute of Communications Engineering     %
%%  Copyright (C) 2004 Torsten Schorr                                         %
%%                                                                            %
%%  Author(s)    : Torsten Schorr (schorr@eit.uni-kl.de)                      %
%%  Project start: 06.07.2004                                                 %
%%  Last change  : 06.07.2004                                                 %
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
%%  Create_SDC_Demapper.m                                                     %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  Description:                                                              %
%%  Demapper for the SDC OFDM cells of a super frame                          %
%%                                                                            %
%%  Usage:                                                                    %
%%  SDC_Demapper =                                                            %
%%           Create_SDC_Demapper(robustness_mode, spectrum_occupancy, K_dc)   %
%%                                                                            %
%%  Invoked by channel_decoding.m                                             %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function SDC_Demapper = Create_SDC_Demapper(robustness_mode, spectrum_occupancy, K_dc, K_modulo)

mode_and_occupancy_code_table = [0,1,2,3,4,5,   6,7,8,9,10,11,   -1,-1,-1,12,-1,13,   -1,-1,-1,14,-1,15];
K_min_K_max_list = [ [  2;102], [  2;114], [-102;102], [-114;114], [ -98;314], [-110;350], ...
						   [  1; 91], [  1;103], [ -91; 91], [-103;103], [ -87;279], [ -99;311], ...
						   [  0;  0], [  0;  0], [   0;  0], [ -69; 69], [   0;  0], [   -67;  213], ...
						   [  0;  0], [  0;  0], [   0;  0], [ -44; 44], [   0;  0], [   -43;  135] ];
Tu_list = [288, 256, 176, 112];
Tu = Tu_list(robustness_mode);
if (spectrum_occupancy >=4 )
    Tu = Tu * 2;
end
x_list = [4,2,2,1];
y_list = [5,3,2,3];
k0_list = [2,1,1,1];
time_ref_cells_k_list = { ...
   [17,18,19,21,28,29,32,33,39,40,41,53,54,55,56,60,61,63,71,72,73]', ...
	[14,16,18,20,24,26,32,36,42,44,48,49,50,54,56,62,64,66,68]', ...
	[08,10,11,12,14,16,18,22,24,28,30,32,33,36,38,42,44,45,46]', ...
   [05,06,07,08,09,11,12,14,15,17,18,20,21,23,24,26,27,28,29,30,32]' };
freq_ref_cells_k_list = { [18;54;72], [16;48;64], [11;33;44], [7;21;28] };
SDC_symbols = [2, 2, 3, 3];

K_min = K_min_K_max_list(spectrum_occupancy*2 + (robustness_mode-1)*2*6 + 1);
K_max = K_min_K_max_list(spectrum_occupancy*2 + (robustness_mode-1)*2*6 + 2);
mode_and_occupancy_code = mode_and_occupancy_code_table( (robustness_mode-1)*6 + spectrum_occupancy + 1);

if (mode_and_occupancy_code<0) error ('Bad mode_and_occupancy_code'); end

carrier_per_symbol = K_max - K_min + 1;

x = x_list(robustness_mode);
y = y_list(robustness_mode);
k0 = k0_list(robustness_mode);

gain_ref_cells_k = [];

for s=[0:2]%(symbols_per_frame-1)]
    n = mod(s,y);
    m = floor( s/y );
    p_min = ceil( ( K_min - k0 - x*n )/(x*y) );
    p_max = floor( ( K_max - k0 - x*n )/(x*y) );
    for p=[p_min:p_max]
        %ETSI ES 201980 / 8.4.4.1 / Table 90
        k = k0 + x*n + x*y*p;         
        gain_ref_cells_k = [ gain_ref_cells_k, k + s * K_modulo ];      
    end
end

unused_carriers_k = [0];
if (robustness_mode == 1)
    unused_carriers_k = [-1, 0, 1];
end

freq_ref_cells_k = freq_ref_cells_k_list{robustness_mode};
time_ref_cells_k = time_ref_cells_k_list{robustness_mode};

SDC_Demapper = K_dc + setdiff([K_min:K_max], [freq_ref_cells_k.', time_ref_cells_k.', gain_ref_cells_k, unused_carriers_k] );
for i = [1:SDC_symbols(robustness_mode) - 1]
    SDC_Demapper = [SDC_Demapper, K_dc + setdiff(i * K_modulo + [K_min:K_max], [i * K_modulo + freq_ref_cells_k.', gain_ref_cells_k, i * K_modulo + unused_carriers_k] )];
end


% SDC_Demapper = K_dc + setdiff( reshape(ones(SDC_symbols(robustness_mode),1)*[K_min:K_max]+(K_modulo * [0:SDC_symbols(robustness_mode) - 1]') * ...
%                                 ones(1,carrier_per_symbol), 1, SDC_symbols(robustness_mode) * carrier_per_symbol), ...
%                                 [reshape(ones(SDC_symbols(robustness_mode),1)*[unused_carriers_k, freq_ref_cells_k.'] +(K_modulo * [0:SDC_symbols(robustness_mode) - 1]') * ...
%                                 ones(1,length([unused_carriers_k, freq_ref_cells_k.'])), 1, SDC_symbols(robustness_mode) * length([unused_carriers_k, freq_ref_cells_k.'])),...
%                                 time_ref_cells_k.', gain_reference_cells_k]);
                        