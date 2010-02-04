# Read common code and global variables
source SettingsRemoteMonitoring.tcl

# Include the rsci library
package require rsci

# Include support for UDP
package require udp

# Server to send status information
proc StartSiteServer {port} {
	socket -server AcceptConnectionSiteSvr $port
	puts "TCP site server started on port $port."
}

proc AcceptConnectionSiteSvr {socket addr port} {

	global siteSocketBuffers
	global siteSubscriberCount
	global siteAddresses
	global siteConnectTime
	global sitePacketCount
	global NORMAL_PROFILE

	fconfigure $socket -blocking 0
	fconfigure $socket -translation binary
	fileevent $socket readable [list ProcessSiteInput $socket $addr $port]
	puts "Connection established with $addr:$port"

	# Start with an empty string in the buffer
	set siteSocketBuffers($socket) ""
	# Store the IP address for diagnostics
	set siteAddresses($socket) "$addr:$port"
	# and connection time
	set siteConnectTime($socket) [clock format [clock seconds] -gmt 1]
	# and a count of packets receiver
	set sitePacketCount($socket) 0

	# Set the profile to the "normal" i.e. non-streaming one
	CmdSetProfile $socket $NORMAL_PROFILE
}

proc ProcessSiteInput {socket addr port} {

	global siteSocketBuffers
	global siteAddresses
	global sitePacketCount
	global siteConnectTime

	global socketToID
	global idToSocket


	# This function is called when data is sent on TCP socket 

	puts "Received stuff on socket $socket $addr:$port"

	if [eof $socket] {
		puts "EOF"
		catch {close $socket}
		if [info exists socketToID($socket)] {
			# Expunge the lookup tables of this connection
			puts "ID = $socketToID($socket)"
			unset idToSocket($socketToID($socket))
			unset socketToID($socket)
			unset siteAddresses($socket)
			unset sitePacketCount($socket)
			unset siteConnectTime($socket)
		}
		return
	}

	set inData [read $socket]


	# prepend anything we have received already
	if {$siteSocketBuffers($socket) != ""} {
		set inData "$siteSocketBuffers($socket)$inData"
	}

	set id "AF"

	while {[string length $inData] != 0} {
		# find the AF (TODO add PF)

		set startPos [string first $id $inData] 
		if {$startPos == -1} {
			# AF was not found, so store the story so far in the buffer and wait for more data
			set siteSocketBuffers($socket) $inData
			return
		}

		# We have found an AF somewhere

		# trim off everything before the AF
		set inData [string range $inData $startPos end]

		set headerLen 10
		if {[string length $inData] < $headerLen} {
			# not enough data for header
			set siteSocketBuffers($socket) $inData
			return
		}

		set header [string range $inData 2 [expr $headerLen - 1]]

		# Decode header
		binary scan $header "IScA" len seq ar pt

		# Convert to unsigned numbers
		set len [expr ($len + 0x1000000)%0x1000000]
		set seq [expr ($seq + 0x10000)%0x10000]
		set ar [expr ($ar + 0x100) % 0x100]
		   set crc [expr (($ar & 0x80) >> 7)]
		   set maj [expr (($ar & 0x70) >> 4)]
		   set min [expr (($ar & 0x0F) >> 0)]

		# puts [format "DCP: len: %4x\t seq: %4x\tcrc: %1x\tversion: %x.%x %s" $len $seq $crc $maj $min $pt]

		# If the length is greater than 10000 bytes, there is an error
		if {$len > 10000} {
			# packet length too long - AF string might be data mimicing header. Throw away input
			puts "Error! Packet size is long: $len bytes. Ignoring."
			set siteSocketBuffers($socket) ""
			return
		}

		if {[string length $inData] < [expr $len + $headerLen]} {
			# not enough data for the payload left, so store and wait for more data
			set siteSocketBuffers($socket) $inData
			return
		}

			
		# Read <len> payload bytes
		set payload [string range $inData $headerLen [expr $len + $headerLen - 1]]

		set tail [string range $inData [expr $len + $headerLen] [expr $len + $headerLen + 2]]

		puts [format "DCP: len: %4x\t seq: %4x\tcrc: %1x\tversion: %x.%x %s" $len $seq $crc $maj $min $pt]
		ProcessStatusPacket $socket $id $header $payload $tail
		set sitePacketCount($socket) [expr $sitePacketCount($socket) + 1]

		set inData [string range $inData [expr $len + $headerLen + 2] end]
	}

	set siteSocketBuffers($socket) $inData

}
	

