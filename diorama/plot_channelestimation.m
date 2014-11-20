%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  University of Kaiserslautern, Institute of Communications Engineering     %
%%  Copyright (C) 2004 Andreas Dittrich                                       %
%%                                                                            %
%%  Author(s)    : Andreas Dittrich (dittrich@eit.uni-kl.de)                  %
%%  Project start: 01.01.2005                                                 %
%%  Last change: 30.06.2005, 18:00                                            %
%%  Changes    : ||                                                           %
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
%%  Last change: 29.04.2005, 16:00                                            %
%%  By         : Andreas Dittrich                                             %
%%  Description: small changes in display of impulse response and guard int.  %
%%               new: optional log-plot of impulse response                   %
%%               smaller windows size                                         %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  Last change: 30.06.2005, 18:00                                            %
%%  By         : Andreas Dittrich                                             %
%%  Description: BUG: log(0) error in plot routine if H is zero - fixed       %
%%               size adjustment of linear-impulse-plot                       %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  plot_channelestimation.m                                                  %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function result = figure_procedure( varargin )
% (-1) close figure and reset figure properties, result should be zero
% (0) close figure, result should be zero
% (1) create empty figure without data if not allready open, result==handle to figure
% (2, ...) update/create figure with data in the following arguments, result==handle to figure
%
% (10) do window into foreground
% (11) do window into background
% (12) do minimize figure
% (13, [xpos,ypos,xwidth,ywidth]) == set position of actual window or of position window will have if created
% (14) get position of actual window or last opened window, empty if not available

persistent position_this_figure ...		%position of figure
   hnd_this_figure subhandles_valid ...			%handles available to directly access data
	...
   hnd_channelestimation_impulse hnd_channelestimation_power hnd_channelestimation_power_pilots ...
	hnd_channelestimation_phase hnd_channelestimation_phase_pilots hnd_Tg_minus_impulse hnd_Tg_plus_impulse hnd_checkbox_logplot...
   Tu_last Tg_last length_h_last impulse_response_logplot ...
	hnd_channelest_impulse_title hnd_channelest_power_title ...   
   is_in_use
   
   settings_number = 4;
TAG_NAME = mfilename;
FIGURE_PROCEDURE_NAME = mfilename;
FIGURE_NAME = 'Channel Estimation';

result=-1;
%avoid reentrance 
if ( ~isempty(is_in_use) )
   return; %ignore this call
end

is_in_use = varargin{1};


if( isempty(hnd_this_figure) )
   hnd_this_figure = findobj('Tag', TAG_NAME);
   subhandles_valid = [];
end

if( isempty(position_this_figure) )
   myScreenSize = get(0,'ScreenSize');
   position_this_figure = [1+floor(myScreenSize(3)*0.45), 1+floor(myScreenSize(4)*0.55), floor(myScreenSize(3)/10)*4, floor(myScreenSize(4)/10)*4];
end


switch varargin{1}
   
case -1,
   if ( ~isempty(hnd_this_figure) )
      delete( hnd_this_figure );
      settings_handler(7,settings_number,0);
      position_this_figure = [];
      hnd_this_figure = [];
   end
   result = 0;
   
case 0,
   if ( ~isempty(hnd_this_figure) )
      position_this_figure = get( hnd_this_figure, 'Position' );
      delete( hnd_this_figure );
      settings_handler(7,settings_number,0);
      hnd_this_figure = [];
   end
   result = 0;
   
case 1,
   if ( isempty(hnd_this_figure)  )
      hnd_this_figure = figure('Tag', TAG_NAME ...
         ,'Position', position_this_figure ...
      	,'CloseRequestFcn', [FIGURE_PROCEDURE_NAME,'(0);'] ...
         ,'Name', FIGURE_NAME ...
         ,'NumberTitle','off' ...
         ,'MenuBar','none' ...
         );
      
      uicontrol('Parent', hnd_this_figure, ...
      'BackgroundColor',[0.8 0.8 0.8], ...
      'Units', 'normalized', ...
      'FontUnits', 'points', ...
		'Position',[0.2 0.5 0.6 0.1], ...
		'String','no data available', ...
      'Style','text', ...
      'Tag', [TAG_NAME,'_nodatatext'], ...
      'FontSize', 12 ...
      );
   
      settings_handler(7,settings_number,1);
      subhandles_valid = [];
   end
 	result = hnd_this_figure;

