%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  University of Kaiserslautern, Institute of Communications Engineering     %
%%  Copyright (C) 2005 Torsten Schorr                                         %
%%                                                                            %
%%  Author(s)    : Torsten Schorr (schorr@eit.uni-kl.de)                      %
%%  Project start: 09.05.2005                                                 %
%%  Last change  : 09.05.2005                                                 %
%%  Changes      :                                                           %
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
%%  settings_handler.m                                                        %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  Description:                                                              %
%%  handling of GUI, plot variables, etc.                                     %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function result = settings_handler( varargin )

% first argument:

% 1     GUI callback
% 2     import all GUI values from GUI
% 3     import all GUI values from cell array as second argument
% 4     get single GUI value
% 5     import all non-GUI values from cell array as second argument
% 6     get single non-GUI value
% 7     put single GUI value
% 8     disable GUI
% 9     get GUI settings
% 10    get non-GUI settings

% second argument: 

% GUI settings:
% 1     PLOT_INPUT_SPECTRUM
% 2     PLOT_SYNCHRONISATION
% 3     PLOT_CONSTELLATIONS
% 4     PLOT_CHANNELESTIMATION
% 5     PLOT_SNR_SPECTRUM
% 6     FLIP_SPECTRUM
% 7     WRITEWAV
% 8     PLAYWAV
% 9     ENABLE_SERVICE1
% 10    ENABLE_SERVICE2
% 11    ENABLE_SERVICE3
% 12    ENABLE_SERVICE4
% 13    input_source
% 14    VERBOSE_LEVEL
% 15    MSD_ITER
% 16    EQUALIZATION
% 17    CHANNELDECODING
% 18    SOURCEDECODING
% 19    RECORD_INPUT
% 20    SHOW_SIGNAL_INFO
% 21    STORE_NEWS
% 22    EXIT
% 23    STOP
% 24    SHOW_ABOUT
% 25    browse input file
% 26    browse data storage path
% 27    input_filename
% 28    DATA_STORAGE_PATH
% 29    input_file_progress
% 30    AUDIO_SERVICE
% 31    audio_services

% non-GUI settings:
% 1     SNR_MAX_DB
% 2     SAMPLERATE_OFFSET_INIT_PPM
% 3     SOFT_MSD
% 4     ENABLE_GUI
% 5     UTILS_PATH
% 6     JINGLE_BEGIN_FILENAME
% 7     JINGLE_END_FILENAME
% 8     CONVENIENCE_NOISE_FILENAME
% 9     DEBUG
% 10    PRINTTIME
% 11    offset_smp


%*******************************************************************************
%* constant global variables                                                   *
%*******************************************************************************
INPUT_SOURCE_SOUNDCARD=0; 
INPUT_SOURCE_FILE=1;

RUN_COMMAND_NONE = 0; 
RUN_COMMAND_RESTART = -1; 
RUN_COMMAND_QUIT = 99; 

RUN_STATE_POWERON = -2; 
RUN_STATE_INIT = -1; 
RUN_STATE_FIRSTRUN = 0; 
RUN_STATE_NORMAL = 1; 
RUN_STATE_LASTRUN = 99;
RUN_STATE_STOP = 100;

%*******************************************************************************
%* constant global variables                                                   *
%*******************************************************************************
INPUT_SOURCE_SOUNDCARD=1; 
INPUT_SOURCE_FILE=2;

RUN_COMMAND_NONE = 0; 
RUN_COMMAND_RESTART = -1; 
RUN_COMMAND_QUIT = 99; 

RUN_STATE_POWERON = -2; 
RUN_STATE_INIT = -1; 
RUN_STATE_FIRSTRUN = 0; 
RUN_STATE_NORMAL = 1; 
RUN_STATE_LASTRUN = 99;
RUN_STATE_STOP = 100;

%*******************************************************************************
%* global variables                                                            *
%*******************************************************************************
global run_command;	% 0 = none, -1 = restart, 99 = quit
global run_state;

%*******************************************************************************
%* static variables                                                            *
%*******************************************************************************
persistent gui_variable_values gui_element_handles gui_element_propertynames ...
           nongui_variable_values gui_found



