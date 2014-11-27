%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  University of Kaiserslautern, Institute of Communications Engineering     %
%%  Copyright (C) 2005 Torsten Schorr                                         %
%%                                                                            %
%%  Author(s)    : Torsten Schorr (schorr@eit.uni-kl.de)                      %
%%  Project start: 01.11.2004                                                 %
%%  Last change  : 25.01.2005                                                 %
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
%%  wavio_linux.mexglx                                                        %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  Description:                                                              %
%%  Sound recording and playback for Open Sound System (OSS) and              %
%%  Advanced Linux Sound Architecture (ALSA)                                  %
%%  Usage:                                                                    %
%%                                                                            %
%%  samples = wavio_linux(0, N, Fs [, ch [, r]]) for recording N samples at   %
%%  sampling frequency Fs from ch channels (default: 1 = mono).               %
%%  Using sample rate conversion, applying the factor r results in            %
%%  conversion of N*r soundcard samples into N output samples.                %
%%                                                                            %
%%  wavio_linux(1, samples, Fs [, r]) for playback of samples at              %
%%  sampling frequency Fs. Using sample rate conversion, applying the         %
%%  factor r results in conversion of N input samples into N*r                %
%%  soundcard samples.                                                        %
%%                                                                            %                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function varargout = wavio_linux(varargin)