case 2,      
   %do we have a figure to plot?
   if ( isempty(hnd_this_figure) )
      hnd_this_figure = figure('Tag', TAG_NAME ...
         ,'Position', position_this_figure ...
      	,'CloseRequestFcn', [FIGURE_PROCEDURE_NAME,'(0);'] ...
         ,'Name', FIGURE_NAME ...
         ,'NumberTitle','off' ...
         ,'MenuBar','none' ...
         );
      settings_handler(7,settings_number,1);
      subhandles_valid = [];
   end
   result = hnd_this_figure;
   
   %get data to plot
   H = varargin{2};
   symbol_position_offset = varargin{3};
   Tg = varargin{4};
   Tu = varargin{5};
   K_min = varargin{6};
   K_max = varargin{7};
   K_dc = varargin{8};
   K_modulo = varargin{9};
   gain_ref_cells_k = varargin{10};
   X_normalized_training_cells = varargin{11};
   %delay_spread_ms = varargin{12};
   %doppler_spread_Hz = varargin{13};

   if ( length(H)==(K_max-K_min+1) )	%is data valid to plot?
      
   	if ( isempty(subhandles_valid) ) %do we have the sub-handles to directly refresh data?
         figure(hnd_this_figure);	%lets do work on this figure
         
         hnd_nodatatext = findobj('Tag', [TAG_NAME,'_nodatatext']);
         if (~isempty(hnd_nodatatext))
            delete(hnd_nodatatext);
         end
         
         if (isempty(impulse_response_logplot)) 
            impulse_response_logplot = 0; 
         end
         
         hnd_checkbox_logplot = uicontrol('Parent', hnd_this_figure, ...
            'Units', 'normalized', 'BackgroundColor',[0.8 0.8 0.8], ...
            'Position',[0.8 0.91 0.15 0.05], 'Style','checkbox', ...
            'TooltipString','log-plot', ...
            'String','log-plot', ...
            'Value', impulse_response_logplot, 'Callback',[FIGURE_PROCEDURE_NAME,'(3);']);
         
         subplot( 'position', [0.1 0.65 0.8 0.25] );
         Tc = 2^ceil( log2( length(H)*4 ) );
         Tch = ceil(Tc/2);
         h_abs = abs( fft(conj([H.*(1-cos([1:length(H)]/(length(H)+1)*2*pi)');zeros(Tc-length(H),1)])) )/(length(H));
         h_gain = 1; %h_gain = sqrt(h_abs'*h_abs);
        
         if (~impulse_response_logplot)
			hnd_channelestimation_impulse = plot( ([1:Tc]-Tch)/Tc*Tu/12,[h_abs((Tch+1):end);h_abs(1:Tch)]/h_gain, '-' );
         axis([-Tg/12, Tg/12,0,1.5]);
         hold on;
      	hnd_Tg_minus_impulse = plot( [(symbol_position_offset-Tg/2)/12,(symbol_position_offset-Tg/2)/12], [0,10], '--r');
      	hnd_Tg_plus_impulse = plot( [(symbol_position_offset+Tg/2)/12,(symbol_position_offset+Tg/2)/12], [0,10], '--r');
         hold off;
			ylabel('|h(t)| (linear)');
      	else
			hnd_channelestimation_impulse = plot( ([1:Tc]-Tch)/Tc*Tu/12, 20*log10( max([h_abs((Tch+1):end);h_abs(1:Tch)],10^(-60/20)) ), '-' );
         axis([-Tg/12, Tg/12,-60,20]);
         hold on;
      	hnd_Tg_minus_impulse = plot( [(symbol_position_offset-Tg/2)/12,(symbol_position_offset-Tg/2)/12], [-60,20], '--r');
      	hnd_Tg_plus_impulse = plot( [(symbol_position_offset+Tg/2)/12,(symbol_position_offset+Tg/2)/12], [-60,20], '--r');
         hold off;
			ylabel('20lg(|h(t)|)  (dB)');            
	      end   
         hnd_channelest_impulse_title = title( sprintf('estimated impulse response') );
         xlabel('t (ms)');
         grid on;
         
         
         subplot( 'position', [0.1 0.1 0.35 0.4] );
			hnd_channelestimation_power = plot([K_min:K_max],20*log10(abs(H)/h_gain + 10^(-5)), '.-');
			hold on
			hnd_channelestimation_power_pilots = plot(rem(gain_ref_cells_k, K_modulo)-K_dc+1, 20*log10(abs(X_normalized_training_cells)/h_gain + 10^(-5)),'xr');
			hold off;
			axis([-Tu/2,+Tu/2,-20,20]);
         hnd_channelest_power_title = title( sprintf('|H(f)|') );
         xlabel('carrier no.');
         ylabel('magnitude (dB)')
         grid on;

         subplot( 'position', [0.55 0.1 0.35 0.4] );
			hnd_channelestimation_phase = plot([K_min:K_max],angle(H)/pi,'.');
			hold on
			hnd_channelestimation_phase_pilots = plot(rem(gain_ref_cells_k, K_modulo)-K_dc+1, angle(X_normalized_training_cells)/pi,'xr');
			hold off;
			axis([-Tu/2,+Tu/2,-1,1]);
         title('angle( H(f) )');
         xlabel('carrier no.');
         ylabel('phase (pi)');
         grid on;
         
         Tu_last = Tu;
         Tg_last = Tg;
         length_h_last = Tc;
         
         subhandles_valid = 1;	%next time we can directly write do the data vector of the figure
      else
         set( hnd_channelest_impulse_title , 'String', sprintf('estimated impulse response' ) );
         set( hnd_channelest_power_title, 'String', sprintf('|H(f)|') );
         
         Tc = 2^ceil( log2( length(H)*4 ) );
         Tch = ceil(Tc/2);
         h_abs = abs( fft(conj([H.*(1-cos([1:length(H)]/(length(H)+1)*2*pi)');zeros(Tc-length(H),1)])) )/(length(H));         
         h_rms = sqrt(H'*H/length(H));
         
         impulse_response_logplot_new = get( hnd_checkbox_logplot, 'Value');
         if (impulse_response_logplot ~= impulse_response_logplot_new)
            impulse_response_logplot = impulse_response_logplot_new;
            if (impulse_response_logplot==1)
               set( get(hnd_channelestimation_impulse,'Parent'), 'YLim', [-60,20] );
					set( get(get(hnd_channelestimation_impulse,'Parent'),'YLabel'), 'String', '20lg(|h(t)|)  (dB)');             
     				set( hnd_Tg_minus_impulse, 'YDATA', [-60,20] );
     				set( hnd_Tg_plus_impulse, 'YDATA', [-60,20] );
            else
               set( get(hnd_channelestimation_impulse,'Parent'), 'YLim', [0,h_rms*1.2] );
					set( get(get(hnd_channelestimation_impulse,'Parent'),'YLabel'), 'String', '|h(t)| (linear)');             
     				set( hnd_Tg_minus_impulse, 'YDATA', [0,10] );
     				set( hnd_Tg_plus_impulse, 'YDATA', [0,10] );
            end
         end

         set( hnd_channelestimation_impulse, 'XDATA', ([1:Tc]-Tch)/Tc*Tu/12 );
         if (~impulse_response_logplot)
            set( hnd_channelestimation_impulse, 'YDATA', [h_abs((Tch+1):end);h_abs(1:Tch)] );
            YLim = get(get(hnd_channelestimation_impulse, 'Parent'), 'YLim');
            Yctrl = h_rms*1.2-YLim(2);
            if (Yctrl>0)
               YLim(2) = YLim(2) + Yctrl;
            else
               YLim(2) = YLim(2) + 0.1*Yctrl;
            end
            
      		set( get(hnd_channelestimation_impulse, 'Parent'), 'YLim', YLim );
            
         else
            set( hnd_channelestimation_impulse, 'YDATA', 20*log10( max([h_abs((Tch+1):end);h_abs(1:Tch)],10^(-80/20)) ) );
         end
         
     		set( hnd_Tg_minus_impulse, 'XDATA', [(symbol_position_offset-Tg/2)/12,(symbol_position_offset-Tg/2)/12] );
     		set( hnd_Tg_plus_impulse, 'XDATA', [(symbol_position_offset+Tg/2)/12,(symbol_position_offset+Tg/2)/12] );
         
         set( hnd_channelestimation_power, 'XDATA', [K_min:K_max] );        
         set( hnd_channelestimation_power, 'YDATA', 20*log10(abs(H) + 10^(-5)) );        
         set( hnd_channelestimation_power_pilots, 'XDATA', rem(gain_ref_cells_k, K_modulo)-K_dc+1 );
         set( hnd_channelestimation_power_pilots, 'YDATA', 20*log10(abs(X_normalized_training_cells) + 10^(-5) ) );
         set( hnd_channelestimation_phase, 'XDATA', [K_min:K_max] );
         set( hnd_channelestimation_phase, 'YDATA', angle(H)/pi );
         
         
         YLim = get(get(hnd_channelestimation_power, 'Parent'), 'YLim');
         Ymax = 20*log10( max(abs(H)) + 10^(-5) );
         if (Ymax>YLim(2))
            YLim = YLim - YLim(2) + 10*ceil( Ymax/10 );
         elseif (Ymax<(YLim(2)-20))
            YLim = YLim - YLim(2) + 10 + 10*ceil( Ymax/10 );
         end
      	set( get(hnd_channelestimation_power, 'Parent'), 'YLim', YLim );
         
         
         set( hnd_channelestimation_phase_pilots, 'XDATA', rem(gain_ref_cells_k, K_modulo)-K_dc+1 );
         set( hnd_channelestimation_phase_pilots, 'YDATA', angle(X_normalized_training_cells)/pi );
         
         
         if ( (Tu~=Tu_last) | (Tg~=Tg_last) | (Tc~=length_h_last) )
	         set( get(hnd_channelestimation_impulse,'Parent'), 'XLim', [-Tg/12,Tg/12] );
   	      set( get(hnd_channelestimation_power,'Parent'), 'XLim', [-Tu/2,+Tu/2] );
            set( get(hnd_channelestimation_phase,'Parent'), 'XLim', [-Tu/2,+Tu/2] );
            Tu_last = Tu;
            Tg_last = Tg;
            length_h_last = Tc;
         end
      end
      
   end 
   
end	%if switch


is_in_use = [];

   
	   
   

