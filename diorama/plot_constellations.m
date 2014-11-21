%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  University of Kaiserslautern, Institute of Communications Engineering     %
%%  Copyright (C) 2005 Torsten Schorr, Andreas Dittrich                       %
%%                                                                            %
%%  Author(s)    : Torsten Schorr (schorr@eit.uni-kl.de)                      %
%%                 Andreas Dittrich (dittrich@eit.uni-kl.de)                  %
%%  Project start: 01.01.2005                                                 %
%%  Last change  : 29.04.2005                                                 %
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
%% Last change: 29.04.2005, 19:10                                             %
%% By         : Torsten Schorr                                                %
%% Description: changed sizes                                                 %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  plot_constellations.m                                                     %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function result = plot_constellations( varargin )
% (-1) close figure and reset figure properties, result should be zero
% (0) close figure, result should be zero
% (1) create empty figure without data if not allready open, result==handle to figure
% (2, ...) update/create figure with data in the following arguments, result==handle to figure
%
% (10) put window into foreground
% (11) put window into background
% (12) put minimize figure
% (13, [xpos,ypos,xwidth,ywidth]) == set position of actual window or of position window will have if created
% (14) get position of actual window or last opened window, empty if not available

persistent position_this_figure ...		%position of figure
   hnd_this_figure ...
   is_in_use ...
   ...
   ...	%----------change from here------------------------------
   line_handles redraw hnd_this_axes...
   msc_mode sdc_mode ...
   checkbox_handles plot_selection data_available ...
   MSC_hists SDC_hists FAC_hists real_data imag_data ...
   current_MSC_hist current_SDC_hist current_FAC_hist ...
   MSC_hist SDC_hist FAC_hist frame_count call_count hist_view view_mode;
   
   settings_number = 3;   
    
   % histogram resolution:
   res_FAC = 50;
   res_SDC = 65;
   res_MSC = 100;
   
   plot_period_FAC = 2; % frames 
   plot_period_SDC = 3; % frames 
   plot_period_MSC = 4; % frames
   
   frames_to_merge = 20;
   merged_frames = 6;
   
   view4QAM = [0,0;0,90;-30,46];
   view16QAM = [0,0;0,90;-30,50];
   view64QAM = [0,0;0,90;-30,70];
   
   max_abs_val = 1.3;
   
   if (isempty(data_available))
       data_available = zeros(1,4);
   end
   if (isempty(redraw))
       redraw = 1;
   end
   if (isempty(call_count))
       call_count = 0;
   end
   if (isempty(plot_selection))
        plot_selection = 2;
   end
   if (isempty(hist_view))
        hist_view = 0;
   end
   if (isempty(call_count))
        call_count = 0;
   end
   if (isempty(view_mode))
        view_mode = 3;
   end
     
        
   plot_period = [0 plot_period_FAC plot_period_SDC plot_period_MSC];
   
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
end

if( isempty(position_this_figure) )
   myScreenSize = get(0,'ScreenSize');
   position_this_figure = [myScreenSize(3) - floor(myScreenSize(3)/10)*4 - 10, myScreenSize(4) - floor(myScreenSize(4)/10)*4 - 40, floor(myScreenSize(3)/10)*4, floor(myScreenSize(4)/10)*4];
end



switch varargin{1}
   
case -1,
   if ( ~isempty(hnd_this_figure) )
      delete( hnd_this_figure );
      position_this_figure = [];
      hnd_this_figure = [];
   end
   hnd_this_axes = [];
   call_count = 0;
   result = 0;
   
case 0,
   if ( ~isempty(hnd_this_figure) )
      position_this_figure = get( hnd_this_figure, 'Position' );
      delete( hnd_this_figure );
      settings_handler(7,settings_number,0);
      hnd_this_figure = [];
   end
   hnd_this_axes = [];
   call_count = 0;
   result = 0;
   
