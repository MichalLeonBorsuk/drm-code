# Reads status information from receiver and stores it into a binary file.
# Call with: tclsh recordall.tcl <freq in kHz>
# The frequency is optional

# Read common code and global variables
#source include.tcl
source SettingsRemoteMonitoring.tcl
package require rsciUtil
package require rsci
#source receptionhtml.tcl
#source psdplot.tcl
#source receptionkml.tcl
source receptionxml.tcl

package require udp

# Variables
set receivedAFS ""; # Received audio frame status


# server to output HTML
proc StartHTMLServer {port} {
	socket -server AcceptHTMLConnection $port
	puts "HTML server started on port $port."
}

proc AcceptHTMLConnection {socket addr port} {
	global rxIDCounters
	puts "\nConnection established with $addr:$port"
	fconfigure $socket -translation binary
	# read a line
	puts "HTML CONNECTION!!!!"
	gets $socket request
	puts "Request: $request"
	if {$request == "main"} {
		WriteReceptionHTML $socket [array names rxIDCounters]
	} elseif [regexp "^(\[a-zA-Z0-9_\]+) (\[a-zA-Z0-9_\]+)" $request dummy type rxID] {
		if {$type == "psd"} {
			puts "Writing PSD for $rxID"
			WritePSDPlot $socket $rxID
		}
	}
	close $socket
}


#server to output KML
proc StartKMLServer {port} {
	socket -server AcceptKMLConnection $port
	puts "KML server started on port $port."
}

proc AcceptKMLConnection {socket addr port} {
	global rxIDCounters
	puts "\nConnection established with $addr:$port"
	WriteReceptionKML $socket [array names rxIDCounters]
	close $socket
}

#server to output XML
proc StartXMLServer {port} {
	socket -server AcceptXMLConnection $port
	puts "XML server started on port $port."
}

proc AcceptXMLConnection {socket addr port} {
	global rxIDCounters
	puts "\nConnection established with $addr:$port"
	WriteReceptionXML $socket [array names rxIDCounters]
	close $socket
}


proc ConfigureStatusPort {} {

    # Configures connection for incoming data.
    # File id is returned

    global COLLECTOR_PORT

	set tty [udp_open $COLLECTOR_PORT]
	fconfigure $tty -buffering none -translation binary

	fconfigure $tty -blocking 1

    return $tty
}

# Shift a history array along by one and add a new value at the end
proc AddToHistory {varHistory value} {
  upvar $varHistory history 
  set length [llength $history]
  set history [concat [lrange $history 1 [expr $length - 1]] [list $value]]
}

