#!/usr/bin/perl
#*******************************************************************************************
# dthttp.pm 
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
# $Id: dthttp.pm,v 1.26 2007/10/04 14:08:45 kln Exp $
#*******************************************************************************************
#
# Description: This module implements a http client using the HTTP::Request
#              perl module. The client is used to set and get parameters.
#
#*******************************************************************************************


{
package MyUserAgent;
    use strict;
    use warnings;
    use LWP::UserAgent;
    
    our @ISA = qw(LWP::UserAgent);
    
    sub get_basic_credentials
    {
	my ($this, $realm, $uri, $isproxy) = @_;
	
	my ($Username, $Password) = LWP::UserAgent::get_basic_credentials(@_);
	if(defined($Username) and defined($Password))
	{
#	    print 'using given username and password for: ' . $Username . "\n";;
	}
	else
	{
	    print "Authentication required.\n";
	    print 'Please enter username and password for: "' . $realm . '"'. "\non " . $uri->scheme . '://' . $uri->authority .  "\n";
 
	    print 'Username: ';
	    $Username = <STDIN>;
	    chop($Username);

	    if ($^O =~ /win/i) 
	    {
		if( not defined eval { require Term::ReadPassword::Win32; } )
		{
		    print "ATTENTION entered password will be readable.\nInstall Term::ReadPassword::Win32 to hide password input.\n(ppm install Term::ReadPassword::Win32)\n";
		    print 'Password: ';
		    $Password = <STDIN>;
		}
		else
		{
		    print 'Password: ';
		    $Password = Term::ReadPassword::Win32::read_password();
		}
	    }
	    else 
	    {
		if( not defined eval { require Term::ReadPassword; } )
		{
		    print "ATTENTION entered password will be readable.\nInstall Term::ReadPassword to hide password input.\n";
		    print 'Password: ';
		    $Password = <STDIN>;
		}
		else
		{
		    print 'Password: ';
		    $Password = Term::ReadPassword::read_password();
		}
	    }
	    chomp($Password);
	    
	    my $host_port = lc($uri->host_port);
	    LWP::UserAgent::credentials($this, $host_port, $realm, $Username, $Password);
	}
        return ($Username, $Password);
    }
    1;
}
# ----------------------------------------------------------------------------
# definition of package
{
package DtHttp;

# let cvs insert the ID here
my $FileId = '$Id: dthttp.pm,v 1.26 2007/10/04 14:08:45 kln Exp $';

# ----------------------------------------------------------------------------
# includes
use strict;
use warnings;

use HTTP::Request;

our $DtHttpMagic = 0x2b782db8;

# this sub must be called in the superior module to enable messages
our ($Debug, $Verbose) = (0, 0);
sub DtHttp::Our2Package($$) {
	($Debug, $Verbose) = @_;
} # end of sub Our2Package

sub DtHttp::GetVersion() {
	return $FileId;
} # end of sub GetVersion

# ----------------------------------------------------------------------------
# register new communication handle
# arguments: hostname
# return: \handle (hash)
sub DtHttp::RegisterHandle($) {
	my ($HostName) = @_;
	my $pHandle = {Magic => $DtHttpMagic, HostName => $HostName, CommId => 0, Protocol => 'http'};

       $pHandle->{UserAgent} = MyUserAgent->new();
    
       if (($HostName eq '') || ($HostName eq 'INVALID')) {
		$pHandle = ();
	} elsif ($Debug) {
		print "  HostName: $pHandle->{HostName}\n";
		print "  CommId: $pHandle->{CommId}\n";
	}

	return $pHandle;
} # end of sub RegisterHandle

# ----------------------------------------------------------------------------
# change hostname of existing handle
# arguments: \handle (hash), hostname
# return: nothing
sub DtHttp::ChangeHostName($$) {
	my ($pHandle, $HostName) = @_;
	
	print "\nCHANGE HostName: $HostName\n" if ($Verbose);
	if ($pHandle->{Magic} == $DtHttpMagic) {
		$pHandle->{HostName} = $HostName;
		$pHandle->{CommId} = 0;
	
		if ($Debug) {
			print "  HostName: $pHandle->{HostName}\n";
			print "  CommId: $pHandle->{CommId}\n";
		}
	} else {
		die "*** ERROR *** wrong magic number ($pHandle->{Magic}) in handle\n";
	} # end if wrong magic number
} # end of sub ChangeHostName

# ----------------------------------------------------------------------------
# set authentication of existing handle
# arguments: \handle (hash), hostname
# return: nothing
sub DtHttp::SetAuth($$$$) {
	my ($pHandle, $Realm, $User, $Pwd) = @_;
	
	print "\nSET Auth: $User\n" if ($Verbose);
	if ($pHandle->{Magic} == $DtHttpMagic) {
		$pHandle->{Realm} = $Realm;
   	        $pHandle->{Username} = $User;
		$pHandle->{Password} = $Pwd;
	
		if ($Debug) {
		    print "  Realm:    $pHandle->{Realm}\n";
		    print "  Username: $pHandle->{Username}\n";
		}
	} else {
		die "*** ERROR *** wrong magic number ($pHandle->{Magic}) in handle\n";
	} # end if wrong magic number
} # end of sub SetAuth

# ----------------------------------------------------------------------------
# set protocol
# arguments: \handle (hash), protocol
# return: nothing
sub DtHttp::SetProtocol($$) {
    my ($pHandle, $Protocol) = @_;
	
    print "\nSET Protocol: $Protocol\n" if ($Verbose);
    if($Protocol eq 'http' || $Protocol eq 'https')
    {
	if ($pHandle->{Magic} == $DtHttpMagic) {
	    $pHandle->{Protocol} = $Protocol;
	    $pHandle->{Port} = 80 if($pHandle->{Protocol} eq 'http');
	    $pHandle->{Port} = 443 if($pHandle->{Protocol} eq 'https');
	
	    if ($Debug) {
		print "  Protocol: $pHandle->{Protocol}\n";
	    }
	} else {
	    die "*** ERROR *** wrong magic number ($pHandle->{Magic}) in handle\n";
	} # end if wrong magic number
    }
    else
    {
	die "*** ERROR *** wrong protocol: $Protocol (allowed: http or https)\n";
    }
} # end of sub SetProtocol

# ----------------------------------------------------------------------------
# write value to parameter
# parameter: \handle (hash), parameter, value
# return: nothing
sub DtHttp::SetParameter ($$$) {
	my ($pHandle, $Parameter, $Value) = @_;
	my $ua = $pHandle->{UserAgent};
        $ua->credentials($pHandle->{HostName}.':'.$pHandle->{Port}, $pHandle->{Realm}, $pHandle->{Username}, $pHandle->{Password}) if defined $pHandle->{Username};
        my $retval = 3;
	my $req = HTTP::Request->new('GET', $pHandle->{Protocol}.'://'.$pHandle->{HostName}
		.'/remote-access?'.$Parameter.'='.$Value.'&CommId='.$pHandle->{CommId});

	print "\nSET $Parameter Value: $Value\n" if ($Verbose);
	if ($pHandle->{Magic} == $DtHttpMagic) {
		if ($Debug) {
			print "  CommId=$pHandle->{CommId}\n";
			print "  Request=".$req->as_string."\n";
		}

		my $resp = $ua->request($req);
		if ($resp->is_success) {
			if ($resp->content =~ /Status: Ok/) {
			} elsif ($resp->content =~ /Status: Error/) {
				die "*** ERROR *** ".$resp->content."\n";
			} elsif ($resp->content =~ /Status: Warning/) {
				print "*** WARNING *** ".$resp->content."\n" if ($Verbose);
			}
		} else {
			die "*** COMMNUNICATION ERROR *** ".$resp->message."\n";
		} # end if response success
	} else {
		die "*** ERROR *** wrong magic number ($pHandle->{Magic}) in handle\n";
	} # end if wrong magic number
} # end of sub SetParameter

# ----------------------------------------------------------------------------
# read value of parameter
# arguments: \handle (hash), parameter name, \value
# return: nothing
sub DtHttp::GetParameter($$$) {
	my ($pHandle, $Parameter, $pValue) = @_;
	my $ua = $pHandle->{UserAgent};
        $ua->credentials($pHandle->{HostName}.':'.$pHandle->{Port}, $pHandle->{Realm}, $pHandle->{Username}, $pHandle->{Password}) if defined $pHandle->{Username};
        my $req = HTTP::Request->new('GET', $pHandle->{Protocol}.'://'.$pHandle->{HostName}
		.'/remote-access?GetParameter='.$Parameter);

	print "\nGET $Parameter Value:\n" if ($Verbose);
	if ($pHandle->{Magic} == $DtHttpMagic) {
		print "  Request=".$req->as_string."\n" if ($Debug);

		my $resp = $ua->request($req);
		if ($resp->is_success) {
			print "  Response=".$resp->content."\n" if ($Debug);
			if ($resp->content =~ /Status: Ok\s+GetVariable:\s+$Parameter\s+Value: (.*)/s) {
				($$pValue = $1) =~ s/\n$//g; # remove tailing newline character
				print "  Value=$$pValue\n" if ($Verbose);
			} elsif ($resp->content =~ /Status: Error/) {
				die "*** ERROR *** ".$resp->content."\n";
			} elsif ($resp->content =~ /Status: Warning/) {
				print "*** WARNING *** ".$resp->content."\n" if ($Verbose);
			} # end if response content
		} else {
			die "*** COMMNUNICATION ERROR *** ".$resp->message."\n";
		} # end if response success
	} else {
		die "*** ERROR *** wrong magic number ($pHandle->{Magic}) in handle\n";
	} # end if wrong magic number
} # end of sub GetParameter

# ----------------------------------------------------------------------------
# set (1) or release (0) lock
# arguments: \handle (hash), lock
# return: nothing
sub DtHttp::SetLock($$) {
	my($pHandle, $SetLock) = @_;

	print "\nWRITE Lock Value: $SetLock\n" if ($Verbose);
	if ($pHandle->{Magic} == $DtHttpMagic) {
		if ($SetLock) {
			print "  Set Lock\n" if ($Verbose);
			SetParameter($pHandle, 'Lock', 1);

			my $MyId;
			GetParameter($pHandle, 'LockId', \$MyId);
			print "  CommId: $MyId received\n" if ($Debug);

			$pHandle->{CommId} = $MyId;
			if ($pHandle->{CommId} == 0) {
				SetParameter($pHandle, 'Lock', 0);
				die "*** NO LOCK POSSIBLE *** $!\n";
			}
		} else {
			print "  Release Lock\n" if ($Verbose);
			$pHandle->{CommId} = 0;
			SetParameter($pHandle, 'Lock', 0);
		} # end if set lock
	} else {
		die "*** ERROR *** wrong magic number ($pHandle->{Magic}) in handle\n";
	} # end if wrong magic number
} # end of sub SetLock

# ----------------------------------------------------------------------------
# verify lock of specific communication id
# arguments: \handle (hash)
# return: 0 = has lock, <0 = lock lost
sub DtHttp::VerifyLock($) {
	my ($pHandle) = @_;
	my $LockId = -1;

	print "\nVERIFY Lock Value:\n" if ($Verbose);
	if ($pHandle->{Magic} == $DtHttpMagic) {
		GetParameter($pHandle, 'LockId', \$LockId);

		if ($LockId == $pHandle->{CommId}) {
			return 0;
		} else {
			return -1;
		}
	} else {
		die "*** ERROR *** wrong magic number ($pHandle->{Magic}) in handle\n";
	} # end if wrong magic number
} # end of sub VerifyLock

# ----------------------------------------------------------------------------
return 1;
}