case {1,2,3}
    
    if (varargin{1} == 3)       % button callback
        if (varargin{2} == 1)
            hist_view = get(checkbox_handles(1), 'Value');
            redraw = 1;
        elseif (varargin{2} == 2)
            set(checkbox_handles([3, 4]), 'Value',0); set(checkbox_handles(2),'Value',1);
            plot_selection = 2;
            redraw = 1;
        elseif (varargin{2} == 3)
            set(checkbox_handles([2, 4]), 'Value',0); set(checkbox_handles(3),'Value',1);
            plot_selection = 3;
            redraw = 1;
        elseif (varargin{2} == 4)
            set(checkbox_handles([2, 3]), 'Value',0); set(checkbox_handles(4),'Value',1);
            plot_selection = 4;
            redraw = 1;
        elseif (varargin{2} == 5)
            set(checkbox_handles(6), 'Value',0); set(checkbox_handles(5),'Value',1);
            view_mode = 2;
            redraw = 1;
        elseif (varargin{2} == 6)
            set(checkbox_handles(5), 'Value',0); set(checkbox_handles(6),'Value',1);
            view_mode = 3;
            redraw = 1;
        end
        call_count = 0;
    end
    
    %do we have a figure to plot?
    if ( isempty(hnd_this_figure) )
        hnd_this_figure = figure('Tag', TAG_NAME, ...
            'Position', position_this_figure, ...
            'CloseRequestFcn', [FIGURE_PROCEDURE_NAME,'(0);'], ...
            'Name', 'Signal Constellations', ...
            'NumberTitle','off', ...
            'MenuBar','none', ...
            'Renderer', 'zbuffer');
        
      settings_handler(7,settings_number,1);
        redraw = 1;
        checkbox_handles = [];
    end
    if (isempty(checkbox_handles))
        checkbox_handles(1) = uicontrol('Parent',hnd_this_figure, ...
            'Units', 'normalized', 'BackgroundColor',[0.8 0.8 0.8], ...
            'Position',[0.75 0.35 0.03 0.05], 'Style','checkbox', ...
            'TooltipString','Plot signal histogram', ...
            'Value', double(hist_view == 1), 'Callback',[FIGURE_PROCEDURE_NAME,'(3,1);']);
        checkbox_handles(2) = uicontrol('Parent',hnd_this_figure, ...
            'Units', 'normalized', 'BackgroundColor',[0.8 0.8 0.8], ...
            'Position',[0.75 0.65 0.03 0.05], 'Style','radiobutton', ...
            'TooltipString','Plot FAC constellation', ...
            'Value', double(plot_selection == 2), 'Callback',[FIGURE_PROCEDURE_NAME,'(3,2);']);
        checkbox_handles(3) = uicontrol('Parent',hnd_this_figure, ...
            'Units','normalized', 'BackgroundColor',[0.8 0.8 0.8], ...
            'Position',[0.75 0.55 0.03 0.05], 'Style','radiobutton', ...
            'TooltipString','Plot SDC constellation', ...
            'Value', double(plot_selection == 3), 'Callback',[FIGURE_PROCEDURE_NAME,'(3,3);']);
        checkbox_handles(4) = uicontrol('Parent',hnd_this_figure, ...
            'Units','normalized', 'BackgroundColor',[0.8 0.8 0.8], ...
            'Position',[0.75 0.45 0.03 0.05], 'Style','radiobutton', ...
            'TooltipString','Plot MSC constellation', ...
            'Value', double(plot_selection == 4), 'Callback',[FIGURE_PROCEDURE_NAME,'(3,4);']);
        uicontrol('Parent',hnd_this_figure, ...
            'Units','normalized', 'BackgroundColor', [0.8 0.8 0.8], ...
            'HorizontalAlignment','left', 'Position', [0.785 0.343 0.3 0.05], ...
            'String','Histogram view', 'Style','text', ...
            'TooltipString','Plot signal histogram');
        uicontrol('Parent',hnd_this_figure, ...
            'Units','normalized', 'BackgroundColor',[0.8 0.8 0.8], ...
            'HorizontalAlignment','left', 'Position',[0.785 0.643 0.3 0.05], ...
            'String','FAC', 'Style','text', ...
            'TooltipString','Plot FAC constellation');
        uicontrol('Parent',hnd_this_figure, ...
            'Units','normalized', 'BackgroundColor',[0.8 0.8 0.8], ...
            'HorizontalAlignment','left', 'Position',[0.785 0.543 0.3 0.05], ...
            'String','SDC', 'Style','text', ...
            'TooltipString','Plot SDC constellation');
        uicontrol('Parent',hnd_this_figure, ...
            'Units','normalized', 'BackgroundColor',[0.8 0.8 0.8], ...
            'HorizontalAlignment','left', 'Position',[0.785 0.443 0.3 0.05], ...
            'String','MSC', 'Style','text', ...
            'TooltipString','Plot MSC constellation'); 
        
        if (hist_view == 0 && ~isempty(findobj('Tag','view_mode_radiobutton')))
            delete(findobj('Tag','view_mode_radiobutton'));
        end
    
    end
    
    
    if (hist_view == 1 && (length(checkbox_handles) < 6 | checkbox_handles(5) == 0))
        checkbox_handles(5) = uicontrol('Parent',hnd_this_figure, ...
            'Units','normalized', 'BackgroundColor',[0.8 0.8 0.8], ...
            'Position',[0.75 0.25 0.03 0.05], 'Style','radiobutton', ...
            'TooltipString','2D view of histogram', 'Tag', 'view_mode_radiobutton', ...
            'Value', double(view_mode == 2), 'Callback',[FIGURE_PROCEDURE_NAME,'(3,5);']);
        checkbox_handles(6) = uicontrol('Parent',hnd_this_figure, ...
            'Units','normalized', 'BackgroundColor',[0.8 0.8 0.8], ...
            'Position',[0.75 0.15 0.03 0.05], 'Style','radiobutton', ...
            'TooltipString','3D view of histogram', 'Tag', 'view_mode_radiobutton', ...
            'Value', double(view_mode == 3), 'Callback',[FIGURE_PROCEDURE_NAME,'(3,6);']);
        uicontrol('Parent',hnd_this_figure, ...
            'Units','normalized', 'BackgroundColor',[0.8 0.8 0.8], ...
            'HorizontalAlignment','left', 'Position',[0.785 0.243 0.3 0.05], ...
            'String','2D', 'Style','text', 'Tag', 'view_mode_radiobutton', ...
            'TooltipString','2D view of histogram');
        uicontrol('Parent',hnd_this_figure, ...
            'Units','normalized', 'BackgroundColor',[0.8 0.8 0.8], ...
            'HorizontalAlignment','left', 'Position',[0.785 0.143 0.3 0.05], ...
            'String','3D', 'Style','text', 'Tag', 'view_mode_radiobutton', ...
            'TooltipString','3D view of histogram');             
    end
    if ((hist_view == 0) && (length(checkbox_handles) > 4) && (checkbox_handles(5) ~= 0))
        delete(findobj('Tag','view_mode_radiobutton'));
        checkbox_handles([5,6])=[0,0];
    end
    
    result = hnd_this_figure;
    %------------dont change end ------------
   
   hnd_nodatatext = findobj('Tag', [TAG_NAME,'_nodatatext']);
   
   if (~data_available(plot_selection))
       if (~isempty(hnd_this_axes))
           delete(hnd_this_axes);
           hnd_this_axes = [];
       end 
       
       if (isempty(hnd_nodatatext))
           uicontrol('Parent', hnd_this_figure, ...
               'BackgroundColor',[0.8 0.8 0.8], ...
               'Units', 'normalized', ...
               'FontUnits', 'points', ...
               'Position',[0.1 0.5 0.6 0.1], ...
               'String','no data available', ...
               'Style','text', ...
               'Tag', [TAG_NAME,'_nodatatext'], ...
               'FontSize', 12 ...
               );
       end
       is_in_use = [];
       redraw = 1;
       return;
   end
   
   call_count = rem(call_count + 1, plot_period(plot_selection));
   if (call_count ~= 1 && hist_view == 1)
       is_in_use = [];
       return;
   end
   
   if (~isempty(hnd_nodatatext))
       delete(hnd_nodatatext);
   end
   
   if (isempty(hnd_this_axes))
       
       set(0,'CurrentFigure',hnd_this_figure);
       hnd_this_axes = subplot('position',[0.1 0.1 0.61 0.815]);
       redraw = 1;
       
   end
   
   if ( redraw ) %do we have the sub-handles to directly refresh data?

       set(0,'CurrentFigure',hnd_this_figure);
       subplot(hnd_this_axes);
       cla reset;
   
       if (hist_view == 0)
           set(hnd_this_figure, 'Renderer', 'zbuffer');
           if (plot_selection == 1)
           elseif(plot_selection == 2)
               grid_type = 2;
               line_handles(plot_selection) = plot(real_data{plot_selection}, imag_data{plot_selection},'.', 'MarkerSize',6, 'Color', [0 0 1]);
               title('FAC signal');
           elseif(plot_selection == 3)
               if (sdc_mode) grid_type = 2; else grid_type = 3; end
               line_handles(plot_selection) = plot(real_data{plot_selection}, imag_data{plot_selection},'.', 'MarkerSize',6, 'Color', [0 0 1]);
               title('SDC signal');
           else
               if (msc_mode == 3) grid_type = 3; else grid_type = 4; end
               line_handles(plot_selection) = plot(real_data{plot_selection}, imag_data{plot_selection},'.', 'MarkerSize',4, 'Color', [0 0 1]);
               title('MSC signal');
           end
           hold on;
           % plot QAM points
           if (grid_type == 1)
               plot((1/sqrt(2)) * (ones(2,1) * [1, -1] + [1, -1]' * j * ones(1,2)),'+','Color',[1 1 0]);
               plot((1/sqrt(10)) * (ones(4,1) * [3, 1, -1, -3] + [3, 1, -1, -3]' * j * ones(1,4)),'+','Color',[1 0 1]);
               plot((1/sqrt(42)) * (ones(8,1) * [7, 5, 3, 1, -1, -3, -5, -7] + [7, 5, 3, 1, -1, -3, -5, -7]' * j * ones(1,8)),'+','Color',[0 1 1]);
           elseif (grid_type == 2)
               plot((1/sqrt(2)) * (ones(2,1) * [1, -1] + [1, -1]' * j * ones(1,2)),'+','Color',[1 0.5 0.5]);
           elseif (grid_type == 3)
               plot((1/sqrt(10)) * (ones(4,1) * [3, 1, -1, -3] + [3, 1, -1, -3]' * j * ones(1,4)),'+','Color',[1 0.5 0.5]);
           else
               plot((1/sqrt(42)) * (ones(8,1) * [7, 5, 3, 1, -1, -3, -5, -7] + [7, 5, 3, 1, -1, -3, -5, -7]' * j * ones(1,8)),'+','Color',[1 0.5 0.5]);
           end
          
       else
           set(hnd_this_figure, 'Renderer', 'OpenGL');
           set(gca,'Color', [0.8 0.8 0.8]);
           if(plot_selection == 2)
               grid_type = 2; view(view4QAM(view_mode,:));
               axes_range = [-max_abs_val + max_abs_val/res_FAC:2*max_abs_val/res_FAC:max_abs_val - max_abs_val/res_FAC];
               hist_to_plot = FAC_hist + FAC_hists{current_FAC_hist};
               title('FAC signal histogram');
           elseif(plot_selection == 3)
               if (sdc_mode) grid_type = 2; view(view4QAM(view_mode,:)); else grid_type = 3; view(view16QAM(view_mode,:)); end
               axes_range = [-max_abs_val + max_abs_val/res_SDC:2*max_abs_val/res_SDC:max_abs_val - max_abs_val/res_SDC];
               hist_to_plot = SDC_hist + SDC_hists{current_SDC_hist};
               title('SDC signal histogram');
           else
               if (msc_mode == 3) grid_type = 3; view(view16QAM(view_mode,:)); else grid_type = 4; view(view64QAM(view_mode,:)); end
               axes_range = [-max_abs_val + max_abs_val/res_MSC:2*max_abs_val/res_MSC:max_abs_val - max_abs_val/res_MSC];
               hist_to_plot = MSC_hist + MSC_hists{current_MSC_hist};
               title('MSC signal histogram');
           end
           hist_to_plot = hist_to_plot * (1 / sum(hist_to_plot(:)));
           line_handles(plot_selection) = surface(axes_range, axes_range, hist_to_plot);
           shading interp;
           colormap jet;
           hold on;
           zlabel('rel. frequency');
       end
       
       % axes settings:
       xlim([-max_abs_val max_abs_val]);
       ylim([-max_abs_val max_abs_val]); 
       xlabel('real part');
       ylabel('imaginary part');
       
       % plot grid:
       if (grid_type == 1)
       elseif (grid_type == 2)
           line([0 0], [-max_abs_val max_abs_val], 'Color',[0.7 0.7 0.7],'LineStyle','--');
           line([-max_abs_val max_abs_val],[0 0],'Color',[0.7 0.7 0.7],'LineStyle','--');
       elseif (grid_type == 3)
           line((1/sqrt(10))*[-2, 0, 2;-2, 0, 2],max_abs_val * [-ones(1,3);ones(1,3)],'Color',[0.7 0.7 0.7],'LineStyle','--');
           line(max_abs_val * [-ones(1,3);ones(1,3)],(1/sqrt(10))*[-2, 0, 2;-2, 0, 2],'Color',[0.7 0.7 0.7],'LineStyle','--');
       else
           line((1/sqrt(42))*[-6, -4, -2, 0, 2, 4, 6; -6, -4, -2, 0, 2, 4, 6],max_abs_val * [-ones(1,7);ones(1,7)],'Color',[0.7 0.7 0.7],'LineStyle','--');
           line(max_abs_val * [-ones(1,7);ones(1,7)],(1/sqrt(42))*[-6, -4, -2, 0, 2, 4, 6; -6, -4, -2, 0, 2, 4, 6],'Color',[0.7 0.7 0.7],'LineStyle','--');
       end
       hold off;
       redraw = 0;	%next time we can directly write do the data vector of the figure
       
   else     % plot is ok, just update data
       
       if (hist_view == 0)
           set (line_handles(plot_selection), 'XData', real_data{plot_selection}, 'YData', imag_data{plot_selection});
       else
           if (plot_selection == 1)
           elseif(plot_selection == 2)
               hist_to_plot = FAC_hist + FAC_hists{current_FAC_hist};
           elseif(plot_selection == 3)
               hist_to_plot = SDC_hist + SDC_hists{current_SDC_hist};
           else
               hist_to_plot = MSC_hist + MSC_hists{current_MSC_hist};
           end
           hist_to_plot = hist_to_plot * (1 / sum(hist_to_plot(:)));
           set (line_handles(plot_selection), 'ZData', hist_to_plot);
           set (line_handles(plot_selection), 'CData', hist_to_plot);
       end
   end
    
   
case 4     % FAC cells
    if (~data_available(2))
        FAC_hists = cell(1,merged_frames);
        current_FAC_hist = 1;
        FAC_hist = zeros(res_FAC);
        frame_count(2) = 0;   
        for (i=1:merged_frames)
            FAC_hists{i} = zeros(res_FAC);
        end
    end
    frame_count(2) = frame_count(2) + 1;
    if (frame_count(2) > frames_to_merge)
        frame_count(2) = 1;
        FAC_hist = FAC_hist + FAC_hists{current_FAC_hist};      
        current_FAC_hist = rem(current_FAC_hist, merged_frames) + 1;
        FAC_hist = FAC_hist - FAC_hists{current_FAC_hist};
        FAC_hists{current_FAC_hist} = zeros(res_FAC);       
    end
    real_data{2} = real(varargin{2});
    imag_data{2} = imag(varargin{2});
    FAC_hists{current_FAC_hist} = FAC_hists{current_FAC_hist} + hist2D_equidist([real_data{2},imag_data{2}],max_abs_val * [-1 -1;1 1],[res_FAC res_FAC]);
    data_available(2) = 1;
    
case 5     % SDC cells

    if (isempty(sdc_mode) || ~isequal(varargin{3},sdc_mode))
        redraw = 1;
        data_available(3) = 0;
    end
    if (~data_available(3))
        SDC_hists = cell(1,merged_frames);
        current_SDC_hist = 1;
        SDC_hist = zeros(res_SDC);
        frame_count(3) = 0;   
        for (i=1:merged_frames)
            SDC_hists{i} = zeros(res_SDC);
        end
    end
    frame_count(3) = frame_count(3) + 1;
    if (frame_count(3) > frames_to_merge)
        frame_count(3) = 1;
        SDC_hist = SDC_hist + SDC_hists{current_SDC_hist};
        current_SDC_hist = rem(current_SDC_hist, merged_frames) + 1;
        SDC_hist = SDC_hist - SDC_hists{current_SDC_hist};
        SDC_hists{current_SDC_hist} = zeros(res_SDC);
    end
    real_data{3} = real(varargin{2});
    imag_data{3} = imag(varargin{2});
    SDC_hists{current_SDC_hist} = SDC_hists{current_SDC_hist} + hist2D_equidist([real_data{3},imag_data{3}],max_abs_val * [-1 -1;1 1],[res_SDC res_SDC]);
    sdc_mode = varargin{3};
    data_available(3) = 1;
    
case 6     % MSC cells
    if (isempty(msc_mode) || ~isequal(varargin{3},msc_mode))
        redraw = 1;
        data_available(4) = 0;
    end
    if (~data_available(4))
        MSC_hists = cell(1,merged_frames);
        current_MSC_hist = 1;
        MSC_hist = zeros(res_MSC);
        frame_count(4) = 0;   
        for (i=1:merged_frames)
            MSC_hists{i} = zeros(res_MSC);
        end
    end
    frame_count(4) = frame_count(4) + 1;
    if (frame_count(4) > frames_to_merge)
        frame_count(4) = 1;
        MSC_hist = MSC_hist + MSC_hists{current_MSC_hist};
        current_MSC_hist = rem(current_MSC_hist, merged_frames) + 1;
        MSC_hist = MSC_hist - MSC_hists{current_MSC_hist};
        MSC_hists{current_MSC_hist} = zeros(res_MSC);
    end
    real_data{4} = real(varargin{2});
    imag_data{4} = imag(varargin{2});
    MSC_hists{current_MSC_hist} = MSC_hists{current_MSC_hist} + hist2D_equidist([real_data{4},imag_data{4}],max_abs_val * [-1 -1;1 1],[res_MSC res_MSC]);
    msc_mode = varargin{3};
    data_available(4) = 1;
    
%   test_length=10000;sigma=0.05;constellation=(1/sqrt(42))*(ones(8,1)*[7,5,3,1,-1,-3,-5,-7]+[7,5,3,1,-1,-3,-5,-7]'*j*ones(1,8));test_signal=constellation(ceil(64*rand(1,test_length)))+sigma*randn(1,test_length)+ j*sigma*randn(1,test_length);
%   hist_size=100;zreal=real(test_signal);zimag=imag(test_signal);zreal_round=floor((zreal+max_abs_val)/(2*max_abs_val)*hist_size);hist2D=reshape(hist(zimag + zreal_round * (2*max_abs_val),[-max_abs_val:(2*max_abs_val)/hist_size:(hist_size-1)*(2*max_abs_val)+max_abs_val-(2*max_abs_val)/hist_size])',hist_size,hist_size);surface(hist2D);
%   tic;hist_size=100;zround=floor((test_signal+max_abs_val+max_abs_val*j)/(2*max_abs_val)*hist_size)+1+j;zreal=real(zround);zimag=imag(zround);hist2D=zeros(hist_size,hist_size);for i=find(zreal>=1 & zimag>=1 & zreal<=hist_size & zimag<=hist_size), hist2D(zimag(i),zreal(i)) = hist2D (zimag(i),zreal(i)) + 1; end,toc
   

case 7      % reset figure  
    
    data_available = zeros(1,4);   
    
   
end	%if switch


is_in_use = [];


   
	   
   