proc ProcessStatusPacket {socket id header tagContent tail} {
	global socketToID
	global idToSocket
	global ttyCollector
	global siteSocketBuffers
	global siteAddresses
	global sitePacketCount
	global siteConnectTime
	global siteSubscriberCount
	
	global STREAMING_PROFILE


	if [info exists socketToID($socket)]  { 
		# we already know the ID for this socket, so don't need to decode the TAG layer

		set rxID $socketToID($socket)
		#puts "Already know the rxID: $rxID"

	} else { # we don't know the ID for this connection so try to decode the packet

		if {[ProcessTagContent $tagContent tags] == -1} {
    		puts "Error with tags!"
			return
		}

		if ![info exists tags(rinf)] {
			#no receiver id - discard
			return
		}

		# ShowTags tags
		
		# Get receiver ID to use as key
		set rinf $tags(rinf);
		set rxID [string range $rinf 10 15]
		#puts "Got packet with RxID:  $rxID"

		# Store in a lookup table each way
		# first expunge any existing connections for that ID
		if [info exists idToSocket($rxID)] {
			catch {close $idToSocket($rxID)}
			unset siteSocketBuffers($idToSocket($rxID))
			unset siteAddresses($idToSocket($rxID))
			unset socketToID($idToSocket($rxID))
			unset sitePacketCount($idToSocket($rxID))
			unset siteConnectTime($idToSocket($rxID))
		}
		set socketToID($socket) $rxID
		set idToSocket($rxID) $socket
#puts "rxID: $rxID..";
#puts "socket: $idToSocket($rxID)..";
		if ![info exists siteSubscriberCount($rxID)] {
			set siteSubscriberCount($rxID) 0
		} else {
			if {$siteSubscriberCount($rxID) != 0} {
				# At least one subscriber: Set to high profile
				puts "$rxID: setting to profile $STREAMING_PROFILE"
				CmdSetProfile $idToSocket($rxID) $STREAMING_PROFILE
			}
		}

	}

	#puts "lengths of parts [string length $id] [string length $header] [string length $tagContent] [string length $tail]"
	# Forward as UDP packet to the collector script	
	puts -nonewline $ttyCollector "$id$header$tagContent$tail"
	#flush $ttyCollector

	#flush $ttyCollector

    #set outString ""
    #for {set i 0} {$i<[string length $tagContent]} {incr i} {
	#binary scan [string range $tagContent $i $i] "c" byte
	#set byte [expr ($byte+0x100)%0x100]
	#append outString [format "%02x " $byte]#
	#if {[expr $i%16] == 15} {
	#    append outString "\n"
	#}
    #}
 	#puts $outString



	# Forward to all subscribers

	ForwardToSubscribers $rxID $id$header$tagContent$tail

}

proc DecSubscriberCount {rxID} {
	global idToSocket
	global siteSubscriberCount
	global NORMAL_PROFILE

	# Decrement subscriber count
	if [info exists siteSubscriberCount($rxID)] {
		set siteSubscriberCount($rxID) [expr $siteSubscriberCount($rxID) - 1]
		puts "reduced count on $rxID to $siteSubscriberCount($rxID)"

		if {$siteSubscriberCount($rxID) == 0} {
			# Last subscriber has unsubscribed: Set to low profile
			puts "$rxID: setting to profile $NORMAL_PROFILE"
			if [info exists idToSocket($rxID)] {
				CmdSetProfile $idToSocket($rxID) $NORMAL_PROFILE
			}
		}
	}
}

proc ForwardToSubscribers {rxID packet} {

	global subscribers
	global subscriberAddresses
	global subscriberConnectTime
	global subscriberPacketCount

	foreach socket [array names subscribers] {
		puts "Iffing $subscribers($socket) against $rxID"
		if {$subscribers($socket) == $rxID} {
			puts "Forwarding to $socket length [string length $packet]"
			puts -nonewline $socket $packet
			if [catch {flush $socket}] {
				catch {close $socket}
				unset subscribers($socket)
				unset subscriberAddresses($socket)
				unset subscriberConnectTime($socket)
				unset subscriberPacketCount($socket)
				DecSubscriberCount $rxID
			}
			set subscriberPacketCount($socket) [expr $subscriberPacketCount($socket) + 1]
		}
	}
}

