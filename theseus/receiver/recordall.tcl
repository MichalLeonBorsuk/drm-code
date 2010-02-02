# Reads status information from receiver and stores it into a binary file.
# Call with: tclsh recordall.tcl <freq in kHz>
# The frequency is optional


# Read common code and global variables
source settings.tcl

package require rsciUtil
package require rsci

# Include support for UDP
if {$CONTROL_PROTOCOL == "UDP" || $STATUS_PROTOCOL == "UDP"} {
	package require udp
}

# Variables
set receivedAFS ""; # Received audio frame status


# server to receive commands
proc StartServer {port} {
	socket -server AcceptConnection $port
	puts "Server started on port $port."
}

proc AcceptConnection {socket addr port} {
	fconfigure stdin -blocking 0
	fileevent $socket readable [list CommandReader $socket]
	puts "\nConnection established with $addr:$port"
}


proc ConfigureStatusPort {} {

    # Configures connection for incoming data.
    # File id is returned

	global DRM_RX_STATUS_IP_ADDRESS
    global STATUS_PORT
    global STATUS_PROTOCOL

#    set tty [open /dev/ttyb r+]
#    fconfigure $tty -mode 115200,n,8,1 -translation binary

	# For receivers that support the UDP protocol
	if {$STATUS_PROTOCOL == "UDP"} {
		set tty [udp_open $STATUS_PORT]
		fconfigure $tty -buffering none -translation binary
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

    # For receivers using the UDP protocol for status
    if {$CONTROL_PROTOCOL == "UDP"} {
		set tty [udp_open]
		fconfigure $tty -remote [list $DRM_RX_CTRL_IP_ADDRESS $CONTROL_PORT]
    } 

    # For receivers using the UDP protocol for status
    if {$CONTROL_PROTOCOL == "TCP"} {
	    set tty [socket $DRM_RX_CTRL_IP_ADDRESS $CONTROL_PORT]
    }

    fconfigure $tty -translation binary 
    fconfigure $tty -blocking 1

    return $tty
}

proc MakeBandscanStats {varSummary} {
	upvar $varSummary bsSummary

	set startFreq $bsSummary(startFreq)
	set stopFreq $bsSummary(stopFreq)
	set freqStep $bsSummary(freqStep)
	set startTime $bsSummary(time)

	set bsData [list $startTime $startFreq $stopFreq $freqStep]
	for {set freq $startFreq} {$freq <= $stopFreq} {set freq [expr $freq + $freqStep]} {		
		if [info exists bsSummary($freq)] {
			set distList [CalcDistribution $bsSummary($freq)]
			set rdbvMed [lindex $distList 2]			
			lappend bsData $rdbvMed
		} else {
			lappend bsData ""
		}
	}

	return $bsData
}

proc CommandReader {socket} {

	global currentStatus frequency mode modelogFileId binaryLogFileId
	global REAL_TIME_LOG
	global BINARY_RECORDING
	global COMMAND_PORT
	global IQ_RECORD
	global previousProfiles
	global bsSummary
	global operation

	# Read command
	gets $socket line

	# Close socket if connection breaks
	if [eof $socket] {
		catch {close $socket} 
		PutLog "Control connection lost"
	}

	PutLog "\nCommand received: $line"
	# Start; Read frequency from command
        if {[regexp {start ([A-Z]*)( )*([0-9]+)(\.0)*( )*([a-z]*)} $line match demod dummy arg1 dummy dummy arg2]} {
		# Activate receiver
		CmdActivate $COMMAND_PORT "1" 0x0002 0x0001

		#andrewm 19/01/07, 
		# Make new mode public
		if {$demod == "DRM"} {
			set mode "drm_"
		} else { # AM or bandscan
			set mode "am__"
		}

		if {$demod == "BS"} {
			set operation "BS"
		} else {
			set operation "RX"
		}

		# Make new frequency public
		set frequency $arg1
		
		# Set frequency on receiver (starting freq in case of bandscan)
		CmdSetFrequency $COMMAND_PORT $frequency $mode 0x0002 0x0001


		# Start real time log
		if {$REAL_TIME_LOG == 1} {
		    set date [clock format [clock seconds] -format "%Y-%m-%d" -gmt 1]
		    set fileName [clock format [clock seconds] -format "%Y-%m-%d_%H-%M-%S_$frequency.log" -gmt 1]
		    set fileName [file join log $date $fileName]
		    file mkdir [file dirname $fileName]
		    set logFileId [open $fileName w]
		    puts "\nFrame-based data log started. Watch your diskspace!"
		}				 

		# Start IQ file recording if the option is set in settings.tcl
		if {$IQ_RECORD == 1} {
		   CmdStartRecording $COMMAND_PORT "i" 0x0002 0x0001
		}

		# Start recording of binary recording
		if {$BINARY_RECORDING == 1} {
		    set date [clock format [clock seconds] -format "%Y-%m-%d" -gmt 1]
		    set fileName [clock format [clock seconds] -format "%Y-%m-%d_%H-%M-%S_$frequency.bin" -gmt 1]
		    set fileName [file join "bin" $date $fileName]
		    file mkdir [file dirname $fileName]
		    set binaryLogFileId [open $fileName w]
		    fconfigure $binaryLogFileId -translation binary 
		    puts "\nBinary data log started. Watch your diskspace!"
		}	

		# Start recording of specific profile at the receiver
		  set profile ""
	        regexp {([a-z]+)} $arg2 match profile

		if {[info exists previousProfiles]} {	# need to check whether the profiles we're asked to record now are the same as before
			if {![string equal $previousProfiles $profile]} {	# need to stop only those that aren't the same as before
				set profilesList [split $profile {}]
				set previousProfilesList [split $previousProfiles {}]

#				puts "previous: $previousProfilesList, current: $profilesList"
				set stopProfilesList [list]
			    	foreach prePro $previousProfilesList {
					if {[lsearch $profilesList $prePro] == -1} {
						lappend stopProfilesList $prePro
					}
				}

				set stopProfiles [join $stopProfilesList ""]

#				puts "stop: $stopProfiles"
				CmdStopRecording $COMMAND_PORT $stopProfiles 0x0002 0x0001
			}
		}

		if {![string equal $profile ""]} {
			CmdStartRecording $COMMAND_PORT $profile 0x0002 0x0001
			puts "\nRecording of datafile started."
		}

		set previousProfiles $profile

		# Wait 2 seconds
		set var 0
		after 2000 {set var 1}
		vwait var
	}

      if {[regexp {BandScanInit ([0-9]+)( )([0-9]+)( )([0-9]+)( )([0-9]+)} $line match startTime dummy startFreq dummy stopFreq dummy freqStep]} {
		if [info exists bsSummary] {
			unset bsSummary
	 	}
		puts "starting bandscan from $startFreq to $stopFreq"
		set bsSummary(time) $startTime
		set bsSummary(startFreq) $startFreq
		set bsSummary(stopFreq) $stopFreq
		set bsSummary(freqStep) $freqStep
	}

      if {[regexp {BandScanEnd} $line match]} {
	    # Store bandscan summary
	    array set currentBandscan [array get bsSummary]

	    # Reset bandscan Summary
          if [info exists bsSummary] {
		    unset bsSummary
          }
	    set operation "RX"
          WriteBandscanFile [MakeBandscanStats currentBandscan]
	}

	# Stop
	if {[regexp {stop.*} $line match arg1]} {

		# Set global variable to stop writing to file
		unset frequency
		unset mode
            set operation "RX"

		
		# Close log file if it exists
		if [info exists logFileId] {
		    close $logFileId
		    unset logFileId
		    puts "Frame-based data log stopped."
		}

		# Stop IQ file recording if the option is set in settings.tcl
		CmdStopRecording $COMMAND_PORT "abcdqri" 0x0002 0x0001
		puts "Recording at receiver stopped (if there was any)."
		
		# Close binary file if it exists
		if [info exists binaryLogFileId] {
		    close $binaryLogFileId
		    unset binaryLogFileId
		    puts "Binary data recording stopped."
		}

		# Deactivate receiver
		CmdActivate $COMMAND_PORT "0" 0x0002 0x0001


		puts "Recording of datafile stopped"
	}
		

	#Reset
	if {[regexp {reset.*} $line match arg1]} {
		CmdStopRecording $COMMAND_PORT "abcdqri" 0x0002 0x0001
		puts "Reset: Recording at receiver stopped (if there was any)."
	}


	# Exit command
	if {[regexp {exit.*} $line match arg]} {
		
		# Exit
		exit
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
    global mode
    global operation
	
    global bsSummary 

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
	set minuteSummary(noafs) [string length $tags(rafs)]
    } else {

	# If we do not know the number of audioframes (as at the beginning of a reception),
	# set to default value
	if ![info exists minuteSummary(noafs)] {
	    set minuteSummary(noafs) 10
	}

	# Append "2"s for for audio frame losses. Use the number of counted AFS of the previous TX frame
	for {set i 0} {$i<$minuteSummary(noafs)} {incr i} {
	    append tags(rafs) "2"
	}
    }

    # Add current AFS to rafString
    append minuteSummary(rafsTotal) $tags(rafs)


    ### RMER ###
    set tagName "rmer"
    if {$tags($tagName) != ""} {
	append minuteSummary($tagName\Total) " $tags($tagName)"
    }
    
    ### RWMF ###
    set tagName "rwmf"
    if {$tags($tagName) != ""} {
	append minuteSummary($tagName\Total) " $tags($tagName)"
    }

    ### RWMM ###
    set tagName "rwmm"
    if {$tags($tagName) != ""} {
	append minuteSummary($tagName\Total) " $tags($tagName)"
    }


    ### RDEL ###
    set tagName "rdel"
    if {$tags($tagName) != ""} {
	foreach double $tags($tagName) {
		set percentage [lindex $double 0]
		set delay [lindex $double 1]
		append minuteSummary($tagName\Total_$percentage) " $delay"
	}	
    }

    ### RDOP ###
    set tagName "rdop"
    if {$tags($tagName) != ""} {
	append minuteSummary($tagName\Total) " $tags($tagName)"		
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
		append minuteSummary($tagName\Total) " $tags($tagName)"

		if {[info exists operation] && [info exists tags(rfre)]} {
			if {$operation == "BS"} {
				if {[string length $tags(rfre)] > 0} {
					set freq [expr round($tags(rfre)/1000)]
					append bsSummary($freq) " $tags($tagName)"
				}
			}
		}
    }
 
    ### FAC
    # Only decode if there is no problem
    if {[lindex $tags(rsta) 1] == 0} {
	set tagName "fac_"
	if {[info exists tags($tagName)]} {
		if {$tags($tagName) != ""} {
		    set facContentList [FACDecoder $tags($tagName)]
		    set referencedShortId [lindex $facContentList 10]
		    set minuteSummary($tagName$referencedShortId) $tags($tagName)
		} else {
		    set facContentList [list]
		}
	} else {
		set facContentList [list]
	}
    } else {
#    	puts "\nFAC Ignored."
	set facContentList "ignored."
    }
 
 ### SDC ###

    # Only decode if there is no problem
    if {[lindex $tags(rsta) 2] == 0} {
	
	set tagName "sdc_"
	if {[info exists tags($tagName)]} {
	if {[string length $tags($tagName)] > 0} {    
	    
	    set sdcDeList [SDCDecoder $tags($tagName)]
	    
	    # Init list which contains stored data entity numbers
	    if ![info exists minuteSummary(deList)] {
		set minuteSummary(deList) [list]
	    }
	    
	    # Check whether the specific SDC data entities have already been stored
	    # Store SDC if a new SDC is found
	    foreach sdcDe $sdcDeList {
		
		if {[lsearch $minuteSummary(deList) $sdcDe] == -1} {
			if ![info exists minuteSummary($tagName\Total)] {
			    # OPH: Put the AFS index byte in the first time only
		          set minuteSummary($tagName\Total) [string range $tags($tagName) 0 0]
		      }
		    append minuteSummary($tagName\Total) [SDCCompressor $tags($tagName)]
		    set minuteSummary(deList) [concat $minuteSummary(deList) $sdcDeList]
		}
		
		# Display content
		#	foreach sdcDe $sdcDeList {
		#    puts \n[SDCExtractor $minuteSummary($tagName\Total)) $sdcDe]
		#	}
		
	    }

	    set sdc0_0 [SDCExtractor $minuteSummary($tagName\Total) 0_0]

	} else {
	    set sdcDeList [list]
	    set sdc0_0 "noSDC"
	}
	} else {
	    set sdcDeList [list]
	    set sdc0_0 "noSDC"
	} 

	    
    } else {
#	puts "\nSDC Ingored!"
	set sdcDeList "ignored."
	set sdc0_0 "igSDC"
    }

    # Process BER tags
    if {[llength $tags(rbp0)] == 2} {
	set ber0Error [lindex $tags(rbp0) 0]
	set ber0Total [lindex $tags(rbp0) 1]
	#set ber0Total [lindex $tags(rbp0) 0]
	#set ber0Error [lindex $tags(rbp0) 1]
    } else {
       set ber0Error 0
       set ber0Total 1
    }


    # Process private BBC tags
    if [info exists tags(Bstr)] {
	puts \n\n$tags(Bstr)\n
    }

    if [info exists tags(Bdia)] {

	binary scan $tags(Bdia) "cB8SScccB8a4a4a4a4a4a4a4a4a4a4a4a4a4a4a4a4Sca4" \
	       dummy rxStat dspStat mlcStat audioStat cpuLoad drmMode stickyFlags \
	       minFP maxFP minTCS maxTCS minSe maxSe \
	       csiMean csiPeak csiPos csiPk2Mn \
	       cirPeak cirPos \
	       mass1 mass1Pos mass2 doppler \
	       cwPos attenuation bufferFP

	# Format floats
#	set minFP [format "%.2e" $minFP]
#	set maxFP [format "%.2e" $maxFP]
#	set minTCS [format "%.2e" $minTCS]
#	set maxTCS [format "%.2e" $maxTCS]
#	set minSe [format "%.2e" $minSe]
#	set maxSe [format "%.2e" $maxSe]

#	set csiMean [format "%.2e" $csiMean]
#	set csiPeak [format "%.2e" $csiPeak]
#	set csiPos [format "%.2e" $csiPos]
#	set csiPk2Mn [format "%.2e" $csiPk2Mn]

#	set cirPeak [format "%.2e" $cirPeak]
#	set cirPos [format "%.2e" $cirPos]

#	set mass1 [format "%.2e" $mass1]
#	set mass1Pos [format "%.2e" $mass1Pos]

	set minFP [format "%.2e" [IEEE2float $minFP 0]] 
	set maxFP [format "%.2e" [IEEE2float $maxFP 0]] 
	set minTCS [format "%.2e" [IEEE2float $minTCS 0]] 
	set maxTCS [format "%.2e" [IEEE2float $maxTCS 0]] 
	set minSe [format "%.2e" [IEEE2float $minSe 0]] 
	set maxSe [format "%.2e" [IEEE2float $maxSe 0]] 

	set csiMean [format "%.2e" [IEEE2float $csiMean 0]] 
	set csiPeak [format "%.2e" [IEEE2float $csiPeak 0]] 
	set csiPos [format "%.2e" [IEEE2float $csiPos 0]] 
	set csiPk2Mn [format "%.2e" [IEEE2float $csiPk2Mn 0]] 

	set cirPeak [format "%.2e" [IEEE2float $cirPeak 0]] 
	set cirPos [format "%.2e" [IEEE2float $cirPos 0]]
	
	set mass1 [format "%.2e" [IEEE2float $mass1 0]] 
	set mass1Pos [format "%.2e" [IEEE2float $mass1Pos 0]] 
	set mass2 [format "%.2e" [IEEE2float $mass2 0]] 

	set doppler [format "%.2e" [IEEE2float $doppler 0]] 

	set bufferFP [format "%.2e" [IEEE2float $bufferFP 0]] 


    } else {

	set rxStat ""
	set dspStat ""
	set mlcStat ""
	set audioStat ""
	set cpuLoad ""
	set drmMode ""
	set stickyFlags ""
	set minFP ""
	set maxFP ""
	set minTCS ""
	set maxTCS ""
	set minSe ""
	set maxSe ""
	set csiMean ""
	set csiPeak ""
	set csiPos ""
	set csiPk2Mn ""
	set cirPeak ""
	set cirPos ""
	set mass1 ""
	set mass1Pos ""
	set mass2 ""
	set doppler ""
	set cwPos ""
	set attenuation ""
	set bufferFP ""
    }



    # Write to logfile
    if [info exists logFileId] {
    
	set audioFlag [expr [lindex $tags(rsta) 3]*10]

	# General data from every receiver	
	set outString [clock format [clock seconds] -gmt 1 -format "%H:%M:%S"]\t
	append outString $tags(dlfc)\t$globalFrameCounter\t$tags(rfre)\t$tags(robm)\t$facContentList\t$sdc0_0\t
	append outString [CalcDistribution $tags(rdbv)]\t
	append outString [CalcDistribution $tags(rwmf)]\t
	append outString $tags(rsta)\t$tags(rafs)\t$ber0Error\t$ber0Total\t$tags(rdel)\t$tags(rinf)\t$tags(rdop)\t

	# Data from BBC receiver
	append outString $rxStat\t$dspStat\t$mlcStat\t$audioStat\t$cpuLoad\t$drmMode\t$stickyFlags\t
	append outString $minFP\t$maxFP\t$minTCS\t$maxTCS\t$minSe\t$maxSe\t
	append outString $csiMean\t$csiPeak\t$csiPos\t$csiPk2Mn\t$cirPeak\t$cirPos\t
	append outString $mass1\t$mass1Pos\t$mass2\t$doppler\t$cwPos\t$attenuation\t$bufferFP

	puts $logFileId $outString
	flush $logFileId
    }


    # Delete minuteSummary if no data is recorded (and therefore the 'frequency' variable is not defined)
    # When the data is recorded, the minuteSummary is reset by AssembleDataBlock
    if ![info exists frequency] {
    	unset minuteSummary
    }

    # ShowTags tags
    # Show incoming audio  frames
    # puts -nonewline $tags(rafs)
    #flush stdout
    if {[expr $tags(dlfc)%5] == 0} {
	#puts ""
    } 

    puts -nonewline "\r                                                                                 "
    puts -nonewline "                                                                                   \r"
    puts -nonewline [clock format [clock seconds] -format "%H:%M:%S"]
    puts -nonewline "\tDLFC: $tags(dlfc)"
    puts -nonewline "\tRDBV:[lindex [CalcDistribution $tags(rdbv)] 2] dBuV"
    puts -nonewline "\tRWMF:[lindex [CalcDistribution $tags(rwmf)] 2] "
    puts -nonewline "RWMM:[lindex [CalcDistribution $tags(rwmm)] 2] " 
    puts -nonewline "RMER:[lindex [CalcDistribution $tags(rmer)] 2] dB"
    puts -nonewline "\tRSTA: $tags(rsta)"
    puts -nonewline "\tAFS: $tags(rafs)"
    puts -nonewline "\tRDEL: $tags(rdel)"
    puts -nonewline "\tRDOP: $tags(rdop)"

    catch {
 #   	puts -nonewline "\t[format "%.1f" $doppler]Hz"
 #   	puts -nonewline "\t[format "%.1f" $mass2]ms"
 #   	puts -nonewline "\t[format "%.1f" $cirPos]ms"
    	puts -nonewline "\t[format "%.1f" $csiPk2Mn]dB"
    	puts -nonewline "\t[format "%.1f" $csiPos]kHz"
	puts -nonewline "\tBER: $ber0Error/$ber0Total/[format "%.2e" [expr 1.0*$ber0Error/$ber0Total]]"
    }
    
    
#    puts -nonewline "\tFAC: $facContentList"
#    puts -nonewline "\tSDC: $sdcDeList"


    flush stdout

    # Beep if audio error occurs. Includes newline.
    if {$AFS_ERROR_BEEP == 1 && [info exists logFileId]} {
	if [regexp {[12]} $tags(rafs)] {
	    puts [binary format c 0x07]
	}
    }

}

proc AssembleBandscan {ttyOut collectionTime startFrequency endFrequency frequencyStep} {

}

proc AssembleDatablock {ttyOut collectionTime frequency mode} {
   
    # Waits <collection time> seconds and assembles data for the current datablock

    global minuteSummary

    # Memorise start time
    set timeStamp [clock seconds]
    
    # Collect data during <collection time> seconds
    set targetTime [expr $timeStamp + $collectionTime]

    # Switch receiver to <frequency> and choose appropriate mode. Source is 0x0002 abd destination is 0x0001
    # Only do this if the frequency has changed

    # Wait until receiver frequency is known
    while {![info exists minuteSummary(rfre)]} {
	set flag 0
	after 100 {set flag 1}
	vwait flag
    }

    # Set frequency on receiver
    if [info exists minuteSummary(rfre)] {
	if {$minuteSummary(rfre) != [expr $frequency*1000]} {
	    CmdSetFrequency $ttyOut $frequency $mode 0x0002 0x0001
	}
    } else {
	CmdSetFrequency $ttyOut $frequency $mode 0x0002 0x0001
    }

    # Set mode on receiver
    if [info exists minuteSummary(rdmo)] {
	if {$minuteSummary(rdmo) != $mode} {
		CmdSetDemodulation $ttyOut $mode 0x0002 0x0001
	}
    } else {
	CmdSetDemodulation $ttyOut $mode 0x0002 0x0001
    }


    while {[clock seconds] <= $targetTime} {

	#puts -nonewline "\r                                       "
	#puts -nonewline \r[clock format [clock seconds]]
	#if [info exists minuteSummary(time)] {
	#    puts -nonewline "\t$minuteSummary(time)"
	#}
	#flush stdout

	set flag 0
	after 1000 {set flag 1}
	vwait flag
    }    

    # Return in case that no data was collected
    if ![info exists minuteSummary] {	
		return -1
    }

    # Store current summary
    array set currentSummary [array get minuteSummary]

    # Reset minute Summary
    unset minuteSummary
    	
    # ShowTags currentSummary

    ########### Build datablock ############
    set dataBlock [list]

    # Add Timestamp TS
    set tagId 0x00
    set itemSize 4
    lappend dataBlock [list $tagId $itemSize [clock seconds]]

    # Reception frequency FREQ
    set tagId 0x01
    set itemSize 4
    set tagName "rfre"
    if [info exists currentSummary($tagName)] {
	if {[string length $currentSummary($tagName)] > 0} {
	    lappend dataBlock [list $tagId $itemSize $currentSummary($tagName)]
	}
    }

    # Robustness mode ROBM
    set tagId 0x02
    set itemSize 1
    set tagName "robm"
    if [info exists currentSummary($tagName)] {
	lappend dataBlock [list $tagId $itemSize $currentSummary($tagName)]
    }

    # FAC
    set tagId 0x03
    set itemSize 1
    set tagName "fac_"

    # Go though facs of all potential short ids...
    for {set id 0} {$id<4} {incr id} {
	if [info exists currentSummary($tagName$id)] {

	    # Get binary content
	    set fac $currentSummary($tagName$id)

	    # Add each of the FAC bytes indiviudally	    
	    set facByteList [list]
	    for {set i 0} {$i<[string length $fac]} {incr i} {
		binary scan [string range $fac $i $i] "c" facByte
		set facByte [expr ($facByte+0x100)%0x100]
		lappend facByteList $facByte
	    }
	    # Add to datablock
	    lappend dataBlock [list [expr $tagId] $itemSize $facByteList]
	}
    }

    # andrewm - demod mode
    # Robustness mode RDMO
    set tagId 0x04
    set itemSize 1
    set tagName "rdmo"
    if [info exists currentSummary($tagName)] {
	    set demod 0
	    if {$currentSummary($tagName) == "drm_"} {
		set demod 0
	    } elseif {$currentSummary($tagName) == "am__"} {
		set demod 1
	    } elseif {$currentSummary($tagName) == "usb_"} {
		set demod 2
	    } elseif {$currentSummary($tagName) == "lsb_"} {
	    	set demod 3
          } elseif {$currentSummary($tagName) == "sam_"} {
	    	set demod 4
	    }
	lappend dataBlock [list $tagId $itemSize $demod]
    }


    # Selected service RSER
    set tagId 0x07
    set itemSize 1
    set tagName "rser"
    if [info exists currentSummary($tagName)] {
	lappend dataBlock [list $tagId $itemSize $currentSummary($tagName)]
    }

    # Bandwidth
    set tagId 0x08
    set itemSize 2
    set tagName "rbw_"
    if [info exists currentSummary($tagName)] {
	lappend dataBlock [list $tagId $itemSize [expr round($currentSummary($tagName)*1000)]]
    }   


    # SDC
    set tagId 0x09
    set itemSize 1
    set tagName "sdc_Total"
    if [info exists currentSummary($tagName)] {

	#  Binary content
	set sdc $currentSummary($tagName)
	
	# Add each of the SDC bytes indiviudally	    
	set sdcByteList [list]
	for {set i 0} {$i<[string length $sdc]} {incr i} {
	    binary scan [string range $sdc $i $i] "c" sdcByte
	    set sdcByte [expr ($sdcByte+0x100)%0x100]
	    lappend sdcByteList $sdcByte
	}
	# Add to datablock
	lappend dataBlock [list $tagId $itemSize $sdcByteList]
    
    }

    # RINF
    set tagId 0x0a
    set itemSize 1
    set tagName "rinf"
    if [info exists currentSummary($tagName)] {

	#  Binary content
	set rinfContent $currentSummary($tagName)
	
	# Add each of the SDC bytes indiviudally	    
	set byteList [list]
	for {set i 0} {$i<[string length $rinfContent]} {incr i} {
	    binary scan [string range $rinfContent $i $i] "c" byte
	    set byte [expr ($byte+0x100)%0x100]
	    lappend byteList $byte
	}
	# Add to datablock
	lappend dataBlock [list $tagId $itemSize $byteList]
    
    }

    

    # Generate audio statistics and append to data block list
    set audioStats [GenerateAudioStatEntities $currentSummary(rafsTotal)]
    set dataBlock [concat $dataBlock $audioStats]

    # Number of audio frames per transmission frame
    set tagId 0x14
    set itemSize 1
    set tagName "noafs"
    lappend dataBlock [list $tagId $itemSize $currentSummary($tagName)]

    # Number of signal strength measurements
    set tagId 0x30
    set itemSize 2
    set tagName "rdbvTotal"
    if [info exists currentSummary($tagName)] {
	lappend dataBlock [list $tagId $itemSize [llength $currentSummary($tagName)]]
    }   
    
    # Signal strength statistics S0
    set tagId 0x33
    set itemSize 1
    set tagName "rdbvTotal"
    if [info exists currentSummary($tagName)] {

	# Calculate distribution
	set distList [CalcDistribution $currentSummary($tagName)]

	# Add offset of 50
	set distList50 [list]
	foreach distValue $distList {
	    lappend distList50 [expr $distValue+50]
	}
	
	# Append to data block
	lappend dataBlock [list $tagId $itemSize $distList50]
    }   



    # Number of MER measurements
    # Note that for version 3 of se127, the MER is in fact rwmf
    set tagId 0x36
    set itemSize 2
    set tagName "rwmfTotal"
    if [info exists currentSummary($tagName)] {
	lappend dataBlock [list $tagId $itemSize [llength $currentSummary($tagName)]]
    }   

    # Modulation Error Rate MER
    set tagId 0x37
    set itemSize 1
    set tagName "rwmfTotal"
    if [info exists currentSummary($tagName)] {

	# Calculate distribution
	set distList [CalcDistribution $currentSummary($tagName)]

	# Add offset of 50
	set distList50 [list]
	foreach distValue $distList {
	    lappend distList50 [expr $distValue+50]
	}
	
	# Append to data block
	lappend dataBlock [list $tagId $itemSize $distList50]
    }   

    # Fading rate
    set tagId 0x38
    set itemSize 2
    set tagName "rdbvTotal"
    if [info exists currentSummary($tagName)] {
	set fadingRate [GetFadingrate $currentSummary($tagName)]
	lappend dataBlock [list $tagId $itemSize $fadingRate]
    }   

    # Delay 90 based on rdel
    set tagId 0x50
    set itemSize 1
    set tagName "rdelTotal_90"
    if [info exists currentSummary($tagName)] {

	# Multiply with 10
	set list10 [list]
	foreach value $currentSummary($tagName) {
	    lappend list10 [expr int($value*10)]
	}

	# Calculate distribution
	set distList [CalcDistribution $list10]

	
	# Append to data block
	lappend dataBlock [list $tagId $itemSize $distList]
    }   

    # Delay 95 based on rdel
    set tagId 0x51
    set itemSize 1
    set tagName "rdelTotal_95"
    if [info exists currentSummary($tagName)] {

	# Multiply with 10
	set list10 [list]
	foreach value $currentSummary($tagName) {
	    lappend list10 [expr int($value*10)]
	}

	# Calculate distribution
	set distList [CalcDistribution $list10]

	
	# Append to data block
	lappend dataBlock [list $tagId $itemSize $distList]
    }   

    # Delay 99 based on rdel
    set tagId 0x52
    set itemSize 1
    set tagName "rdelTotal_99"
    if [info exists currentSummary($tagName)] {

	# Multiply with 10
	set list10 [list]
	foreach value $currentSummary($tagName) {
	    lappend list10 [expr int($value*10)]
	}

	# Calculate distribution
	set distList [CalcDistribution $list10]

	
	# Append to data block
	lappend dataBlock [list $tagId $itemSize $distList]
    }   

    # Experimental BBC doppler
    set tagId 0x53
    set itemSize 1
    set tagName "rdopTotal"
    if [info exists currentSummary($tagName)] {

	# Multiply with 10
	set list10 [list]
	foreach value $currentSummary($tagName) {
	    lappend list10 [expr int($value*10)]
	}

	# Calculate distribution
	set distList [CalcDistribution $list10]

	
	# Append to data block
	lappend dataBlock [list $tagId $itemSize $distList]
    }   

    puts $dataBlock\n
    return $dataBlock
}

proc GenerateAudioStatEntities {currentAfs} {

    # Generates the part of the data block that contains the audio statistics
    # Return list of audio block description

    # Determine number of audio frames in first audio entity
    set blockLength [string length $currentAfs]
    
    # Init variables
    for {set i 0} {$i<=2} {incr i} {
	set NF($i) 0
    }
    set RLList [list]
    
    # Go through content of current data block
    while {[string length $currentAfs] > 0} {
	
	# Chop off first block of equal bits (0, 1, 2 or 3)
	set chopResult [ChopAFS $currentAfs]
	
	# Extract result
	set type [lindex $chopResult 0]
	set length [lindex $chopResult 1]
	set currentAfs [lindex $chopResult 2]; # Remaining string
	
	# puts \t$type\t$length
	
	# Update counters
	
	# Update Number of frames (NF) counters
	incr NF($type) $length
	
	# Run-length list
	lappend RLList [list [expr 0x20+$type] $length]
	
    }
    
    # Size in bytes
    set itemSize 2
    
    # Prepare data block with AFS content
    set dataBlock [list] 
   
    # Add NF data entities to data block
    set indexList [lsort [array names NF]]
    foreach index $indexList {
	
	# Extract parts
	set ID [expr 0x10+$index]
	set data $NF($index)
	# puts $ID\t$data
	
	# Append to data block
	lappend dataBlock [list $ID $itemSize $data]
    }
    
    # Add RL data entities to data block
    foreach RLpair $RLList {
	
	# Extract parts
	set ID [lindex $RLpair 0]
	set data [lindex $RLpair 1]
	# puts $ID\t$data
	
	# Append to data list as 16 bit data field
	lappend dataBlock [list $ID $itemSize $data]
    }

    return $dataBlock
}    
   
proc GetFadingrate {rdbvList} {

    # Calculates the fading rate (fades per minute) from the list of rdbv values

    global MIN_FADING_DEPTH

    set oldRdbvValue -100
    set fadingCounter 0
    # OPH - crashed because fadingMaxCounted wasn't defined 
    # (must have been spurious rdbv value < -100, possibly because of rx crash)
    set fadingMaxCounted 0
    set maxLevel -100

    foreach rdbvValue $rdbvList {

	#Update fading rate
	if {$rdbvValue > $oldRdbvValue} {
	    
	    set maxLevel $rdbvValue
	    set fadingMaxCounted 0
	} else {
	    
	    if {$fadingMaxCounted != 1 && $rdbvValue <= [expr $maxLevel-$MIN_FADING_DEPTH]} {
		incr fadingCounter
		set fadingMaxCounted 1
	    }
	}
	
#	puts $rdbvValue\t$oldRdbvValue\t$maxLevel\t$fadingCounter

	set oldRdbvValue $rdbvValue
    }

    return $fadingCounter
}




proc ChopAFS {afsString} {

	# Extract first block of equal afs bits.
	# Returns list [type length remainder]

	# First bit is the type
	set type [string range $afsString 0 0]

	set length 0
	while {[string range $afsString 0 0] == $type} {
		set afsString [string range $afsString 1 end]
		incr length
	}

	return [list $type $length $afsString]
}


			
#### Start ####


# Init currentStatus
if {[string length [lindex $argv 0]] > 0} {
    set frequency [lindex $argv 0]
}

# Start TCP/IP server
StartServer $TCP_PORT_RECORDER

# Configure connection for incoming data
set statusPort [ConfigureStatusPort]
fileevent $statusPort readable [list InformationCollector $statusPort] 

# Configure conenction for outgoing data
set COMMAND_PORT [ConfigureCmdPort]

#CmdSetFrequency $COMMAND_PORT 243 "am__" 0x01 0x02

#CmdSetAMFilter $COMMAND_PORT 5 "upper" 0x01 0x02


# Needed for periodic update of the receiver time
set startTime [clock seconds]

while {1} {

    # Set receiver time if configured. Update rate is 24 h
    set currentTime [clock seconds]
    if {$ADJUST_RECEIVER_TIME && [expr ($currentTime-$startTime)%86400] == 0} {

	    set currentDate [clock format [clock seconds] -format "%Y-%m-%d" -gmt 1]
	    set currenthhmmss [clock format [clock seconds] -format "%H:%M:%S" -gmt 1]
    	    CmdSetTime $COMMAND_PORT $currentDate $currenthhmmss 1 0x01 0x02
	
            puts "\nReceiver time adjusted at $currentDate $currenthhmmss"
    }

       # Call Reader if the frequency is set
      if [info exists frequency] {
	
        # Get current second of time and omit leading '0'
        set currentSec [clock format [clock seconds] -format "%S" -gmt 1]
        if [regexp {^0} $currentSec match] {
	      set currentSec [string range $currentSec 1 1]
        }		       

        # Collection time in seconds
        set collectionTime [expr round($DATA_BLOCK_LENGTH-$currentSec)]
	
        # Read <collectionTime> of data from receiver. Returns list with contents of the data block
        set dataBlock [AssembleDatablock $COMMAND_PORT $collectionTime $frequency $mode]

        # Write total data block (tdb) to disk 
        if {$dataBlock != -1} {
          WriteDataBlock $dataBlock "dt2" 	
        }
    
        # Delete current AFSString
        #set receivedAFS ""
    }
   
    # Wait 100 ms
    set flag 0
    after 1000 {set flag 1}
    vwait flag
    
}

