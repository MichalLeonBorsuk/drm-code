# Reads status information from receiver and uses to measure average BER and frame error rate.
# Call with: tclsh MeasureRxErrors.tcl

# Read common code and global variables
source SettingsReceiverTesting.tcl
package require rsci
package require rsciUtil

# Include support for UDP
if {$CONTROL_PROTOCOL == "UDP" || $STATUS_PROTOCOL == "UDP"} {
	if {$UDP_PACKAGE == "DP"} {
		package require dp
	} else {
		package require udp
	}
}

# Variables
set receivedAFS ""; # Received audio frame status


proc ConfigureStatusPort {} {

    # Configures connection for incoming data.
    # File id is returned

	global DRM_RX_STATUS_IP_ADDRESS
    global STATUS_PORT
    global STATUS_PROTOCOL
    global UDP_PACKAGE

#    set tty [open /dev/ttyb r+]
#    fconfigure $tty -mode 115200,n,8,1 -translation binary

	# For receivers that support the UDP protocol
	if {$STATUS_PROTOCOL == "UDP"} {
		if {$UDP_PACKAGE == "DP"} {
			set tty [dp_connect udp -host 127.0.0.1 -myport $STATUS_PORT]
			fconfigure $tty -translation binary 
			fconfigure $tty -blocking 1
		} else {
			set tty [udp_open $STATUS_PORT]
			fconfigure $tty -buffering none -translation binary
		}

	}	

	# For receivers that support the TCP protocol
	if {$STATUS_PROTOCOL == "TCP"} {
		set tty [socket $DRM_RX_STATUS_IP_ADDRESS $STATUS_PORT]
		fconfigure $tty -translation binary 
		fconfigure $tty -blocking 1
	}

    return $tty
}

proc ConfigureCmdPort {} {

    # Configures connection for outgoing data.
    # File id is returned

    global CONTROL_PORT
    global CONTROL_PROTOCOL
    global DRM_RX_CTRL_IP_ADDRESS
    global UDP_PACKAGE

    # For receivers using the UDP protocol for status
    if {$CONTROL_PROTOCOL == "UDP"} {
	if {$UDP_PACKAGE == "DP"} {
	    set tty [dp_connect udp -host $DRM_RX_CTRL_IP_ADDRESS -port $CONTROL_PORT]
	} else {
		set tty [udp_open]
		fconfigure $tty -remote [list $DRM_RX_CTRL_IP_ADDRESS $CONTROL_PORT]
	}
    } 

    # For receivers using the UDP protocol for status
    if {$CONTROL_PROTOCOL == "TCP"} {
	    set tty [socket $DRM_RX_CTRL_IP_ADDRESS $CONTROL_PORT]
    }

    fconfigure $tty -translation binary 
    fconfigure $tty -blocking 1

    return $tty
puts "\n Configure Command port."

}

proc MeasureAudioBER {str0} {

    global origAudioFrameLength
    global origAudioFrame
    global AUDIO_FILE_PATH

    set numErrors 0

    if {[string length $str0] == 0} {
      # need to wait until we know the frame length?
	if [info exists origAudioFrameLength] {
		#must have lost synchronisation - assume BER = 0.5
		return [list [expr $origAudioFrameLength * 4] [expr $origAudioFrameLength * 8]]
	} else {
	      return [list 0 0]
	}
    } else {
      if {![info exists origAudioFrameLength]} {
        set origAudioFrameLength [expr [string length $str0] - 4] ;# subtract 4 for text message
        set origAudioFileName "${AUDIO_FILE_PATH}_${origAudioFrameLength}_chopped.aac"
        set origAudioFile [open $origAudioFileName r]
        fconfigure $origAudioFile -translation binary
        set origAudioHeader [read $origAudioFile 44]
        set origAudioFrame [read $origAudioFile]
        puts "opened file $origAudioFileName and read frame length [string length $origAudioFrame]"
      }

    
      for {set i 0} {$i < $origAudioFrameLength} {incr i} {
        binary scan [string range $origAudioFrame $i $i] c orig
        binary scan [string range $str0 $i $i] c rec
        binary scan [binary format c [expr $orig ^ $rec]] B8 err

        set errList [split $err {}]
        set errCount [llength [lsearch -all $errList 1]]
        set numErrors [expr $numErrors + $errCount]
      }

      return [list $numErrors [expr $origAudioFrameLength * 8]]
    }
}