num_of_gui_variables = 31;
num_of_nongui_variables = 11;
result = 0;

if (isempty(findobj('Tag','radio_gui')))
    
    gui_found = [];
    
    
end


if (isempty(gui_variable_values))
    
    gui_element_handles = cell(1,num_of_gui_variables);
    
    gui_element_propertynames = cell(1,num_of_gui_variables);
    [gui_element_propertynames{[1:26,29:30]}] = deal('Value');
    [gui_element_propertynames{[27:28,31]}] = deal('String');
    
    gui_variable_values = cell(1,num_of_gui_variables);
    nongui_variable_values = cell(1,num_of_nongui_variables);
    [gui_variable_values{:}] = deal([]);
    [nongui_variable_values{:}] = deal([]);
    
end

if (isempty(gui_found) &  ~isempty(findobj('Tag','radio_gui')))
  
    gui_element_handles{1} = findobj('Tag','input_spectrum_checkbox');
    gui_element_handles{2} = findobj('Tag','synchronisation_checkbox');
    gui_element_handles{3} = findobj('Tag','constellations_checkbox');
    gui_element_handles{4} = findobj('Tag','channelestimation_checkbox');
    gui_element_handles{5} = findobj('Tag','snr_spectrum_checkbox');
    gui_element_handles{6} = findobj('Tag','flip_spectrum_checkbox');
    gui_element_handles{7} = findobj('Tag','writewav_checkbox');
    gui_element_handles{8} = findobj('Tag','playwav_checkbox');
    gui_element_handles{9} = findobj('Tag','service1_checkbox');
    gui_element_handles{10} = findobj('Tag','service2_checkbox');
    gui_element_handles{11} = findobj('Tag','service3_checkbox');
    gui_element_handles{12} = findobj('Tag','service4_checkbox');
    gui_element_handles{13} = findobj('Tag','input_file_checkbox');
    gui_element_handles{14} = findobj('Tag','verbose_level_popupmenu');
    gui_element_handles{15} = findobj('Tag','msd_iter_popupmenu');
    gui_element_handles{16} = findobj('Tag','equalization_checkbox');
    gui_element_handles{17} = findobj('Tag','channeldecoding_checkbox');
    gui_element_handles{18} = findobj('Tag','sourcedecoding_checkbox');
    gui_element_handles{19} = findobj('Tag','record_input_checkbox');
    gui_element_handles{20} = findobj('Tag','signal_info_checkbox');
    gui_element_handles{21} = findobj('Tag','store_news_checkbox');
    gui_element_handles{22} = findobj('Tag','exit_pushbutton');
    gui_element_handles{23} = findobj('Tag','stop_pushbutton');
    gui_element_handles{24} = findobj('Tag','about_pushbutton');
    gui_element_handles{25} = findobj('Tag','input_file_pushbutton');
    gui_element_handles{26} = findobj('Tag','data_storage_path_pushbutton');
    gui_element_handles{27} = findobj('Tag','input_file_edittext');
    gui_element_handles{28} = findobj('Tag','data_storage_path_edittext');
    gui_element_handles{29} = findobj('Tag','input_file_progress_slider');
    gui_element_handles{30} = findobj('Tag','audio_service_popupmenu');
    gui_element_handles{31} = findobj('Tag','audio_service_popupmenu');

    gui_found = 1;
    
end

switch varargin{1}
   
