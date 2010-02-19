#!/usr/bin/perl
#*******************************************************************************************
# FadingChannelTest.pl
# based partly on dt230.pl which is Copyright (c) 2010  by Fraunhofer IIS NUE/ANT2, Erlangen
# Copyright (c) 2010 by BBC R&D, London
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
#  $Id: dt230.pl,v 1.32 2006/08/29 15:27:50 lgr Exp $
#*******************************************************************************************
#
#
# Description: perl script for conducting DRM receiver tests in fading channels, using DT230 as signal source.
#              => change hostname to your setting
#
#*******************************************************************************************

use dthttp;
use dt230;

# ----------------------------------------------------------------------------
# communication settings
my $pHttpHandle;

my $HostName;

if(defined $ARGV[0]) {
    $HostName = $ARGV[0];
} else {
    print STDERR "Usage: $0 [Hostname] [RxName] [TestNum] \nHostname not set.\n";
    exit 1;
}

if(defined $ARGV[1]) {
    $RxName = $ARGV[1];
} else {
    print STDERR "Usage: $0 [Hostname] [RxName] [TestNum] \nRxName not set.\n";
    exit 1;
}

if(defined $ARGV[2]) {
    $TestNum = $ARGV[2];
} else {
    print STDERR "Usage: $0 [Hostname] [RxName] [TestNum] \nTestNum not set.\n";
    exit 1;
}



