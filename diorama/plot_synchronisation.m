%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  University of Kaiserslautern, Institute of Communications Engineering     %
%%  Copyright (C) 2004 Andreas Dittrich                                       %
%%                                                                            %
%%  Author(s)    : Andreas Dittrich (dittrich@eit.uni-kl.de)                  %
%%  Project start: 01.01.2005                                                 %
%%  Last change: 02.05.2005, 12:00                                            %
%%  Changes    : |                                                            %
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
%%  Last change: 02.05.2005, 11:30                                            %
%%  By         : Andreas Dittrich                                             %
%%  Description: smaller windows size and new default position                %
%%               display of frequency offset instead of freq. off. correction %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  plot_synchronisation.m                                                    %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function result = plot_synchronisation( varargin )
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
	sync_figure_xtick_pos ...
   time_offset_log_sequence hnd_sync_time_offset_seq hnd_time_offset_axes ...
   freq_offset_log_sequence hnd_sync_freq_offset_seq hnd_freq_offset_axes ...
   time_per_symbol_last ...
   is_in_use
   
settings_number = 2;
TAG_NAME = mfilename;
FIGURE_PROCEDURE_NAME = mfilename;
FIGURE_NAME = 'Synchronisation';

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
   position_this_figure = [1+floor(myScreenSize(3)*0.55), 1+floor(myScreenSize(4)*0.05), floor(myScreenSize(3)/10)*4, floor(myScreenSize(4)/10)*4];
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
         ,'MenuBar','none' ...         
         ,'NumberTitle','off' ...
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
         ,'MenuBar','none' ...         
         ,'NumberTitle','off' ...
         );
      settings_handler(7,settings_number,1);
      subhandles_valid = [];
   end
   result = hnd_this_figure;
   
   %get data to plot
	time_offset_log = varargin{2};
   freq_offset_log = varargin{3};
   freq_offset_init = 0;%varargin{4};
   N_symbols_needed = varargin{5};
   time_per_symbol = varargin{6};
   
	N_pilot_angle_sequence = 400;
   
   if ( N_symbols_needed>0 )	%is data valid to plot?
   
   	if ( isempty(subhandles_valid) ) %do we have the sub-handles to directly refresh data?
         figure(hnd_this_figure);	%lets do work on this figure
         
         hnd_nodatatext = findobj('Tag', [TAG_NAME,'_nodatatext']);
         if (~isempty(hnd_nodatatext))
            delete(hnd_nodatatext);
         end
      	sync_figure_xtick_pos = 0;
      
      	subplot( 'position', [0.1 0.6 0.8 0.3] );
      	time_offset_log_sequence = [NaN*ones(1,N_pilot_angle_sequence)];
      	hnd_sync_time_offset_seq = plot( [(1-N_pilot_angle_sequence):0]*time_per_symbol, time_offset_log_sequence, '-');
      	axis( [(1-N_pilot_angle_sequence)*time_per_symbol,0.5,-1,1] );
      	hnd_time_offset_axes = gca;
      	set(gca,'XTickMode','manual');
         set(gca,'XTickLabelMode','manual');
         set(gca,'XTickLabel', [] );
			title('time-offset correction (w/o sample rate conversion)');
			xlabel('time (grid spacing = 1s)');
			ylabel('(ms)');
      	grid on;

      	subplot( 'position', [0.1 0.1 0.8 0.3] );
      	freq_offset_log_sequence = [NaN*ones(1,N_pilot_angle_sequence-length(freq_offset_log)), (freq_offset_log-freq_offset_init)];
      	hnd_sync_freq_offset_seq = plot( [(1-N_pilot_angle_sequence):0]*time_per_symbol, freq_offset_log_sequence, '-');
      	axis( [(1-N_pilot_angle_sequence)*time_per_symbol,0.5,-1,1] );
      	hnd_freq_offset_axes = gca;
      	set(gca,'XTickMode','manual');
         set(gca,'XTickLabelMode','manual');
         set(gca,'XTickLabel', [] );
			title('frequency-offset');
			xlabel('time (grid spacing = 1s)');
			ylabel('(Hz)');
         grid on;
         
         time_per_symbol_last = time_per_symbol;
	
         subhandles_valid = 1;	%next time we can directly write do the data vector of the figure
      else
         if (time_per_symbol_last ~= time_per_symbol)
	      	sync_figure_xtick_pos = 0;
   	   	time_offset_log_sequence = [NaN*ones(1,N_pilot_angle_sequence)];
	      	freq_offset_log_sequence = [NaN*ones(1,N_pilot_angle_sequence-length(freq_offset_log)), (freq_offset_log-freq_offset_init)];
      		set( hnd_sync_time_offset_seq, 'XDATA', [(1-N_pilot_angle_sequence):0]*time_per_symbol );
            set( hnd_sync_freq_offset_seq, 'XDATA', [(1-N_pilot_angle_sequence):0]*time_per_symbol );
            time_per_symbol_last = time_per_symbol;
         end
         
      	sync_figure_xtick_pos = rem( sync_figure_xtick_pos + N_symbols_needed*time_per_symbol, 1 );
      
      	time_offset_log_sequence = [time_offset_log_sequence((length(time_offset_log)+1):end),time_offset_log];
      	set( hnd_sync_time_offset_seq, 'YDATA', time_offset_log_sequence );
      	YLim = get(hnd_time_offset_axes, 'YLim');
      	Yshift = (YLim(2)-YLim(1))*1/2;
      	Yactual = time_offset_log_sequence(end);
      	if (Yactual>YLim(2))
         	YLim = YLim + ceil( (Yactual-YLim(2))/Yshift )*Yshift;
      	elseif (Yactual<YLim(1))
         	YLim = YLim + floor( (Yactual-YLim(1))/Yshift )*Yshift;
      	end
      	set( hnd_time_offset_axes, 'YLim', YLim );
      	set( hnd_time_offset_axes, 'XTick', [(-sync_figure_xtick_pos+(1-N_pilot_angle_sequence)*time_per_symbol):1:0.5]);
         
      	freq_offset_log_sequence = [freq_offset_log_sequence((length(freq_offset_log)+1):end),(freq_offset_log-freq_offset_init)];
      	set( hnd_sync_freq_offset_seq, 'YDATA', freq_offset_log_sequence );
      	YLim = get(hnd_freq_offset_axes, 'YLim');
      	Yshift = (YLim(2)-YLim(1))*1/2;
      	Yactual = freq_offset_log_sequence(end);
      	if (Yactual>YLim(2))
         	YLim = YLim + ceil( (Yactual-YLim(2))/Yshift )*Yshift;
      	elseif (Yactual<YLim(1))
         	YLim = YLim + floor( (Yactual-YLim(1))/Yshift )*Yshift;
      	end
      	set( hnd_freq_offset_axes, 'YLim', YLim );
         set( hnd_freq_offset_axes, 'XTick', [(-sync_figure_xtick_pos+(1-N_pilot_angle_sequence)*time_per_symbol):1:0.5]);
         
   	end
      
   end 
   
end	%if switch


is_in_use = [];

   
	   
   