case 1          % GUI call
    
        event_origin = varargin{2};
        event_handle = varargin{3};
        event_value  = varargin{4};
    
        switch event_origin
        
        case 0 % gui close request
            ENABLE_GUI = 0;
            delete (findobj('Tag','radio_gui'));
            close (findobj('Type','figure'));
    
        case 1 % input_spectrum_checkbox
            if (plot_input_spectrum( event_value == 1 ) >= 0)
                gui_variable_values{1} = event_value;
            end
            
        case 2 % synchronisation_checkbox
            if (plot_synchronisation( event_value == 1 ))
                gui_variable_values{2} = event_value;
            end
            
        case 3 % constellations_checkbox
            if (plot_constellations( event_value == 1 ))
                gui_variable_values{3} = event_value;
            end 
            
        case 4 % channelestimation_checkbox
            if (plot_channelestimation( event_value == 1 ))
                gui_variable_values{4} = event_value;
            end
            
        case 5 % snr_spectrum_checkbox
            if (plot_snr_spectrum( event_value == 1 ))
                gui_variable_values{5} = event_value;
            end   
            
        case 6 % flip_spectrum_checkbox
            gui_variable_values{6} = event_value;
            
        case 7 % writewav_checkbox
            gui_variable_values{7} = event_value;
            
        case 8 % playwav_checkbox
            gui_variable_values{8} = event_value;
            
        case 9 % service1_checkbox
            gui_variable_values{9} = event_value;
            
        case 10 % service2_checkbox
            gui_variable_values{10} = event_value;
            
        case 11 % service3_checkbox
            gui_variable_values{11} = event_value;
            
        case 12 % service4_checkbox
            gui_variable_values{12} = event_value;    
            
        case 13 % input_file_checkbox
            new_input_filename = get(findobj('Tag','input_file_edittext'),'String');
            if (~exist('input_filename','var'))
                input_filename = '';
            end
                        
            if (isequal(exist(new_input_filename),2) & isequal(event_value,1))
                if (~isequal(new_input_filename, input_filename) | gui_variable_values{13} ~=1)
                    run_command = RUN_COMMAND_RESTART;
                end
                run_command = RUN_COMMAND_RESTART;
                gui_variable_values{13} = 1;
            else
                if (gui_variable_values{13} ~=0)
                    run_command = RUN_COMMAND_RESTART;
                end
                gui_variable_values{13} = 0;
            end
            if (isequal(exist(new_input_filename),2))
                new_input_filename = input_filename;
            end  
            
        case 14 % verbose_level listbox
            gui_variable_values{14} = event_value;
            
        case 15 % msd_iter_listbox
            gui_variable_values{15} = event_value;
            
        case 16 % equalization_checkbox
            gui_variable_values{16} = event_value;
            
        case 17 % channeldecoding_checkbox
            gui_variable_values{17} = event_value;
            
        case 18 % sourcedecoding_checkbox
            gui_variable_values{18} = event_value;
            
        case 19 % record_input_checkbox
            gui_variable_values{19} = event_value;  
            
        case 20 % show_signal_info
            if (show_signal_info( event_value == 1 ) >= 0)
                gui_variable_values{20} = event_value;
            end
            
        case 21 % store_news_checkbox
            gui_variable_values{21} = event_value;
            
        case 22 % exit_pushbutton
            gui_variable_values{22} = 1;
            run_command = 100;
            nongui_variable_values{4} = 0;
            delete (findobj('Tag','radio_gui'));
            close (findobj('Type','figure'));              
            %close (findobj('Tag','radio_gui'));
            
        case 23 % stop_pushbutton
            gui_variable_values{23} = 1;
                set(gui_element_handles{23},'String','Restart');  
                set(gui_element_handles{23},'Callback','diorama');
                run_command = 100;
                
        case 24
            if (about( event_value == 1 ) >= 0)
                gui_variable_values{24} = event_value;
            end

            
        case 25 % input_file_pushbutton   
            
            input_filename = gui_variable_values{27};
            
            last_input_path = fileparts(input_filename);
            save_pwd = pwd;
            if (isequal(exist(last_input_path),7))
                cd (last_input_path);
            end      
            
            % [new_input_filename, input_path_name] = uigetfile({'*.wav','WAVE files (*.wav)';'*.*','All files (*.*)'},'Select input file');
            [new_input_filename, input_path_name] = uigetfile('*.wav','Select input file');
            cd (save_pwd);
            if (~isequal(new_input_filename,0))
                new_input_filename = fullfile(input_path_name, new_input_filename);
                gui_variable_values{27} = new_input_filename;
                set(gui_element_handles{27}, gui_element_propertynames{27}, new_input_filename);
                gui_variable_values{13} = 1;
                set(gui_element_handles{13}, gui_element_propertynames{13}, gui_variable_values{13});
                run_command = RUN_COMMAND_RESTART;
                if (run_state >= 99)
                    set (gui_element_handles{25},'enable','on');
                    diorama;
                end
            end
            
        case 26 % data_storage_path_pushbutton
            
            DATA_STORAGE_PATH = gui_variable_values{28};
            
            [last_data_storage_path, path_tail]  = fileparts(DATA_STORAGE_PATH);
            if (~isequal(exist(last_data_storage_path),7))
                last_data_storage_path = pwd;
            end               
            if (isequal(exist(DATA_STORAGE_PATH),7))
                last_data_storage_path = fullfile(last_data_storage_path, path_tail);
            end

