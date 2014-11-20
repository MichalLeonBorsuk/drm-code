function [Params] = testsignal_init( model_parameters )


%load constants of DRM Standard
load('equalization_init.mat');

% set default parameters
robustness_mode = 1;	% == Mode A
spectrum_occupancy = 2; 
K_modulo = 288*2;
K_dc = 145;
SDC_mode = 0;	% 0 = 4-QAM, 1 = 16-QAM
MSC_mode = 0; % 0/1/2 = 64-QAM, 3 = 16-QAM

% get (overwrite) model parameters
N_parameter = length(model_parameters)/2;
for i=1:N_parameter,
   eval( [model_parameters{2*i-1}, ' = model_parameters{2*i};'] );
end

%argument checking
mode_and_occupancy_code = mode_and_occupancy_code_table( (robustness_mode-1)*6 + spectrum_occupancy + 1);   
if (mode_and_occupancy_code<0) %not supported
   warning('testsignal generator: choosen mode/spectrum_occupancy not supported - using spectrum_occupancy=3');
   %setting default parameters
   spectrum_occupancy = 3;
	mode_and_occupancy_code = mode_and_occupancy_code_table( (robustness_mode-1)*6 + spectrum_occupancy + 1);   
end

% initialize variables
Ts = Ts_list(robustness_mode);
Tu = Tu_list(robustness_mode);
K_min = K_min_K_max_list(spectrum_occupancy*2 + (robustness_mode-1)*2*6 + 1);
K_max = K_min_K_max_list(spectrum_occupancy*2 + (robustness_mode-1)*2*6 + 2);
carrier_per_symbol = K_max - K_min + 1;
symbols_per_frame = symbols_per_frame_list(robustness_mode);   
Tu = Tu_list(robustness_mode);
freq_ref_cells_k_list = { [18;54;72], [16;48;64], [11;33;44], [7;21;28] };
freq_ref_cells_theta_1024_list = { [205;836;215], [331;651;555], [214;392;242], [788;1014;332] };
mean_energy_of_used_cells = mean_energy_of_used_cells_list(spectrum_occupancy + (robustness_mode-1)*6 + 1);

%create all pilot cells

%gain ref cells (includes maybe time/frequency cells)
gain_ref_cells_k = gain_ref_cells_k_list{mode_and_occupancy_code+1};
gain_ref_cells_k = gain_ref_cells_k(:);
gain_ref_cells_theta_1024 = gain_ref_cells_theta_1024_list{mode_and_occupancy_code+1};
gain_ref_cells_a = gain_ref_cells_a_list{mode_and_occupancy_code+1};
gain_ref_cells = gain_ref_cells_a(:).*exp(j*gain_ref_cells_theta_1024(:)*2*pi/1024);

%time ref cells (includes maybe gain/frequency cells)
time_ref_cells_k = time_ref_cells_k_list{robustness_mode};
time_ref_cells_theta_1024 = time_ref_cells_theta_1024_list{robustness_mode};
time_ref_cells_a = sqrt(2);
time_ref_cells = time_ref_cells_a*exp(j*time_ref_cells_theta_1024*2*pi/1024);

%frequency cells (includes maybe gain/time cells)
freq_ref_cells_k = freq_ref_cells_k_list{robustness_mode}*ones(1,symbols_per_frame) + ones(3,1)*[0:(symbols_per_frame-1)]*carrier_per_symbol;
freq_ref_cells_k = freq_ref_cells_k(:);
freq_ref_cells_theta_1024 = freq_ref_cells_theta_1024_list{robustness_mode}*ones(1,symbols_per_frame);
if (robustness_mode == 4)
   temp = reshape( freq_ref_cells_theta_1024, 3, symbols_per_frame );
   temp(1:2,2:2:end) = temp(1:2,2:2:end) - 512;
   freq_ref_cells_theta_1024 = temp(:);
end
freq_ref_cells_a = sqrt(2);
freq_ref_cells = freq_ref_cells_a*exp(j*freq_ref_cells_theta_1024(:)*2*pi/1024);

%lets combine all pilot cells
[dummy, g_indx, t_indx] = union(gain_ref_cells_k, time_ref_cells_k);
pilot_cells_k = [gain_ref_cells_k(g_indx); time_ref_cells_k(t_indx)];
pilot_cells = [gain_ref_cells(g_indx); time_ref_cells(t_indx)];
[dummy, p_indx, f_indx] = union(pilot_cells_k, freq_ref_cells_k);
pilot_cells_k = [pilot_cells_k(p_indx); freq_ref_cells_k(f_indx)];
pilot_cells = [pilot_cells(p_indx); freq_ref_cells(f_indx)];
[pilot_cells_k, p_indx] = sort(pilot_cells_k);
pilot_cells = pilot_cells(p_indx);

%reformat considering K_modulo and K_dc
temp = floor( (pilot_cells_k-K_min)/carrier_per_symbol )*(K_modulo-carrier_per_symbol);
pilot_cells_k = ( pilot_cells_k + temp ) + ( K_dc - 1 );

%FAC cells
FAC_cells_k = Create_FAC_Demapper(robustness_mode, K_dc - 1, K_modulo);
FAC_cells_k = FAC_cells_k(:);

%SDC cells
SDC_cells_k = Create_SDC_Demapper(robustness_mode, spectrum_occupancy, K_dc - 1, K_modulo);
SDC_cells_k = SDC_cells_k(:);

%unused cells (here we only consider unused cells between K_min..K_max)
if (robustness_mode == 1)
   unused_cells_k = [-1;0;1]*ones(1,symbols_per_frame) + ones(3,1)*[0:(symbols_per_frame-1)]*K_modulo + ( K_dc - 1 );
   unused_cells_k = unused_cells_k(:);
else
   unused_cells_k = [0:(symbols_per_frame-1)]'*K_modulo;
end

%data cells
all_cells_k = [K_min:K_max]'*ones(1,symbols_per_frame) + ones(carrier_per_symbol,1)*[0:(symbols_per_frame-1)]*K_modulo + ( K_dc - 1 );
data_cells_k = setdiff( all_cells_k(:), [pilot_cells_k; FAC_cells_k; SDC_cells_k; unused_cells_k] );


% Generate Parameter Vector
% create header
Params_Header =  [0; ...
      length(pilot_cells_k); length(pilot_cells); ...
      length(FAC_cells_k); length(SDC_cells_k); length(data_cells_k); length(unused_cells_k); ...
      1; 1; 1; ...
      1; 1; 1; 1; ...
   	1; 1; 1 ];
Params_Header =  length(Params_Header) + 1 + cumsum(Params_Header);	% Jetzt ist P(i) der Index des 1. Elements der i. Komponente
% add components
Params = [Params_Header; ...
      pilot_cells_k; pilot_cells; ...
      FAC_cells_k; SDC_cells_k; data_cells_k; unused_cells_k; ...
      K_dc; K_modulo; symbols_per_frame; ...
      Ts; Tu; K_min; K_max; ...
      SDC_mode; MSC_mode; mean_energy_of_used_cells ];
   