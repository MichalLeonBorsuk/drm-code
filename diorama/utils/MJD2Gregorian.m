%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                            %
%%  University of Kaiserslautern, Institute of Communications Engineering     %
%%  Copyright (C) 2005 Torsten Schorr                                         %
%%                                                                            %
%%  Author(s)    : Torsten Schorr (schorr@eit.uni-kl.de)                      %
%%  Project start: 20.01.2005                                                 %
%%  Last change  : 20.01.2005                                                 %
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
%%  MJD2Gregorian.m                                                           %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  Description:                                                              %
%%  Conversion of Modified Julian Date to Gregorian date                      % 
%%                                                                            %
%%  Usage:                                                                    %
%%                                                                            %
%%  [day,month, year] = MJD2Gregorian(MJD);                                   %
%%                                                                            %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function [day, month, year] = MJD2Gregorian(MJD)

JD = MJD + 2400001;                     % Julian Date

% Fliegel and van Flandern (1968)
L = JD + 68569;
N = floor(4 * L/146097);
L = L - floor(( 146097 * N + 3 ) / 4);
I = floor((4000 * ( L + 1 )) / 1461001);
L = L - floor(1461 * I/4) + 31;
J = floor(80 * L/2447);
day = L - floor(2447 * J/80);
L = floor(J/11);
month = J + 2 - 12 * L;
year = 100 * ( N - 49 ) + I + L;


% straight forward:
%
% JD0 = 1721426;                          % Julian Date of Jan. 01. 0001 
% MC = [-1,0,-2,-1,-1,0,0,1,2,2,3,3];     % month correction
% 
% N400 = floor((JD - JD0)/146097);        % full 400 years cycles since Jan. 01. 0001
% R400 = rem((JD - JD0),146097);
% N100 = floor(R400/36524);               % remaining full 100 years cycles
% R100 = rem(R400,36524);
% if (N100==4) N100=3; R100=36524; end    % correction at the end of a 400 years cylce    
% N4 = floor(R100/1461);                  % remaining full 4 years cycles
% R4 = rem(R100,1461);
% N1 = floor(R4/365);                     % remaining full years
% RD = rem(R4,365);                       % running day
% if (N1==4) N1=3; LT=365; end            % correction at the end of a 4 years cylce
% RY = 400*N400 + 100*N100 + 4*N4 + N1;   % running year
% 
% year = RY + 1;
% month = floor((RD+1)/30) + 1;
% if (month > 12) month = month - 1; end
% LYC = (N1 == 3) & (month > 2);          % leap year correction;
% day = RD - 30*(month-1) - (LYC + MC(month));
% if (day < 1) month = month - 1; end
% LYC = (N1 == 3) & (month > 2);          % leap year correction;
% day = RD - 30*(month-1) - (LYC + MC(month));