%             if (isequal(exist('uigetdir.m'),2) | isequal(exist('uigetdir'),3) | isequal(exist('uigetdir'),5))
%                 new_data_storage_path = uigetdir(last_data_storage_path,'Select path to store decoded data');
%             else
                save_pwd = pwd;
                cd (last_data_storage_path);
                [dummy, new_data_storage_path] = uiputfile('_','Select path to store decoded data');
                cd (save_pwd);
%             end
            if (~isequal(new_data_storage_path,0))
                
                gui_variable_values{28} = new_data_storage_path;
                set(gui_element_handles{28}, gui_element_propertynames{28}, new_data_storage_path);

            end  
            
        case 27 % input_file_edittext
       
            input_filename = gui_variable_values{27};            
            new_input_filename = get(gui_element_handles{27}, gui_element_propertynames{27});
                           
            if (isequal(exist(new_input_filename),2))
                
                gui_variable_values{27} = new_input_filename;
                gui_variable_values{13} = 1;
                
                run_command = RUN_COMMAND_RESTART;               
                
            else
                gui_variable_values{13} = 0;
                run_command = RUN_COMMAND_RESTART;
            end

            set(gui_element_handles{13}, gui_element_propertynames{13}, gui_variable_values{13});
            
            
        case 28 % data_storage_path_edittext
            new_data_storage_path = get(gui_element_handles{28}, gui_element_propertynames{28});
            gui_variable_values{28} = new_data_storage_path;
        
        case 29 % input_file_progress_slider
            gui_variable_values{29} = event_value;
            
        case 30 % audio_service_popupmenu
            gui_variable_values{30} = event_value;  

        case 31 % audio_service_popupmenu: string change (never happens)
            gui_variable_values{31} = event_value;  
            
         
            
    end 

case 2      % take values from GUI, if present
    
    if (~isempty(gui_found))
        
        for i = 1:num_of_gui_variables
            
            gui_variable_values{i} = get(gui_element_handles{i}, gui_element_propertynames{i});
            
        end        
        
    end
    
case 3      % load settings
    
    settings_array = varargin{2};
    
    for i = 1:num_of_gui_variables
        
        gui_variable_values{i} = settings_array{i};
        
    end
    
    if (~isempty(gui_found))
        
        for i = 1:num_of_gui_variables
            
            set(gui_element_handles{i}, gui_element_propertynames{i}, settings_array{i});
            
        end
    end
    
    
case 4      % get single values
    
    result = gui_variable_values{varargin{2}};
    
case 5      % load settings
    
    settings_array = varargin{2};
    
    
    for i = 1:num_of_nongui_variables
        
        nongui_variable_values{i} = settings_array{i};
        
    end
    
    
case 6      % get single values
    
    result = nongui_variable_values{varargin{2}};
    
case 7      % set single values
    
    if (~isempty(gui_found))
        set(gui_element_handles{varargin{2}}, gui_element_propertynames{varargin{2}}, varargin{3});
    end
    gui_variable_values{varargin{2}} = varargin{3};    
    
case 8
    
    delete (findobj('Tag','radio_gui'));
    close (findobj('Type','figure'));
    gui_found = [];
    nongui_variable_values{4} = 0;
    
case 9      % get GUI settings
        
    result = gui_variable_values;
    
case 10      % get non-GUI settings

    result = nongui_variable_values;
    
    
end
