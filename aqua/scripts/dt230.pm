#!/usr/bin/perl
#*******************************************************************************************
# dt230.pm
# Copyright (c) 2010  by Fraunhofer IIS NUE/ANT2, Erlangen
#
#*******************************************************************************************
# This program is free software; you can redistribute it and/or modify it under the terms
# of the GNU General Public License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
# without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with this
# program; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth
# Floor, Boston, MA 02110, USA
#
#*******************************************************************************************
#  $Id: dt230.pm,v 1.15 2010-02-17 09:43:06 kln Exp $
#*******************************************************************************************
#
# Description: constants for parameters of the drm test equipment dt230
#
#*******************************************************************************************

# ----------------------------------------------------------------------------
# definition of package
package Dt230;

# ----------------------------------------------------------------------------
# includes
use strict;
use warnings;

# ----------------------------------------------------------------------------
# on/off
use constant OFF => 0;
use constant ON  => 1;

# ----------------------------------------------------------------------------
# time
use constant WAIT_DEFAULT     => 0;
use constant WAIT_MINIMUM     => 1;
use constant WAIT_BEFORE_RF   => 2;
use constant WAIT_BEFORE_MDI  => 3;
use constant WAIT_MEASURE     => 4;
use constant WAIT_MEASURELONG => 5;
use constant WAIT_PERFORMANCE => 6;

# ----------------------------------------------------------------------------
# source
use constant SOURCE_FILE     => 0;
use constant SOURCE_NETWORK  => 1;
use constant SOURCE_AD       => 2;

# ----------------------------------------------------------------------------
# media
use constant MEDIA_HD    => 0;
use constant MEDIA_HDDRM => 0;
use constant MEDIA_HDUSR => 1;
use constant MEDIA_DVD   => 2;
use constant MEDIA_USB   => 3;

# ----------------------------------------------------------------------------
# interferer
use constant INTERFERER_OFF   => 0;
use constant INTERFERER_SINUS => 1;
use constant INTERFERER_USER  => 2;

# ----------------------------------------------------------------------------
# power measurement
use constant POWER_MANUAL => 0;
use constant POWER_INPUT  => 1;

# ----------------------------------------------------------------------------
# sample rate (playing)
use constant PLAY_SR_96 => 1;
use constant PLAY_SR_48 => 2;
use constant PLAY_SR_24 => 4;
use constant PLAY_SR_12 => 8;

# ----------------------------------------------------------------------------
# sample rate (recording)
use constant REC_SR_96 => -1;
use constant REC_SR_48 => -2;
use constant REC_SR_24 => -4;
use constant REC_SR_12 => -8;

# ----------------------------------------------------------------------------
# rsci
use constant RSCI_INPUT_OFF       => 0;
use constant RSCI_INPUT_MULTICAST => 1;
use constant RSCI_INPUT_NETWORK   => 2;
use constant RSCI_INPUT_RS232     => 3;

# ----------------------------------------------------------------------------
# dynamic profile
use constant MODE_STATIC  => 0;
use constant MODE_DYNAMIC => 1;

# ----------------------------------------------------------------------------
return 1;