eval {
    
# hostname
$pHttpHandle = DtHttp::RegisterHandle($HostName);

# set lock
DtHttp::SetLock($pHttpHandle, 1);

# set to stopped state first
DtHttp::SetParameter($pHttpHandle, 'Stop', 1);

$SNRRange = 2;

%IQFileNames = (
  1 => 'A 3 9 K.iq48',
  2 => 'A 2 9 K.iq48',
  3 => 'B 2 10 K.iq48',
  4 => 'B 1 10 K.iq48',
  5 => 'C 1 10 K.iq48'
);

%TestDurationsMinutes = (1 => 3, 2=>40, 3=>67, 4=>67, 5=>20);
%TargetSNRs = (1=>21, 2=>22, 3=>31, 4=>26, 5=>20);
%Bandwidths = (1=>9, 2=>9, 3=>10, 4=>10, 5=>10);

$IQFilePath = '/cpc/user/hd-usr/Playback/Fading/';
$WavFilePath = 'C:/drm/AudioFiles/Aqua';
$hdoggCommand = 'C:/Program Files/hdogg252/harddisk.exe';
#$RxName = 'Test';

@ChannelParams = (
  # Channel 1
  [
    {Delay => 0, Atten => 0, DopplerShift => 0, DopplerSpread => 0}
  ],

  # Channel 2
  [
    {Delay => 0, Atten => 0, DopplerShift => 0, DopplerSpread => 0},
    {Delay => 1, Atten => 6, DopplerShift => 0, DopplerSpread => 0.5}
  ],
  
  # Channel 3
  [
    {Delay => 0, Atten => 0, DopplerShift => 0.1, DopplerSpread => 0.1},
    {Delay => 0.7, Atten => 3, DopplerShift => 0.2, DopplerSpread => 0.5},
    {Delay => 1.5, Atten => 6, DopplerShift => 0.5, DopplerSpread => 1},
    {Delay => 2.2, Atten => 12, DopplerShift => 1, DopplerSpread => 2}    
  ],

  # Channel 4
  [
    {Delay => 0, Atten => 0, DopplerShift => 0, DopplerSpread => 1},
    {Delay => 2, Atten => 0, DopplerShift => 0, DopplerSpread => 1},
  ],

  # Channel 5
  [
    {Delay => 0, Atten => 0, DopplerShift => 0, DopplerSpread => 2},
    {Delay => 4, Atten => 0, DopplerShift => 0, DopplerSpread => 2},
  ],

);

# print "Channel 2 path 2 gain = ${${$ChannelParams[1]}[1]}{Gain}";  


# ----------------------------------------------------------------------------
# file parameters

# select signal source
DtHttp::SetParameter($pHttpHandle, 'File.InputSource', Dt230::SOURCE_FILE);  # select file input
DtHttp::SetParameter($pHttpHandle, 'File.Media',       Dt230::MEDIA_HD);     # select internal hard disk

# select playback file
$IQFileName = "$IQFilePath$IQFileNames{$TestNum}";
print ("Filename = $IQFileName\n");

DtHttp::SetParameter($pHttpHandle, 'File.Name', $IQFileName);

# NOTE: sample rate is automatically choosen by extension of playback file

# ----------------------------------------------------------------------------
# setup parameters

# input frequency
DtHttp::SetParameter($pHttpHandle, 'Setup.InputFrequency', 12000);

# output frequency
DtHttp::SetParameter($pHttpHandle, 'Setup.OutputFrequency', 12000);

# ----------------------------------------------------------------------------
# channel parameters

# enable propagation simulation
DtHttp::SetParameter($pHttpHandle, 'Channel.PropagationEnable', Dt230::ON);


# Set all paths off to begin with
for ($path = 0; $path < 4; $path ++) {
  DtHttp::SetParameter($pHttpHandle, 'Channel.PathEnable:'.($path+1),  Dt230::OFF);
}

print("Recording in clean channel\n");

# Reference recording in clean channel
#Set path 1 on, with no fading
DtHttp::SetParameter($pHttpHandle, 'Channel.PathEnable:1',  Dt230::ON);
DtHttp::SetParameter($pHttpHandle, 'Channel.PathDelay:1', 0); # ms
DtHttp::SetParameter($pHttpHandle, 'Channel.PathAttenuation:1',  0); # dB
DtHttp::SetParameter($pHttpHandle, 'Channel.PathDopplerSpread:1', 0); # Hz
DtHttp::SetParameter($pHttpHandle, 'Channel.PathDopplerShift:1', 0); # Hz

DtHttp::SetParameter($pHttpHandle, 'Channel.AwgnEnable', Dt230::OFF); # no AWGN
DtHttp::SetParameter($pHttpHandle, 'Channel.InterfererType', Dt230::INTERFERER_OFF);   # no interferer

# start playback
print("Starting playback\n");
DtHttp::SetParameter($pHttpHandle, 'Play', 1);

sleep(5); # wait 5 seconds before enabling RF output

#print("Enabling RF output\n");

DtHttp::SetParameter($pHttpHandle, 'RfOn', Dt230::ON);

$WavFileNameClean = $WavFilePath."/RxAudioClean_".$RxName."_".$TestNum.".wav";

sleep(10); # wait 10 seconds before starting recording

print("Starting recording of file $WavFileNameClean\n");

system ("$hdoggCommand", "-preset", "wav48.hdp", "-filter", "NormNone.hfs", "-output", "$WavFileNameClean", "-overwrite", "-record", "-timelimit", "5", "-tos");

print("Setting parameters for channel\n");

my $channel = $ChannelParams[$TestNum - 1];

for ($path = 0; $path < scalar(@{$channel}); $path++) {
  print "Path $path\n";
  #print "Delay = ${${$channel}[$path]}{Delay}\n";

  DtHttp::SetParameter($pHttpHandle, 'Channel.PathEnable:'.($path+1),  Dt230::ON);
  DtHttp::SetParameter($pHttpHandle, 'Channel.PathDelay:'.($path+1),         ${${$channel}[$path]}{Delay}); # ms
  DtHttp::SetParameter($pHttpHandle, 'Channel.PathAttenuation:'.($path+1),   ${${$channel}[$path]}{Atten}); # dB
  DtHttp::SetParameter($pHttpHandle, 'Channel.PathDopplerSpread:'.($path+1), ${${$channel}[$path]}{DopplerSpread}); # Hz
  DtHttp::SetParameter($pHttpHandle, 'Channel.PathDopplerShift:'.($path+1),  ${${$channel}[$path]}{DopplerShift}); # Hz
  
}

# power settings
DtHttp::SetParameter($pHttpHandle, 'Channel.PowerSelect', Dt230::POWER_INPUT);
	# power values of AWGN and interferer are relative to simulation input power

# AWGN settings
$TargetSNR = $TargetSNRs{$TestNum};
$Bandwidth = $Bandwidths{$TestNum};

DtHttp::SetParameter($pHttpHandle, 'Channel.AwgnEnable', Dt230::ON); # enable AWGN
DtHttp::SetParameter($pHttpHandle, 'Channel.AwgnPower',    -$TargetSNR); # dB, will be reset in playback loop
DtHttp::SetParameter($pHttpHandle, 'Channel.AwgnBandwidth', $Bandwidth); # kHz

# interferer settings
 DtHttp::SetParameter($pHttpHandle, 'Channel.InterfererType', Dt230::INTERFERER_OFF);   # no interferer

# set output attenuation
DtHttp::SetParameter($pHttpHandle, 'Channel.Attenuation', 20);

# get minimal attenuation
my $MinAttn;
DtHttp::GetParameter($pHttpHandle, 'Channel.MinAttenuation', \$MinAttn);
print STDOUT "minimal attenuation: $MinAttn dB\n";

# ----------------------------------------------------------------------------

print("Starting playback\n");
# start playback
DtHttp::SetParameter($pHttpHandle, 'Play', 1);

sleep(5); # wait 5 seconds before enabling RF output

print("Enabling RF\n");

DtHttp::SetParameter($pHttpHandle, 'RfOn', Dt230::ON);

for ($SNR=$TargetSNR-$SNRRange; $SNR<=$TargetSNR+$SNRRange; $SNR++) {

  $WavFileName = $WavFilePath."/RxAudio_".$RxName."_".$TestNum."_".$SNR.".wav";
  $TestDurationSeconds = $TestDurationsMinutes{$TestNum} * 60;

  print("Setting SNR to $SNR\n");
  DtHttp::SetParameter($pHttpHandle, 'Channel.AwgnPower',    -$SNR); # dB

  sleep(10); # wait 10 seconds before starting recording

  print("Starting error measurement script\n");
  unless ($pid = fork) {
  	system("cmd.exe /c cd C:\\drm\\AquaRxTesting\\RSCIscripts && tclsh MeasureRxErrors.tcl $TestDurationSeconds $TestNum $RxName $SNR $WavFilePath");
        exit 0;
  }

  print ("Starting recording to file $WavFileName for $TestDurationSeconds seconds\n");

  system ("$hdoggCommand", "-preset", "wav48.hdp", "-filter", "NormNone.hfs", "-output", "$WavFileName", "-overwrite", "-record", "-timelimit", "$TestDurationSeconds", "-tos");

  print ("Waiting for process $pid to exit\n");
  waitpid($pid,0);


  $matlabCommand = "\"cd 'C:/drm/AquaRxTesting/matlab'; RunAudioError\(\'$TestNum\', \'$RxName\', \'$SNR\'\,\'$WavFilePath\'); exit;\"";
  print("Matlab command:\n$matlabCommand\n");
  system("Matlab.exe", "/r", $matlabCommand);

}

};

if ($@) {
    print STDERR $@;
    
    my $Message;
    DtHttp::GetParameter($pHttpHandle, 'Message', \$Message);
    print STDERR "$Message\n" unless ($Message eq "");

    # release lock
    DtHttp::SetLock($pHttpHandle, 0);

    exit 1;
}
