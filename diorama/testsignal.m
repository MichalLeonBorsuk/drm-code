
function [samples_output, Zf] = testsignal( N_samples, P, Zi )

% initialize fixed Parameters
pilot_cells_k = P(P(1):(P(2)-1)); 
pilot_cells = P(P(2):(P(3)-1)); 
FAC_cells_k = P(P(3):(P(4)-1)); 
SDC_cells_k = P(P(4):(P(5)-1)); 
data_cells_k = P(P(5):(P(6)-1)); 
unused_cells_k = P(P(6):(P(7)-1)); 
K_dc = P(P(7));
K_modulo = P(P(8));
symbols_per_frame = P(P(9));
Ts = P(P(10));
Tu = P(P(11));
Tg = Ts-Tu;
K_min = P(P(12));
K_max = P(P(13));
SDC_mode = P(P(14));	% 0 = 4-QAM, 1 = 16-QAM
MSC_mode = P(P(15)); % 0/1/2 = 64-QAM, 3 = 16-QAM
mean_energy_of_used_cells = P(P(16));

% initialize state variables
if (Zi(1)~=0),
   samples_buffer = Zi(Zi(1):(Zi(2)-1));
   samples_buffer_ptr = Zi(Zi(2));
   samples_buffer_level = Zi(Zi(3));
   %maybe init seed of random generators here!
else
   samples_buffer = zeros(4*Ts*symbols_per_frame,1);
   samples_buffer_ptr = 1;
   samples_buffer_level = 0;
end

% initialize local variables
frame_buffer = zeros(K_modulo, symbols_per_frame);
X = zeros(4*Tu,1);
x = zeros(4*Tu,1);

samples_output = zeros(1, N_samples);
samples_output_ptr = 1;
samples_output_level = N_samples;

N_data_cells = length(data_cells_k);
N_FAC_cells = length(FAC_cells_k);
N_SDC_cells = length(SDC_cells_k);

factor_4QAM = 2/sqrt(2);
factor_16QAM = 2/sqrt(10);
factor_64QAM = 2/sqrt(42);

% main processing loop
while (samples_output_level>0)
   
   if (samples_buffer_level<=0)   % create new samples for a complete frame
      % add FAC-symbols (4 QAM)
	   %u_i = ( floor( 2*rand(N_FAC_cells,1) ) - 0.5 )*factor_4QAM;
      %u_q = ( floor( 2*rand(N_FAC_cells,1) ) - 0.5 )*factor_4QAM;
      u_i = ( [ 1, 0, 0, 1, 1, 1, 0, 0, ...
				1, 1, 1, 0, 1, 0, 0, 0, ...
				1, 1, 1, 1, 1, 1, 1, 0, ...
				1, 0, 0, 0, 1, 1, 1, 0, ...
				0, 0, 0, 0, 1, 0, 1, 1, ...
				0, 1, 1, 1, 0, 1, 0, 0, ...
				0, 1, 1, 0, 1, 1, 1, 0, ...
				0, 0, 0, 0, 0, 0, 1, 1, ...
            1 ] - 0.5 )*factor_4QAM;
      u_q = ( [ 0, 0, 0, 1, 0, 0, 0, 1, ...
				0, 0, 1, 0, 1, 0, 1, 1, ...
				1, 1, 1, 0, 0, 0, 0, 1, ...
				0, 0, 0, 0, 0, 1, 1, 1, ...
				1, 1, 1, 0, 0, 1, 0, 1, ...
				1, 1, 0, 1, 0, 1, 0, 0, ...
				1, 1, 1, 0, 0, 0, 1, 0, ...
				0, 1, 0, 1, 1, 0, 1, 1, ...
            1 ] - 0.5 )*factor_4QAM;
	   frame_buffer( FAC_cells_k + 1 ) = u_i + j*u_q;
      
      % add SDC-symbols 
      if (SDC_mode==0)	%4-QAM
	   	u_i = ( floor( 2*rand(N_SDC_cells,1) ) - 0.5 )*factor_4QAM;
	   	u_q = ( floor( 2*rand(N_SDC_cells,1) ) - 0.5 )*factor_4QAM;
      else	%16-QAM
	   	u_i = ( floor( 4*rand(N_SDC_cells,1) ) - 1.5 )*factor_16QAM;
	   	u_q = ( floor( 4*rand(N_SDC_cells,1) ) - 1.5 )*factor_16QAM;
      end
	   frame_buffer( SDC_cells_k + 1 ) = u_i + j*u_q;
      
      % add MSC-symbols
      if (MSC_mode==3)	%16-QAM
	   	u_i = ( floor( 4*rand(N_data_cells,1) ) - 1.5 )*factor_16QAM;
	   	u_q = ( floor( 4*rand(N_data_cells,1) ) - 1.5 )*factor_16QAM;
      else	%64-QAM
	   	u_i = ( floor( 8*rand(N_data_cells,1) ) - 3.5 )*factor_64QAM;
	   	u_q = ( floor( 8*rand(N_data_cells,1) ) - 3.5 )*factor_64QAM;
      end
	   frame_buffer( data_cells_k + 1 ) = u_i + j*u_q;
      
   	% add scattered pilots
      frame_buffer( pilot_cells_k + 1 ) = pilot_cells;
      
      % convert frame to signal (frequency -> ifft -> time)
      for (n=0:(symbols_per_frame-1)),
         X(Tu+1+[K_min:K_max]) =  frame_buffer(n*K_modulo+K_dc+[K_min:K_max]);
         % ifft(x) = conj( conj(fft(x)) )
   		x = real( fft( conj(X) ) )*sqrt(2/Tu/100);
   		%add guard interval
   		samples_buffer(n*4*Ts+[1:(4*Ts)]) = [x((end-(4*Tg)+1):end);x];
      end
      
      %reset samples_buffer pointer and level
      samples_buffer_ptr = 1;
      samples_buffer_level = 4*Ts*symbols_per_frame;
   end
   
   N_samples_to_write = min( samples_buffer_level, samples_output_level );
   samples_output(samples_output_ptr:(samples_output_ptr+N_samples_to_write-1)) = samples_buffer(samples_buffer_ptr:(samples_buffer_ptr+N_samples_to_write-1));
   
   samples_output_ptr = samples_output_ptr + N_samples_to_write;
   samples_output_level = samples_output_level - N_samples_to_write;
   
   samples_buffer_ptr = samples_buffer_ptr + N_samples_to_write;
   samples_buffer_level = samples_buffer_level - N_samples_to_write;
   
end


% build up final state
% create header
Zf_Header = [0; ...
      prod(size(samples_buffer)); ...
      1; 1 ];
Zf_Header = length(Zf_Header) + 1 + cumsum(Zf_Header);	% Jetzt ist Zf(i) der Index des 1. Elements der i. Komponente
% add components
Zf = [Zf_Header; ...
      samples_buffer(:); ...
  		samples_buffer_ptr; samples_buffer_level; ...
 ];