proc InformationCollector {inDataId} {

    global SE127_VERSION 
	global rxIDCounters
    global HISTORY_LENGTH
    global MAX_LABEL_COUNT

    # This function gets called each time any receiver sends an RSI frame

    # Read 400 ms worth of information
    # Boy: There was an error reading the errorFlag... which crashed the scripts
    # occasionaly

    set errorFlag [GetTagInfo $inDataId tags binaryFrame]
    #if {[catch {set errorFlag [GetTagInfo $inDataId tags binaryFrame]} errorString]} {
    #  puts "caught error in tag decoding"
    #  puts stderr "Caught error in tag decoding:\n$errorString"
    #  return
    #}

    if {$errorFlag == -1} {
		puts "Receiver error!"
		return
    }	

	if [info exists tags(rinf)] {
		set rxID [string range $tags(rinf) 10 15]
	} else {
		# no id: can't do anything with it
		return
	}

	upvar receptionInfo$rxID receptionInfo

	#puts "Got a packet from $rxID"
 
	set rxIDCounters($rxID) 0

    #andrewm - trim tag items that should be present in every RSCI packet
    foreach t {rpir rpsd rmer rwmf rwmm rafs} {
      if [info exists receptionInfo($t)] {
        unset receptionInfo($t)
      }
    } 

    # trim station label if frequency changes
    if {[info exists receptionInfo(rfre)] && [info exists tags(rfre)]} {
      if {$receptionInfo(rfre) != $tags(rfre)} {
        if [info exists receptionInfo(label)] {
          #unset receptionInfo(label)
        }
      }
    }

    # Store info for later in minuteSummary
    foreach tagName [array names tags] {
		if {$tags($tagName) != ""} {
			set receptionInfo($tagName) $tags($tagName)
		}
	}
  #take median for rdbv value when there is more than one RF value
  if {$tags(rdbv) != ""} {
    set receptionInfo(rdbv) [lindex [CalcDistribution $tags(rdbv)] 2]
  }


    # Store extra info

    ### AFS ###
    # Process rafs string and update number of receiver frames noafs
    if {$tags(rafs) != ""} {

		# Here, we have received audio frames
		set receptionInfo(noafs) [string length $tags(rafs)]
    } else {

		# If we do not know the number of audioframes (as at the beginning of a reception),
		# set to default value
		if ![info exists receptionInfo(noafs)] {
			set receptionInfo(noafs) 10
		}

		# Append "2"s for for audio frame losses. Use the number of counted AFS of the previous TX frame
		for {set i 0} {$i< $receptionInfo(noafs) } {incr i} {
			append tags(rafs) "2"
		}
    }

    # Add current AFS to rafString
    #append receptionInfo(rafsTotal) $tags(rafs)


    ### RMER ###
    #set tagName "rmer"
    #if {$tags($tagName) != ""} {
	#append minuteSummary($tagName\Total) " $tags($tagName)"
    #}
    
    ### RWMF ###
    #set tagName "rwmf"
    #if {$tags($tagName) != ""} {
	#append minuteSummary($tagName\Total) " $tags($tagName)"
    #}

    ### RWMM ###
    #set tagName "rwmm"
    #if {$tags($tagName) != ""} {
	#append minuteSummary($tagName\Total) " $tags($tagName)"
    #}


    ### RDEL ###
    #set tagName "rdel"
    #if {$tags($tagName) != ""} {
	#foreach double $tags($tagName) {
	#	set percentage [lindex $double 0]
	#	set delay [lindex $double 1]
	#	append minuteSummary($tagName\Total_$percentage) " $delay"
	#}	
    #}

    ### RDOP ###
    #set tagName "rdop"
    #if {$tags($tagName) != ""} {
	#append minuteSummary($tagName\Total) " $tags($tagName)"		
    #}

    ### RDBV ###
    #set tagName "rdbv"

    ### FAC
    # Only decode if there is no problem
    if {[lindex $tags(rsta) 1] == 0} {
	set tagName "fac_"
	if {$tags($tagName) != ""} {
	    set facContentList [FACDecoder $tags($tagName)]
	    set referencedShortId [lindex $facContentList 10]
	    set receptionInfo($tagName$referencedShortId) $tags($tagName)
	} else {
	    set facContentList [list]
	}
    } else {
#    	puts "\nFAC Ignored."
	set facContentList "ignored."
    }
 
	# Get the selected service
	if [info exists receptionInfo(rser)] {
		set serviceSelected $receptionInfo(rser)
	} else {
		set serviceSelected 0
	}

 ### SDC ###



    # Only decode if there is no problem
    if {[lindex $tags(rsta) 2] == 0 && [info exists receptionInfo(sdc_)]} {

	set tagName "sdc_"
	if {[string length $tags($tagName)] > 0} {    

	    set sdcDeList [SDCDecoder $tags($tagName)]

		# Extract the service label
		if {[lsearch $sdcDeList 1_$serviceSelected] != -1} {
			set serviceLabelList [SDCExtractor $tags($tagName) 1_$serviceSelected]

			set serviceLabel [lindex $serviceLabelList 2]
			if {$serviceLabel != ""} {
				set receptionInfo(label) $serviceLabel
				set receptionInfo(labelCount) 0
			}
		}


	} else {
	    set sdcDeList [list]
	} 

	    
    } else {
#	puts "\nSDC Ingored!"
	set sdcDeList "ignored."
    }

    # Reap station label after none received for some time
    if [info exists receptionInfo(labelCount)] {
      incr receptionInfo(labelCount)
      if {$receptionInfo(labelCount) > $MAX_LABEL_COUNT} {
        unset receptionInfo(labelCount)
        if [info exists receptionInfo(label)] {
          unset receptionInfo(label)
        }
      }
    }

    # Update history tags
    foreach t {rdbv rwmm} {
      if ![info exists receptionInfo(hist_$t)] {
        # set receptionInfo(hist_$t) [lrepeat $HISTORY_LENGTH "?"] # lrepeat not in this tcl version?
          set receptionInfo(hist_$t) [list]
          for {set i 0} {$i < $HISTORY_LENGTH} {set i [expr $i + 1]} {
            lappend receptionInfo(hist_$t) "?"
          }
      }
      if [info exists receptionInfo($t)] {
        AddToHistory receptionInfo(hist_$t) $receptionInfo($t)
      } else {
        AddToHistory receptionInfo(hist_$t) "?"
      }
    }

    # Process BER tags
    #if {[llength $tags(rbp0)] == 2} {
	#set ber0Error [lindex $tags(rbp0) 0]
	#set ber0Total [lindex $tags(rbp0) 1]
	#set ber0Total [lindex $tags(rbp0) 0]
	#set ber0Error [lindex $tags(rbp0) 1]
    #} else {
    #   set ber0Error 0
    #   set ber0Total 1
    #}


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

}


			
#### Start ####


# Configure connection for incoming data

set statusPort [ConfigureStatusPort]
fileevent $statusPort readable [list InformationCollector $statusPort]

StartHTMLServer $HTML_PORT
StartKMLServer $KML_PORT
StartXMLServer $XML_PORT

while {1} {
   
    # Wait 5s
    set flag 0
    after 1000 {set flag 1}
    vwait flag

	# Reap any inactive receivers
	foreach rxID [array names rxIDCounters] {
		set rxIDCounters($rxID) [expr $rxIDCounters($rxID) + 1]
		if {$rxIDCounters($rxID) >= $RECEIVER_TIMEOUT} {
			unset rxIDCounters($rxID)
			unset receptionInfo$rxID
		}

		#set giffile [open "psd_$rxID.png" w]
		#WritePSDPlot $giffile $rxID
		#close $giffile

		# Use this to write a html file
#		set kmlfile [open "reception.kml" w]
#		WriteReceptionKML $kmlfile [array names rxIDCounters]
#		close $kmlfile
	}

	# Use this to write a html file
	#set htmlfile [open "reception.html" w]
	#WriteReceptionHTML $htmlfile [array names rxIDCounters]
	#close $htmlfile
    
}