# Server for subscribing clients
proc StartSubscriptionServer {port} {
	socket -server AcceptConnectionSubscriptionSvr $port
	puts "TCP subscription server started on port $port."
}

proc AcceptConnectionSubscriptionSvr {socket addr port} {

	global subscribers
	global subscriberAddresses
	global subscriberConnectTime
	global subscriberPacketCount

	fconfigure $socket -blocking 0
	fconfigure $socket -translation binary
	fileevent $socket readable [list ProcessSubscriberInput $socket $addr $port]
	puts "Subscriber connection established with $addr:$port"

	# Start off subscribed to no stream
	set subscribers($socket) ""
	
	# store the IP address and port (for diagnostics only)
	set subscriberAddresses($socket) "$addr:$port"
	set subscriberConnectTime($socket) [clock format [clock seconds] -gmt 1]
	set subscriberPacketCount($socket) 0

}

proc ProcessSubscriberInput {socket addr port} {

	# This function is called when data is sent on TCP socket from a subscriber

	global subscribers
	global siteSubscriberCount
	global idToSocket
	global subscriberAddresses
	global subscriberConnectTime
	global subscriberPacketCount

	global NORMAL_PROFILE
	global STREAMING_PROFILE

	puts "Received command from subscriber on socket $socket $addr:$port"

	# What are they currently subscribed to?
	set rxID $subscribers($socket)

	# Decrement subscriber count and change profile if necessary
	DecSubscriberCount $rxID

	# subscribe them to nothing
	set subscribers($socket) ""

	if [eof $socket] {
		puts "EOF"
		catch {close $socket}
		if [info exists subscribers($socket)] {
			# Expunge the lookup tables of this connection
			unset subscribers($socket)
			unset subscriberAddresses($socket)
			unset subscriberConnectTime($socket)
			unset subscriberPacketCount($socket)
		}
		return
	}

	set inData [read $socket]

	set rxID [string trim $inData "\n"]
        puts "Sub rxID: $rxID.."
	if {$rxID == ""} {
		return
	}

	# Increment subscriber count - set to 0 first if doesn't exist
	if ![info exists siteSubscriberCount($rxID)] {
		set siteSubscriberCount($rxID) 0
	}
	set siteSubscriberCount($rxID) [expr $siteSubscriberCount($rxID) + 1]
        puts "increased count on $rxID to $siteSubscriberCount($rxID)"
	if {$siteSubscriberCount($rxID) == 1} {
		# First subscriber: Set to high profile
		puts "$rxID: setting to profile $STREAMING_PROFILE"
		CmdSetProfile $idToSocket($rxID) $STREAMING_PROFILE
	}

	set subscribers($socket) $rxID

	puts "socket $socket has subscribed to $subscribers($socket)"

	
}

proc ConnectToCollector {port} {
	set tty [udp_open]
	fconfigure $tty -remote [list localhost $port]
	#fconfigure $tty -remote [list 132.185.128.1 $port]
	#fconfigure $tty -remote [list 172.29.136.85 $port]
	fconfigure $tty -buffering none -translation binary
	return $tty
}

proc StartDiagServer {port} {
	socket -server AcceptConnectionDiag $port
	puts "TCP diagnostic server started on port $port."
}

