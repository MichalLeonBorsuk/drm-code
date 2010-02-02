# Read common code and global variables
source settings.tcl

package require rsciUtil
package require rsci
package require udp

# Set IP addresses and ports
set STATUS_IN_PROTOCOL UDP
set STATUS_TCP_HOST 192.168.1.55
set STATUS_TCP_PORT [expr $PORT_BASE + 21]

set STATUS_OUT_PROTOCOL UDP
set STATUS_OUT_HOST localhost
set STATUS_OUT_PORT [expr $PORT_BASE + 11]

set STATISTICS [list 0 50 100]
set SUBSAMPLE_FACTOR 10

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

proc CalcStats {values stats} {
	set result [list]
	set sortedVals [lsort -integer $values]
	set numVals [llength $values]
	foreach stat $stats {
		lappend result [lindex $sortedVals [expr $stat * ($numVals-1) / 100]]			
	}
	return $result
}

proc CalcAudioStats {rafsTotal} {
	set nAudioFrames [string length $rafsTotal]
	set nCorrectFrames 0
	for {set i 0} {$i<$nAudioFrames} {incr i} {
		if {[string range $rafsTotal $i $i] == "0"} {
			incr nCorrectFrames
		}
	}
	return [list $nAudioFrames $nCorrectFrames]
}

proc ProcessStatusData {tcpId} {
	# Read status tag packet, modify it and send it to the output port
	global statusId
	global SE127_PROTOCOL
	global STATUS_IN_PROTOCOL 

	global LATITUDE
	global LONGITUDE
	global RX_NAME

	global summary
	global frameCount

	global USE_EXTERNAL_SIGNAL_STRENGTH
	global SUBSAMPLE_FACTOR
	global STATISTICS

    	# Data is returned in 3 binary segments: (IO)id, (IO)header and IO(payload)
    	if {$SE127_PROTOCOL=="IPIO"} {
	    set binarySegments [ReadIPIOPacket $tcpId]
    	}
    	if {$SE127_PROTOCOL=="DCP"} {
	    set binarySegments [ReadDCPPacket $tcpId]
    	}


	set tagContent [lindex $binarySegments 2]
	puts "got packet tag length is [string length $tagContent]"

      # Process the TAG content
      if {[ProcessTagContent $tagContent tags] == -1} {
      	puts "Error with tags!"
	  return -1
      }

    # Now read the status and add it to the summary for later processing

    ### AFS ###
    # Process rafs string and update number of receiver frames noafs
    if {$tags(rafs) != ""} {

	# Here, we have received audio frames
	set summary(noafs) [string length $tags(rafs)]
    } else {

	# If we do not know the number of audioframes (as at the beginning of a reception),
	# set to default value
	if ![info exists summary(noafs)] {
	    set summary(noafs) 10
	}

	# Append "2"s for for audio frame losses. Use the number of counted AFS of the previous TX frame
	for {set i 0} {$i<$summary(noafs)} {incr i} {
	    append tags(rafs) "2"
	}
    }

    # Add current AFS to rafString
    append summary(rafsTotal) $tags(rafs)


    ### RMER ###
    set tagName "rmer"
    if {$tags($tagName) != ""} {
	append summary($tagName\Total) " $tags($tagName)"
    }
    
    ### RWMF ###
    set tagName "rwmf"
    if {$tags($tagName) != ""} {
	append summary($tagName\Total) " $tags($tagName)"
    }

    ### RWMM ###
    set tagName "rwmm"
    if {$tags($tagName) != ""} {
	append summary($tagName\Total) " $tags($tagName)"
    }

    ### RDEL ###
    set tagName "rdel"
    if {$tags($tagName) != ""} {
	foreach double $tags($tagName) {
		set percentage [lindex $double 0]
		set delay [lindex $double 1]
		append summary($tagName\Total_$percentage) " $delay"
	}	
    }

    ### RDOP ###
    set tagName "rdop"
    if {$tags($tagName) != ""} {
	append summary($tagName\Total) " $tags($tagName)"		
    }

    ### RDBV ###
    set tagName "rdbv"

    # If the signal strength is read from an external source, replace the value provided by the DRM receiver
    if {$USE_EXTERNAL_SIGNAL_STRENGTH == 1 && [info exists tags(rfre)]} {

	# Establish frequency (FhG Rx does not always provide frequency)
	if {[string length $tags(rfre)] > 0} {
	    set freq [expr round($tags(rfre)/1000)]
	} else {

	    puts "rfre tag is empty!"

	    # Use global frequency if it exists
	    if [info exists frequency] {
		set fre $frequency
	    }
	}	       

	# Read AGC on fre kHz
	if [info exists fre] {
	    set tags($tagName) [expr [GetAGC $fre]+107]
	} else {
	    set tags($tagName) ""
	}
    }

    if {$tags($tagName) != ""} {
	append summary($tagName\Total) " $tags($tagName)"
    }
 

      incr frameCount

      if {$frameCount < $SUBSAMPLE_FACTOR} {
        # Don't send this time
        return
      }
	set frameCount 0
	puts "Generating output packet\n"

      if [info exists CHANGE_RXID] {
        set newTagContent [ChangeRxID $tagContent $RX_NAME]
      } else {
        set newTagContent $tagContent
      }

      if [info exists CHANGE_GPS] {
        append newTagContent [BuildTagItemGPS [list $LATITUDE $LONGITUDE]]
      }

	# Calculate and add statistics tags
	set tagBaseName "wmf"
	if [info exists summary(r$tagBaseName\Total)] {
	      set rwmfStats [CalcStats $summary(r$tagBaseName\Total) $STATISTICS]
      	      append newTagContent [BuildTagItemStatsdB $SUBSAMPLE_FACTOR $rwmfStats $STATISTICS "x$tagBaseName"]
	}
      
	set tagBaseName "wmm"
	if [info exists summary(r$tagBaseName\Total)] {
	      set rwmfStats [CalcStats $summary(r$tagBaseName\Total) $STATISTICS]
      	      append newTagContent [BuildTagItemStatsdB $SUBSAMPLE_FACTOR $rwmfStats $STATISTICS "x$tagBaseName"]
	}
      
	set tagBaseName "mer"
	if [info exists summary(r$tagBaseName\Total)] {
	      set rwmfStats [CalcStats $summary(r$tagBaseName\Total) $STATISTICS]
      	      append newTagContent [BuildTagItemStatsdB $SUBSAMPLE_FACTOR $rwmfStats $STATISTICS "x$tagBaseName"]
	}
      
	set tagBaseName "dbv"
	if [info exists summary(r$tagBaseName\Total)] {
	      set rwmfStats [CalcStats $summary(r$tagBaseName\Total) $STATISTICS]
      	      append newTagContent [BuildTagItemStatsdB $SUBSAMPLE_FACTOR $rwmfStats $STATISTICS "x$tagBaseName"]
	}

	set audioStats [CalcAudioStats $summary(rafsTotal)]
	append newTagContent [BuildTagItemAudioStats $audioStats]
      
	# delete summary
	unset summary

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

set frameCount 0

# Configure incoming status data on TCP port to be forwarded to STATUS_UDP_PORT and STATUS_OUT_PORT
fileevent $tcpId readable [list ProcessStatusData $tcpId] 


while {1} {
	
	set waitVar 0
	after 1000 {set waitVar 1}
	vwait waitVar
}
