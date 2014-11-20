%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  University of Kaiserslautern, Institute of Communications Engineering     %
%%  Copyright (C) 2005 Torsten Schorr, Andreas Dittrich                       %
%%                                                                            %
%%  Author(s)    : Torsten Schorr (schorr@eit.uni-kl.de)                      %
%%                 Andreas Dittrich (dittrich@eit.uni-kl.de)                  %
%%  Project start: 01.01.2005                                                 %
%%  Last change  : 03.03.2005                                                 %
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
%%  plot_snr_spectrum.m                                                       %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function result = plot_snr_spectrum( varargin )
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
   hnd_this_figure subhandles_valid ...	%handles available to directly access data
   is_in_use ...
   ...
   ...	%----------change from here------------------------------
   line_handle ...
   symbol_period_old snr_min_max...
   K_dc_old;
   
   settings_number = 5;
   
   
   %------------dont change begin ------------
TAG_NAME = mfilename;
FIGURE_PROCEDURE_NAME = mfilename;

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
   position_this_figure = [10, 300, floor(myScreenSize(3)/10)*4, floor(myScreenSize(4)/10)*4];
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
      hnd_this_figure = figure('Tag', TAG_NAME, ...
         'Position', position_this_figure, ...
      	'CloseRequestFcn', [FIGURE_PROCEDURE_NAME,'(0);'], ...
         'Name', 'SNR spectrum', ...
         'NumberTitle','off', ...
        'MenuBar','none', ...
         'Renderer', 'zbuffer' );
      
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
         ,'Name', FIGURE_PROCEDURE_NAME ...
        ,'MenuBar','none' ...
         ,'Renderer', 'zbuffer' );
      settings_handler(7,settings_number,1);
      subhandles_valid = [];
   end
   result = hnd_this_figure;
   %------------dont change end ------------
   
   %get data to plot
   signal_to_noise_ratio = varargin{2};
   MSC_used_carriers = varargin{3};
   symbol_period = varargin{4};
   K_dc = varargin{5};
   
   
   hnd_nodatatext = findobj('Tag', [TAG_NAME,'_nodatatext']);
   if (~isempty(hnd_nodatatext))
       delete(hnd_nodatatext);
   end
   
   if (~isequal(symbol_period, symbol_period_old) || ~isequal(K_dc, K_dc_old))
       subhandles_valid = [];
    end
     
   symbol_period_old = symbol_period;
   K_dc_old = K_dc;   
   
   signal_to_noise_ratio_log = nan*ones(1,symbol_period);
   snr_zeros = find(signal_to_noise_ratio == 0);
   signal_to_noise_ratio_log(setdiff(MSC_used_carriers,snr_zeros)) = 10*log10(signal_to_noise_ratio(setdiff(MSC_used_carriers,snr_zeros)));   
   
   snr_min = min(signal_to_noise_ratio_log(setdiff(MSC_used_carriers,snr_zeros)));
   snr_max = max(signal_to_noise_ratio_log(setdiff(MSC_used_carriers,snr_zeros)));
   
   if (isempty(snr_min_max))
       snr_min_max(1) = floor((snr_min - 10)/5) * 5;
       snr_min_max(2) = ceil((snr_max + 10)/5) * 5;
   else
       if (snr_min < snr_min_max(1))
           snr_min_max(1) = floor((snr_min - 5)/5) * 5;
           subhandles_valid = [];
       elseif (snr_min > snr_min_max(1) + 15)
           snr_min_max(1) = floor((snr_min - 10)/5) * 5;
           subhandles_valid = [];
       end
       if (snr_max > snr_min_max(2))
           snr_min_max(2) = ceil((snr_max + 5)/5) * 5;
           subhandles_valid = [];
       elseif (snr_max < snr_min_max(2) - 15)
           snr_min_max(2) = ceil((snr_max + 10)/5) * 5;
           subhandles_valid = [];
       end
   end
   
   
   if ( isempty(subhandles_valid) ) %do we have the sub-handles to directly refresh data?
       
       set(0,'CurrentFigure',hnd_this_figure);
       
       line_handle = plot([1-K_dc:symbol_period-K_dc],signal_to_noise_ratio_log,'.-');
       
       xlim([1-K_dc, symbol_period-K_dc]);
       ylim(snr_min_max);
       title('SNR spectrum for MSC carriers');
       xlabel('carrier no.');
       ylabel('SNR / dB');
       grid on;
       
       
       subhandles_valid = 1;	%next time we can directly write do the data vector of the figure
   else
       set (line_handle(end), 'YData', signal_to_noise_ratio_log);
   end
   

   
end	%if switch


is_in_use = [];


   
	   
   

