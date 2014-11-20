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
%%  get_symbol_index.m                                                        %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  Description:                                                              %
%%  Estimation of the position of the first symbol in a frame                 %
%%  Input is a sequence of N symbols, where N is the number of symbols in a   %
%%  frame.                                                                    %
%%                                                                            %
%%  Invoked by demodulation_and_equalization.m                                %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function symbol_0 = get_symbol_index( symbol_buffer, symbols_per_frame, time_ref_cells_k, time_ref_cells_theta_1024, K_dc, K_modulo )

S = reshape( symbol_buffer(1:(symbols_per_frame*K_modulo)), K_modulo, symbols_per_frame );
sum_real_xx = zeros(1,symbols_per_frame);

for (n=1:(length(time_ref_cells_k)-1))
   if( time_ref_cells_k(n+1)-time_ref_cells_k(n)==1 )
      k1_index = K_dc + time_ref_cells_k(n);
      k2_index = K_dc + time_ref_cells_k(n+1);
      phi1 = 2*pi*time_ref_cells_theta_1024(n)/1024;
      phi2 = 2*pi*time_ref_cells_theta_1024(n+1)/1024;
     
   	sum_real_xx = sum_real_xx + real( S(k1_index,1:symbols_per_frame).*exp(-j*phi1).* ...
      	conj(  S(k2_index,1:symbols_per_frame).*exp(-j*phi2)  ) );
   end
end
[dummy,symbol_0] = max( abs(sum_real_xx) );
