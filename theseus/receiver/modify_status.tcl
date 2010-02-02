# Read common code and global variables
source settings.tcl

package require rsciUtil
package require rsci
package require udp

# Set IP addresses and ports
set STATUS_IN_PROTOCOL TCP
set STATUS_TCP_HOST 192.168.1.55
set STATUS_TCP_PORT [expr $PORT_BASE + 11]

set STATUS_OUT_PROTOCOL TCP
set STATUS_OUT_HOST 192.168.1.55
set STATUS_OUT_PORT [expr $PORT_BASE + 21]

# override anything in settings.tcl
set STATUS_PROTOCOL $STATUS_IN_PROTOCOL

# Server to send status information
proc StartStatusServer {port} {
	socket -server AcceptStatusConnectionSvr $port
	puts "TCP status server started on port $port."
}

proc AcceptStatusConnectionSvr {socket addr port} {

	global statusId

	fconfigure $socket -blocking 0
	fconfigure $socket -translation binary
	fileevent $socket readable [list ProcessStatusProblem]
	puts "Connection established with $addr:$port"
	set statusId $socket
}

proc ProcessStatusProblem {} {

	# This function is called when data is sent on TCP socket 
	# indicating that the conenction is lost

	global statusId

	read $statusId

	if [eof $statusId] {
		catch {close $statusId}  
		puts "Status connection lost"
		unset statusId
	}

}

proc ChangeRxID {tagContent rxID} {

    # force to length 6
    set rxID [string range ${rxID}______ 0 5]
    set pointer 0;
    set contentLength [string length $tagContent]

    # Go through individual tags
    while {$pointer != $contentLength} {

	# Tag name
	set tagName [string range $tagContent $pointer [expr $pointer+3]]

	# Data length in bytes
	binary scan [string range $tagContent [expr $pointer+4] [expr $pointer+7]] "I" dataBitSize
	set byteLength [expr $dataBitSize/8]

	# Treat empty tags
	if {$byteLength == 0} {
	    set tags($tagName) ""

	    # Advance pointer
	    set pointer [expr $pointer+8+$byteLength]

	    continue
	}

	if {$tagName=="rinf"} {
		return [string replace $tagContent [expr $pointer+18] [expr $pointer+23] $rxID]
	}

	# Advance pointer
	set pointer [expr $pointer+8+$byteLength]
    }

	# not found
	return $tagContent
}	

proc ProcessStatusData {tcpId} {
	# Read status tag packet, modify it and send it to the output port
	global statusId
	global SE127_PROTOCOL
	global STATUS_IN_PROTOCOL 

	global LATITUDE
	global LONGITUDE
	global RX_NAME

    	# Data is returned in 3 binary segments: (IO)id, (IO)header and IO(payload)
    	if {$SE127_PROTOCOL=="IPIO"} {
	    set binarySegments [ReadIPIOPacket $tcpId]
    	}
    	if {$SE127_PROTOCOL=="DCP"} {
	    set binarySegments [ReadDCPPacket $tcpId]
    	}


	set tagContent [lindex $binarySegments 2]
	puts "got packet tag length is [string length $tagContent]"

	set newTagContent [ChangeRxID $tagContent $RX_NAME]

	append newTagContent [BuildTagItemGPS [list $LATITUDE $LONGITUDE]]

	set outPacket [BuildLayer $newTagContent]

	# Close socket if connection breaks
	if {[eof $tcpId] && $STATUS_IN_PROTOCOL == "TCP"} {
		catch {close $tcpId}  
		puts "Control connection lost"; flush stdout
		unset tcpId
	}

	if {$outPacket != ""} {
		puts "Packet of length [string length $outPacket] generated!"; flush stdout
	}

	if [info exists statusId] {
		puts -nonewline $statusId $outPacket
		flush $statusId
	}
}



##### Main #####

if {$STATUS_IN_PROTOCOL == "TCP"} {
	# Open conenction to TCP PORT
	puts "Connecting to port $STATUS_TCP_PORT on $STATUS_TCP_HOST"
	set tcpId [socket $STATUS_TCP_HOST $STATUS_TCP_PORT]
	fconfigure $tcpId -translation binary -blocking 1
} else {
	# Open UDP port for reception
	# set tcpId [dp_connect udp -host $STATUS_TCP_HOST -myport $STATUS_TCP_PORT]
	set tcpId [udp_open $STATUS_TCP_PORT]

	fconfigure $tcpId -buffering none -translation binary  
	# -blocking 1??????
}

# Open main status out port - could be TCP or UDP
if {$STATUS_OUT_PROTOCOL == "TCP"} {
	# Start TCP server for outgoing status data
	StartStatusServer $STATUS_OUT_PORT
} else {
	#set statusId [dp_connect udp -host $STATUS_OUT_HOST -port $STATUS_OUT_PORT]
	set statusId [udp_open]
	fconfigure $statusId -remote [list $STATUS_OUT_HOST  $STATUS_OUT_PORT]

	fconfigure $statusId -translation binary -blocking 1
}

# Configure incoming status data on TCP port to be forwarded to STATUS_UDP_PORT and STATUS_OUT_PORT
fileevent $tcpId readable [list ProcessStatusData $tcpId] 


while {1} {
	
	set waitVar 0
	after 1000 {set waitVar 1}
	vwait waitVar
}