proc InformationCollector {inDataId} {
    global minuteSummary
    global frequency
    global USE_EXTERNAL_SIGNAL_STRENGTH
    global logFileId
    global binaryLogFileId
    global AFS_ERROR_BEEP
    global globalFrameCounter
    global SE127_VERSION 
    global AUDIO_FILE_PATH

    global totalAudioFrameCount
    global totalGoodAudioFrames
    global numAudioFrames 
    global totalBitCount
    global totalBitErrorCount

    global origAudioFrameLength
    # This function gets called when data is available at the COM port
    # Read 400 ms worth of information
    # Boy: There was an error reading the errorFlag... which crashed the scripts
    # occasionaly
    set errorFlag [GetTagInfo $inDataId tags binaryFrame]
    # Write to hard disk if the binary data is recorded
    if [info exists binaryLogFileId] {
	puts -nonewline $binaryLogFileId $binaryFrame
    }

    if {$errorFlag == -1} {
	puts "Receiver error!"
	return
    }	

	# Check receiver status
 #       if {[lindex $tags(rsta) 0] > 0} {
#		puts -nonewline "\rNo Signal! Ignoring packet. RSTA: $tags(rsta)\tDLFC: $tags(dlfc)       "
#		flush stdout
#		return 
#	}

 
    # Store info for later in minuteSummary
    foreach tagName [array names tags] {
	if {$tags($tagName) != ""} {
	    set minuteSummary($tagName) $tags($tagName)
	}
    }

    # Increase global frame counter
    if ![info exists globalFrameCounter] {
       set globalFrameCounter 0
	    } else {
      incr globalFrameCounter

    }


    # Store extra info

    ### AFS ###
    # Process rafs string and update number of receiver frames noafs
    if {$tags(rafs) != ""} {

	# Here, we have received audio frames
	set numAudioFrames [string length $tags(rafs)]
    } else {

	# If we do not know the number of audioframes (as at the beginning of a reception),
	# set to default value
	if ![info exists numAudioFrames] {
	    set numAudioFrames 10
	}

	# Append "2"s for for audio frame losses. Use the number of counted AFS of the previous TX frame
	for {set i 0} {$i<$numAudioFrames} {incr i} {
	    append tags(rafs) "2"
	}
    }

    set audioList [split $tags(rafs) {}]
    set goodAudioFrames [llength [lsearch -all $audioList 0]]

    if {![info exists totalAudioFrameCount]} {
      set totalAudioFrameCount 0
      set totalGoodAudioFrames 0
    }

    set totalAudioFrameCount [expr $totalAudioFrameCount + $numAudioFrames]
    set totalGoodAudioFrames [expr $totalGoodAudioFrames + $goodAudioFrames]

    # Measure BER using known input data
    if {![info exists tags(str0)]} {
      set tags(str0) ""
    }

    set audioErrors [MeasureAudioBER $tags(str0)]
    set totalBitErrorCount [expr $totalBitErrorCount + [lindex $audioErrors 0]]
    set totalBitCount [expr $totalBitCount + [lindex $audioErrors 1]]

#    puts -nonewline "\r                                                                                 "
#    puts -nonewline "                                                                                   \r"
#    puts -nonewline [clock format [clock seconds] -format "%H:%M:%S"]
#    puts -nonewline "\tDLFC: $tags(dlfc)"
#    puts -nonewline "\tRDBV:[lindex [CalcDistribution $tags(rdbv)] 2] dBuV"
#    puts -nonewline "\tRWMF:[lindex [CalcDistribution $tags(rwmf)] 2] "
#    puts -nonewline "\r                                                                                 "
#    puts -nonewline "                                                                                   \r"
puts -nonewline [clock format [clock seconds] -format "%H:%M:%S"]

puts -nonewline "\tAFS: $tags(rafs)"
    puts -nonewline "\tGoodFrames: $totalGoodAudioFrames / $totalAudioFrameCount"
    #puts -nonewline "\tOverall audio Quality: [expr 100 * $totalGoodAudioFrames / $totalAudioFrameCount]%"

   
    puts -nonewline "\t FER: [expr  (double($totalAudioFrameCount - $totalGoodAudioFrames) / $totalAudioFrameCount)]"

    puts -nonewline "\tBit Errors:  $totalBitErrorCount / $totalBitCount"
    if {$totalBitCount != 0} {
      #puts -nonewline "\tOverall BER: [expr 100 * $totalBitErrorCount / $totalBitCount]%"
        
       puts -nonewline "\tBER: [expr double($totalBitErrorCount) / $totalBitCount]"

    } else {
       puts -nonewline "\t Overall BER: ?"
    }
    puts -nonewline "\r"

    
    
#    puts -nonewline "\tFAC: $facContentList"
#    puts -nonewline "\tSDC: $sdcDeList"


    flush stdout

}




			
#### Start ####

set totalBitCount 0
set totalBitErrorCount 0

# Init currentStatus

set runTime [lindex $argv 0]
set testNum [lindex $argv 1]
set RxName [lindex $argv 2]
set SNR [lindex $argv 3]

puts "Measuring BER for $runTime seconds"

# Configure connection for incoming data
set statusPort [ConfigureStatusPort]
fileevent $statusPort readable [list InformationCollector $statusPort] 

# Configure conenction for outgoing data
set COMMAND_PORT [ConfigureCmdPort]

# Needed for periodic update of the receiver time
set startTime [clock seconds]

# Wait 100 ms
set flag 0
after [expr 1000 \* $runTime] {set flag 1}
vwait flag

set outFilename "${AQUA_PATH}/Results/${RxName}_RSCIResults.txt"
set outfile [open $outFilename "a"]
puts -nonewline $outfile "Case: $testNum\tSNR $SNR"
puts -nonewline $outfile "Frames: $totalAudioFrameCount\tGood: $totalGoodAudioFrames"
#puts -nonewline $outfile "\tOverall audio Quality: [expr 100 * $totalGoodAudioFrames / $totalAudioFrameCount]%"
puts -nonewline $outfile "\t FER: [expr double($totalAudioFrameCount - $totalGoodAudioFrames) / $totalAudioFrameCount]"
puts -nonewline $outfile "\tBits: $totalBitCount\tErrors: $totalBitErrorCount"

if {$totalBitCount != 0} {
      #puts -nonewline "\tOverall BER: [expr 100 * $totalBitErrorCount / $totalBitCount]%"        
   puts $outfile "\t Overall BER: [expr double($totalBitErrorCount) / $totalBitCount]"
} else {
   puts $outfile ""
}
close $outfile