proc AcceptConnectionDiag {socket addr port} {
	global siteSubscriberCount
	global siteAddresses
	global sitePacketCount
	global siteConnectTime
	global subscribers
	global subscriberAddresses
	global subscriberConnectTime
	global subscriberPacketCount
	global socketToID
	global idToSocket

	puts "Diagnostics connection established with $addr:$port"

	fconfigure $socket -translation binary

	puts $socket "<!DOCTYPE html>"
	puts $socket "<html>"
	puts $socket "<B>SITE SOCKETS:</B>"
        puts $socket "<table border=\"1\" cellspacing=\"1\" cellpadding=\"3\">"
        puts $socket "<tr><th>Socket</th><th>Address</th><th>Rx ID</th><th>Connected</th><th>packets received</th>"
	puts $socket "</tr>"

	foreach siteSocket [array names siteAddresses] {
		puts $socket "<tr>"
		puts $socket "<td>$siteSocket</td><td>"
		if [info exists siteAddresses($siteSocket)] {
			puts $socket "$siteAddresses($siteSocket)"
		}
		puts $socket "</td><td>"
		if [info exists socketToID($siteSocket)] {
			puts $socket "$socketToID($siteSocket)"
		}
		puts $socket "</td><td>"
		if [info exists siteConnectTime($siteSocket)] {
			puts $socket "$siteConnectTime($siteSocket)"
		}
		puts $socket "</td><td>"
		if [info exists sitePacketCount($siteSocket)] {
			puts $socket "$sitePacketCount($siteSocket)"
		}
		puts $socket "</td></tr>"
	}
	puts $socket "</table>"

	puts $socket "<B>SITE IDs:</B>"
        puts $socket "<table border=\"1\" cellspacing=\"1\" cellpadding=\"3\">"
        puts $socket "<tr><th>RxID</th><th>Socket</th><th>Subscriber Count</th>"
	puts $socket "</tr>"

	foreach rxID [array names siteSubscriberCount] {
		puts $socket "<tr>"
		puts $socket "<td>$rxID</td><td>"
		if [info exists idToSocket($rxID)] {
			puts $socket "$idToSocket($rxID)"
		}
		puts $socket "</td><td>$siteSubscriberCount($rxID)</td>"
		puts $socket "</tr>"
	}
	puts $socket "</table>"

	puts $socket "<B>SUBSCRIBERS:</B>"
        puts $socket "<table border=\"1\" cellspacing=\"1\" cellpadding=\"3\">"
        puts $socket "<tr><th>Socket</th><th>Address</th><th>Connected</th><th>Rx ID</th><th>Packets sent</th><th>EOF?</th>"
	puts $socket "</tr>"
	foreach subscriberSocket [array names subscribers] {
		puts $socket "<tr>"
		puts $socket "<td>$subscriberSocket</td><td>$subscriberAddresses($subscriberSocket)</td><td>$subscriberConnectTime($subscriberSocket)</td><td>$subscribers($subscriberSocket)</td>"
		puts $socket "<td>$subscriberPacketCount($subscriberSocket)</td>"
		puts $socket "<td>[eof $subscriberSocket]</td>"
		puts $socket "</tr>"
		
	}
	puts $socket "</table>"
	close $socket

	
}

proc ConfigureControlPort {} {

    # Configures connection for control commands (e.g. change freq).
    # File id is returned

    global CONTROL_PORT

    set tty [udp_open $CONTROL_PORT]
    fconfigure $tty -buffering none -translation binary

    fconfigure $tty -blocking 1

    return $tty

}

proc ProcessControlInput {socket} {
    global idToSocket
    set input [read $socket]
    puts "Got control input\n$input\n"
    if {[scan $input "<cfre rxID=\"%\[^\"\]\" mode=\"%\[^\"\]\">%d</cfre>" rxIDs mode freq] == 3} {
      puts "matched ids: $rxIDs freq $freq"
      foreach rxID [split $rxIDs ","] {
          puts "tuning $rxID to $freq\n"
        if [info exists idToSocket($rxID)] {
          puts "Socket $idToSocket($rxID)"
          #CmdSetFrequency $idToSocket($rxID) $freq "drm_" 0 0
          CmdSetFrequency $idToSocket($rxID) $freq $mode 0 0
        }
      }
    }

}
##### Main #####

set ttyCollector [ConnectToCollector $COLLECTOR_PORT]

# Server to send status information
proc StartSiteServer {port} {
	socket -server AcceptConnectionSiteSvr $port
	puts "TCP site server started on port $port."
}

StartSiteServer $SITES_SERVER_PORT

StartSubscriptionServer $SUBSCRIBERS_SERVER_PORT

StartDiagServer $MEDIATOR_DIAG_SERVER_PORT

set ttyControl [ConfigureControlPort]
fileevent $ttyControl readable [list ProcessControlInput $ttyControl]

while {1} {
	puts "waiting"
	set waitVar 0
	puts "about to after"
	#after 1000 {set waitVar 1}
	puts "about to vwait"
	vwait waitVar
	puts "waited"
}
puts "exiting"
