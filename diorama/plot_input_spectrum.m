%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  University of Kaiserslautern, Institute of Communications Engineering     %
%%  Copyright (C) 2004 Andreas Dittrich                                       %
%%                                                                            %
%%  Author(s)    : Andreas Dittrich (dittrich@eit.uni-kl.de)                  %
%%  Project start: 01.01.2005                                                 %
%%  Last change: 29.04.2005, 16:00                                            %
%%  Changes    : |                                                          %
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
%%  Description: smaller windows size and new default position                %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  plot_input_spectrum.m                                                     %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function result = plot_input_spectrum( varargin )
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
   hnd_in_spectr_signal hnd_in_spectr_signal_title ...	% "
   hnd_in_spectr_fft hnd_in2_spectr_fft ...					% "
   
   settings_number = 1;
   
   %------------dont change begin ------------
TAG_NAME = mfilename;
FIGURE_PROCEDURE_NAME = TAG_NAME;
FIGURE_NAME = 'Input spectrum';

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
   position_this_figure = [1+floor(myScreenSize(3)/16)*4, 1+floor(myScreenSize(4)/12)*4, floor(myScreenSize(3)/10)*4, floor(myScreenSize(4)/10)*4];
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
         ,'MenuBar', 'none' ...
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
         ,'MenuBar', 'none' ...
         );
      settings_handler(7,settings_number,1);
      subhandles_valid = [];
   end
   result = hnd_this_figure;
   %------------dont change end ------------
   
   
   
   %get data to plot
   input_samples_buffer = varargin{2};
   input_samples_buffer_writeptr = varargin{3};
	rs_buffer = varargin{4};
	rs_buffer_writeptr = varargin{5};
   input_mean = varargin{6};
	input_rms = varargin{7};
   
   % check if data is valid
	N1_temp = 2^9;
	N2_temp = min( floor( (input_samples_buffer_writeptr-1)/N1_temp ), 10 );
   N1_temp2 = 2^7;
   N2_temp2 = min( floor( (rs_buffer_writeptr-1)/N1_temp2 ), 10 );
      
   if ( (N2_temp>0) && (N2_temp2>0) )	%is data valid to plot?
      temp = reshape( input_samples_buffer(1:(N1_temp*N2_temp)), N1_temp, N2_temp )';
      temp = 10*log10( mean( abs(fft(temp,[],2)).^2,1) + 1e-8 ) - 10*log10(N1_temp);
      temp2 = reshape( rs_buffer(1:(N1_temp2*N2_temp2)), N1_temp2, N2_temp2 ).';
      temp2 = 10*log10( mean( abs(fft(temp2,[],2)).^2,1) + 1e-8 ) - 10*log10(N1_temp2/4);
      
   	if ( isempty(subhandles_valid) ) %do we have the sub-handles to directly refresh data?
         figure(hnd_this_figure);	%lets do work on this figure
         
         hnd_nodatatext = findobj('Tag', [TAG_NAME,'_nodatatext']);
         if (~isempty(hnd_nodatatext))
            delete(hnd_nodatatext);
         end
         
         subplot( 'position', [0.1 0.8 0.8 0.1] );
         hnd_in_spectr_signal = plot( [1:N1_temp], input_samples_buffer(1:N1_temp) );
         axis([0,N1_temp,-1,1]);
         set(gca,'XTickLabelMode','manual');
         set(gca,'XTick',[]);
      	hnd_in_spectr_signal_title = title(sprintf('input signal, mean: %2.2f, RMS: %0.2f',input_mean, input_rms));
         
      	subplot( 'position', [0.1 0.1 0.5 0.6] );
         hnd_in_spectr_fft = plot( [1:(N1_temp/2)]/N1_temp*48, temp( 2:(N1_temp/2+1) ) ); 
         axis([0,24,-60,10]);      
         set(gca,'XTick',[0:2:24]);
         hold on;
      	plot( [6,6], [-60,10], '-r');
      	plot( [18,18], [-60,10], '-r' );
         plot( [12,12], [-60,10], '--r' );
         hold off;
      	title(sprintf('input signal powerspectrum',input_mean, input_rms));
      	xlabel('frequency (kHz)');
      	ylabel('normalized power density (dB)');
         grid on;
         
      	subplot( 'position', [0.65 0.1 0.25 0.6] );
         hnd_in2_spectr_fft = plot( [1:N1_temp2]/N1_temp2*12 - 6 , [temp2(N1_temp2/2:end),temp2(1:(N1_temp2/2-1))] ); 
         axis([-6,6,-60,10]);
         set(gca,'YTickLabelMode','manual');
         set(gca,'YTickLabel',[]);
         set(gca,'XTick',[-6,-4,-2,0,2,4,6]);
         grid on;
         title('12 kHz shifted + lowpass filtered');
         xlabel('frequency (kHz)');
         
         subhandles_valid = 1;	%next time we can directly write do the data vector of the figure
      else
      	set( hnd_in_spectr_fft, 'YDATA', temp( 2:(N1_temp/2+1) ) );
      	set( hnd_in_spectr_signal, 'YDATA', input_samples_buffer(1:N1_temp) );
      	set( hnd_in_spectr_signal_title,'String', sprintf('input signal, mean: %2.2f, rms: %0.2f',input_mean, input_rms) );
      	set( hnd_in2_spectr_fft, 'YDATA', [temp2(N1_temp2/2:end),temp2(1:(N1_temp2/2-1))] );
      end
      
   end 
   
end	%if switch


is_in_use = [];


   
	   
   

