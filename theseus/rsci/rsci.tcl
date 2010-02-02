
package provide rsci 1.0

if ![info exists SE127_PROTOCOL] {
    #puts "SE127_PROTOCOL undefined, defaulting to DCP!\n\r"
	set SE127_PROTOCOL DCP
}

if {$SE127_PROTOCOL == "DCP"} {
	package require dcp
	namespace import dcp::*
}

if {$SE127_PROTOCOL == "IPIO"} {
	package require ipio
	namespace import ipio::*
}

proc DisplayData {dataString} {

#   puts $dataString
}

proc ProcessTagContent {tagContent tagsVar} {

    # Expects binary string with tag content and returns array <tags>
    # which contains alls TAGS reference in the string

    upvar $tagsVar tags
    global SE127_VERSION 

    set pointer 0;
    set contentLength [string length $tagContent]

#    set outString ""
#    for {set i 0} {$i<=[string length $tagContent]} {incr i} {
#	binary scan [string range $tagContent $i $i] "c" byte
#	set byte [expr ($byte+0x100)%0x100]
#	append outString [format "%02x " $byte]#
#	if {[expr $i%16] == 15} {
#	    append outString "\n"
#	}
#    }
#    DisplayData $outString

    # RDEL and RDOP are currently not implemented in all receivers. Create an empty tag which is
    # overridden in case that the receiver provides it
    set tags(rdel) ""
    set tags(rdop) ""
    set tags(rwmf) ""
    set tags(rwmm) ""

    # Go through individual tags
    while {$pointer != $contentLength} {

	# Tag name
	set tagName [string range $tagContent $pointer [expr $pointer+3]]

	# If the tag name has illegal characters, return with -1 error
	if ![regexp {[a-zA-Z0-9_]} $tagName] {
		puts "Tag name $tagName is illegal. Skipping packet"
		return -1
	}

	# Data length in bytes
	binary scan [string range $tagContent [expr $pointer+4] [expr $pointer+7]] "I" dataBitSize
	set byteLength [expr $dataBitSize/8]

	# Output length
	DisplayData "      $tagName\t$byteLength"

	# Treat empty tags
	if {$byteLength == 0} {
	    set tags($tagName) ""

	    # Advance pointer
	    set pointer [expr $pointer+8+$byteLength]

	    continue
	}

	# Treat tags with data
	set data [string range $tagContent [expr $pointer+8] [expr $pointer+7+$byteLength]]

	# Advance pointer
	set pointer [expr $pointer+8+$byteLength]

	if {$tagName == "dlfc"} {
	    binary scan $data "I"  tags($tagName)
	    continue
	}
	
	if {$tagName == "robm"} {
	    binary scan $data "c" tags($tagName) 
	    set tags($tagName) [expr ($tags($tagName) + 0x100)%0x100]
	    continue
	}

	if {$tagName == "rdmo"} {
	    binary scan $data "a4" tags($tagName)
          continue
      }

	if {$tagName == "fmjd"} {
	    binary scan $data "II" mjd tms 
	    set tags($tagName) [list $mjd $tms]
	    continue
	}

	# andrewm - 20070228 - GPS TAG decode
	if {$tagName == "rgps"} {
	    set count [binary scan $data "ccScSScSSccccSccSS" source satellites latDD latM \
			latmm longDD longM longmm altAA alta timeH timeM timeS dateYY dateM dateD speed heading]
        if {$count == 18} {
		
	      set latM [expr { $latM & 0xff }]
	      set latmm [expr { $latmm & 0xffff }]

	      set latDeg [expr { ( $latDD + ( $latM + $latmm/65536.0) / 60.0 ) }]

	      set longM [expr { $longM & 0xff }]
	      set longmm [expr { $longmm & 0xffff }]
	      set longDeg [expr { ( $longDD + ( $longM + $longmm/65536.0) / 60.0 ) }]

          set alta [expr { $alta & 0xff }]
          set altMetres [expr { $altAA + ( $alta / 60.0 ) }]

	      set speed [expr { $speed & 0xffff }]
	      set heading [expr { $heading & 0xffff }]

	      set tags($tagName) [list $latDeg $longDeg $altMetres $dateYY $dateM $dateD $timeH $timeM $timeS $speed $heading]
        } 

	    continue
	}

	if {$tagName == "rfre"} {
	    binary scan $data "I" tags($tagName)
	    continue
	}		

	if {$tagName == "rmer"} {

	    # Init list
	    set tags($tagName) [list]

	    # Go though every value and store
	    for {set i 0} {$i<$byteLength} {incr i 2} {
		set subData [string range $data $i [expr $i+1]]
		binary scan $subData "cc" byte1 byte2
		set byte2 [expr ($byte2 + 0x100)%0x100]
		lappend tags($tagName) [format "%.0f" [expr $byte1+$byte2/256.0]]
	    }

	    # In case that we are using se127 v2 or lower, copy rmer value to rwmf
	    if {$SE127_VERSION <3} {
		set tags(rwmf) $tags(rmer)
	    }

	    continue
	}	

	if {$tagName == "rwmf"} {

	    # Init list
	    set tags($tagName) [list]

	    # Go though every value and store
	    for {set i 0} {$i<$byteLength} {incr i 2} {
		set subData [string range $data $i [expr $i+1]]
		binary scan $subData "cc" byte1 byte2
		set byte2 [expr ($byte2 + 0x100)%0x100]
		lappend tags($tagName) [format "%.0f" [expr $byte1+$byte2/256.0]]
	    }
	    continue
	}	

	if {$tagName == "rwmm"} {

	    # Init list
	    set tags($tagName) [list]

	    # Go though every value and store
	    for {set i 0} {$i<$byteLength} {incr i 2} {
		set subData [string range $data $i [expr $i+1]]
		binary scan $subData "cc" byte1 byte2
		set byte2 [expr ($byte2 + 0x100)%0x100]
		lappend tags($tagName) [format "%.0f" [expr $byte1+$byte2/256.0]]
	    }
	    continue
	}	



	if {$tagName == "rser"} {
	    binary scan $data "c" tags($tagName) 
	    continue
	}

	if {$tagName == "rdel"} {
	    # Init list
	    set tags($tagName) [list]

	    # Go though every value and store
	    for {set i 0} {$i<$byteLength} {incr i 3} {
		set subData [string range $data $i [expr $i+2]]
		binary scan $subData "ccc" percentage byte1 byte2
		set byte2 [expr ($byte2 + 0x100)%0x100]
		lappend tags($tagName) [list $percentage [format "%.1f" [expr $byte1+$byte2/256.0]]]
	    }
	    continue
	}

	if {$tagName == "Bdia"} {
		binary scan $data "cB8SScccB8a4a4a4a4a4a4a4a4a4a4a4a4a4a4a4a4Sca4" \
	       dummy rxStat dspStat mlcStat audioStat cpuLoad drmMode stickyFlags \
	       minFP maxFP minTCS maxTCS minSe maxSe \
	       csiMean csiPeak csiPos csiPk2Mn \
	       cirPeak cirPos \
	       mass1 mass1Pos mass2 doppler \
	       cwPos attenuation bufferFP

		# Use Bdiag doppler if we are in version 2 or lower of the se127 protocol
		if {$SE127_VERSION < 3} {
			set tags(rdop) [format "%.1f" [IEEE2float $doppler 0]]
		}
	}

	if {$tagName == "rdbv"} {

	    # Init list
	    set tags($tagName) [list]

	    # Go though every value and store
	    for {set i 0} {$i<$byteLength} {incr i 2} {
		set subData [string range $data $i [expr $i+1]]
		binary scan $subData "cc" byte1 byte2
		set byte2 [expr ($byte2 + 0x100)%0x100]
		lappend tags($tagName) [format "%.0f" [expr $byte1+$byte2/256.0]]
	    }
	    continue
	}	

	if {$tagName == "rsta"} {
	    binary scan $data "cccc" byte0 byte1 byte2 byte3
	    set byte0 [expr ($byte0 + 0x100)%0x100]
	    set byte1 [expr ($byte1 + 0x100)%0x100]
	    set byte2 [expr ($byte2 + 0x100)%0x100]
	    set byte3 [expr ($byte3 + 0x100)%0x100]
	    set tags($tagName) [list $byte0 $byte1 $byte2 $byte3]
	    continue
	}

	if {$tagName == "rbp0" || $tagName == "rbp1" || $tagName == "rbp2" || $tagName == "rbp3"} {
	    binary scan $data "SS" byte0 byte1
	    set byte0 [expr ($byte0 + 0x10000)%0x10000]
	    set byte1 [expr ($byte1 + 0x10000)%0x10000]
	    set tags($tagName) [list $byte0 $byte1]
	    continue
	}

	if {$tagName == "rbw_"} {
	    binary scan $data "cc" byte1 byte2
	    set byte2 [expr ($byte2 + 0x100)%0x100]
	    set tags($tagName) [format "%.2f" [expr $byte1+$byte2/256.0]]
	    continue
	}	


	if {$tagName == "rafs"} {
	    binary scan $data "cB8B8B8B8B8" noOfFrames b1 b2 b3 b4 b5
	    set bitString [string range $b1$b2$b3$b4$b5 0 [expr $noOfFrames-1]]
	    set tags($tagName) $bitString
	    continue
	}	


	if {$tagName == "rdop"} {
	    binary scan $data "cc" byte1 byte2
	    set byte2 [expr ($byte2 + 0x100)%0x100]
	    set tags($tagName) [format "%.2f" [expr $byte1+$byte2/256.0]]
	    continue
	}	

#	if {$tagName == "str0"} {
#	    set tags($tagName) ""
#	    continue
#	}

	# All other tags: Use data as it is
	set tags($tagName) $data
    }

    ########### Put this in since the receiver occasionally seems to forget to send AFS 
    # Look into this in the new year
    # Boy: I guess this is because occasionaly some UDP-packets get messed up and
    #      not all the tags are correctly received, there was an error
    #      calling this function from recordall.tcl, it was fixed.
    #
    if ![info exists tags(rafs)] {
    	return -1
    }
}

proc GetTagInfo {tty tagsVar binaryFrameVar} {
    global SE127_PROTOCOL

    # Reads next SUDP packet from serial port and extracts tag content into tags.
    # The whole packet including header is returned in binaryFrameVar

    upvar $tagsVar tags
    upvar $binaryFrameVar binaryFrame

    # Data is returned in 3 binary segments: (IO)id, (IO)header and IO(payload)

	set binarySegments [ReadPacket $tty]

    set id [lindex $binarySegments 0]
    set header [lindex $binarySegments 1]
    set tagContent [lindex $binarySegments 2]

    # Compose binary frame
    set binaryFrame $id$header$tagContent

    # Return -1 if error occurred
    if {$tagContent == -1 || $binarySegments == -1} {
	puts "Error in packet!"
	return -1
    }

    # Process the TAG content
    if {[ProcessTagContent $tagContent tags] == -1} {
    	puts "Error with tags!"
	return -1
    }

   return 0
}

proc ShowTags {tagsVar} {

    # Diplays content of tags array
    upvar $tagsVar tags

    puts ""

    foreach tagName [array names tags] {
	if ![regexp {^str[0-3]$} $tagName match] {
	    puts $tagName\t$tags($tagName)
	}
    }
}

proc BuildTagLayer {tagName tagContent} {

    # Builds tag item and returns it, ready to be inserted into tag layer
    # <tag content> needs to be binary string (it is inserted as is)

    set size [string length $tagContent]
    set dataBitSize [expr $size*8]

    set tag $tagName
    append tag [binary format "I" $dataBitSize] $tagContent

    return $tag
}

proc CmdSetFrequency {tty frequency mode source destination} {

    # Sets frequency of receiver. Frequency is given in kHz
    # Sets the demodulation mode of the receiver.
    # Options are drm_, am__, usb_, lsb_, san_ 
    # Source and destination are 16 bit numbers

    set frequency [expr $frequency*1000]

    # Build TAG
    set tagLayer [BuildTagLayer "cfre" [binary format "I" [expr $frequency]]]
    append tagLayer [BuildTagLayer "cdmo" [string range $mode 0 3]]

    # Build IPIO/DCP
    set protLayer [BuildLayer $tagLayer]
#    DisplayBinaryString $protLayer

    puts -nonewline $tty $protLayer
    flush $tty
}


proc CmdSetAMFilter {tty bandwidthKHz limit source destination} {

    # limit should be "upper" or "lower". "lower is default.
    if {$limit == "upper"} {
	set command "cbwg"
    } else {
	set command "cbws"
    }

    set byte1 [expr int($bandwidthKHz)]
    set byte2 [expr int ( ($bandwidthKHz-int($bandwidthKHz))*256 )]	 

    # Build TAG
    set tagLayer [BuildTagLayer $command [binary format "cc" $byte1 $byte2]]

    # Build IPIO/DCP
    set protLayer [BuildLayer $tagLayer]
#    DisplayBinaryString $protLayer

    puts -nonewline $tty $protLayer
    flush $tty
}

proc CmdSetTime {tty date hhmmss overwrite source destination} {

    # hhmmss as hh:mm:ss
    # date as yyyy-mm-dd
    # overwrite = 1: overwrite external time
    
    set overwrite [expr $overwrite & 0x01]

    regexp {(..):(..):(..)} $hhmmss match hh mm ss
    regexp {(....)-(..)-(..)} $date match y m d

    # Correct in case that number start with '0' -> octal (how stupid is that?)
    if [regexp {^0} $hh match] {set hh [string range $hh 1 1]}
    if [regexp {^0} $mm match] {set mm [string range $mm 1 1]}
    if [regexp {^0} $ss match] {set ss [string range $ss 1 1]}
    if [regexp {^0} $m match] {set m [string range $m 1 1]}
    if [regexp {^0} $d match] {set d [string range $d 1 1]}

    # calculate mjd
    set a_ [expr ceil( ($m-14)/12.0 )]
    set b_ [expr $y+4800+$a_]
    set c_ [expr int( (1461*$b_)/4.0 )]
	
    set d_ [expr ceil( ($m-14)/12.0 )]
    set e_ [expr $m-2-12*$d_]
    set f_ [expr int( (367*$e_)/12.0 )]
	
    set g_ [expr ceil( ($m-14)/12.0 )]
    set h_ [expr int( ($y+4900+$g_)/100.0 )] 
    set i_ [expr int( (3*$h_)/4.0 )]	

    set jd  [expr $c_ + $f_ - $i_ + $d - 32075]
    set mjd [expr $jd-2400000-1]
 
    # cal milliseconds
    set millisec [expr 10000*(int($hh)*3600+int($mm)*60+int($ss))]

    # Build TAG
    set tagLayer [BuildTagLayer "cclk" [binary format "cII" $overwrite $mjd $millisec]]

    # Build IPIO/DCP
    set protLayer [BuildLayer $tagLayer]
#    DisplayBinaryString $protLayer

    puts -nonewline $tty $protLayer
    flush $tty
}


proc CmdStartRecording {tty profile source destination} {

    # Send a command to the receiver to start recording
    # Source and destination are 16 bit number

    set profilelist [ split $profile {} ]
    set profile {}

    foreach profile $profilelist {
	# Build TAG
	
	if {$profile == "i"} {
		set profiletag "iq_"
		set profilename "IQ"
	} else {
		set profiletag [ format "%s%s" "st" $profile ]
		set profilename [ string toupper $profile ]
	}

	set tagLayer [BuildTagLayer "crec" [ format "%s%d" $profiletag 1 ]]



	# Build IPIO/DCP
	set protLayer [BuildLayer $tagLayer]

	# Send over to control port
	puts -nonewline $tty $protLayer
	flush $tty

	puts "Started recording of profile $profilename at the receiver..."
    }

}

proc CmdStopRecording {tty profile source destination} {

    # Send a command to the receiver to stop recording
    # Source and destination are 16 bit number

    set profilelist [ split $profile {} ]
    set profile {}

    foreach profile $profilelist {
	# Build TAG
	
	if {$profile == "i"} {
		set profiletag "iq_"
		set profilename "IQ"
	} else {
		set profiletag [ format "%s%s" "st" $profile ]
		set profilename [ string toupper $profile ]
	}

	set tagLayer [BuildTagLayer "crec" [ format "%s%d" $profiletag 0 ]]

	# Build IPIO/DCP
	set protLayer [BuildLayer $tagLayer]

	# Send over to control port
	puts -nonewline $tty $protLayer
	flush $tty

	puts "Stopped recording of profile $profilename at the receiver..."
    }
}

proc CmdActivate {tty flag source destination} {

    # Sends cact command with $flag as content
    # flag = 0: decativate
    # flag = 1: activate
    # Source and destination are 16 bit numbers

    # Build TAG
    set tagLayer [BuildTagLayer "cact" $flag]]

    # Build IPIO/DCP
    set protLayer [BuildLayer $tagLayer]

    # Send over to control port
    puts -nonewline $tty $protLayer
    flush $tty
}

proc CmdSetService {tty service source destination} {

    # Sends cser command with $service as content
    # flag = 0-4: to select servicenumber (integer 1-4)
    # Source and destination are 16 bit numbers

    # Build TAG
    set tagLayer [BuildTagLayer "cser" $service]]

    # Build IPIO/DCP
    set protLayer [BuildLayer $tagLayer]

    # Send over to control port
    puts -nonewline $tty $protLayer
    flush $tty
}

proc CmdSetDemodulation {tty demodulationtype source destination} {

    # Sends cdmo command with $demodulationtype as content
    # Receiver Demodulation Type:
    # ASCII Text for demodulation type of receiver
    # drm_ : DRM Demodulation
    # am__ : AM Demodulation
    # usb_ : Upper Side Band Demodulation
    # lsb_ : Lower Side Band Demodulation
    # sam_ : Synchronous AM Demodulation

    # Source and destination are 16 bit numbers

    # Build TAG
    set tagLayer [BuildTagLayer "cdmo" $demodulationtype]]

    # Build IPIO/DCP
    set protLayer [BuildLayer $tagLayer]

    # Send over to control port
    puts -nonewline $tty $protLayer
    flush $tty
}


proc CmdSetProfile {tty profile} {

    # Sends cpro command with $profile as content
    # Source and destination are 16 bit numbers

    # Build TAG
    set tagLayer [BuildTagLayer "cpro" $profile]

    # Build IPIO/DCP
    set protLayer [BuildLayer $tagLayer]

    # Send over to control port
    puts -nonewline $tty $protLayer
    flush $tty
}




proc FACDecoder {facString} {


    # Decodes FAC nd returns list with content

    # Read CRC
    set facLength [string length $facString]
    binary scan [string range $facString [expr $facLength-1] [expr $facLength-1]] "c" crc
    set crc [expr ($crc+0x100)%0x100]
    set crc [expr $crc ^ 0xff];# Invert
       
    # Convert to string of bits
    for {set i 0} {$i<$facLength} {incr i} {
	binary scan [string range $facString $i $i] "B8" byte
	append bitString $byte
    }

    # puts $bitString

    # Channel parameters
    set enhance [string range $bitString 0 0]
    binary scan [binary format "B8" 000000[string range $bitString 1 2]] "c" identity
    binary scan [binary format "B8" 0000[string range $bitString 3 6]] "c" occupancy
    set interleaver [string range $bitString 7 7]
    binary scan [binary format "B8" 000000[string range $bitString 8 9]] "c" mscMode
    set sdcMode [string range $bitString 10 10]
    binary scan [binary format "B8" 0000[string range $bitString 11 14]] "c" noServices
    binary scan [binary format "B8" 00000[string range $bitString 15 17]] "c" reconfigure
    binary scan [binary format "B8" 000000[string range $bitString 18 19]] "c" rfu

    # Service parameters
    binary scan [binary format "B32" 00000000[string range $bitString 20 43]] "I" longService
    binary scan [binary format "B8" 000000[string range $bitString 44 45]] "c" shortService
    set ca [string range $bitString 46 46]
    binary scan [binary format "B8" 0000[string range $bitString 47 50]] "c" language
    set audioData [string range $bitString 51 51]
    binary scan [binary format "B8" 000[string range $bitString 52 56]] "c" descriptor
    binary scan [binary format "B8" 0[string range $bitString 57 63]] "c" rfa

    set resultList [list $enhance $identity $occupancy $interleaver $mscMode $sdcMode $noServices]
    lappend resultList $reconfigure $rfu $longService $shortService $ca $language $audioData
    lappend resultList $descriptor $rfa
    
    return $resultList
}


proc SDCDecoder {sdcString} {

    # Decodes SDC and retruns a list with the data entity numbers and short IDs that were found
    # Format: {DEid_shortId ...}

    set sdcLength [string length $sdcString]

    for {set i 0} {$i<$sdcLength} {incr i} {
	binary scan [string range $sdcString $i $i] "B8" byte
	append bitString $byte
    }
	

    # AFS index
    binary scan [binary format "B8" [string range $bitString 0 7]] "c" afsIndex
    set afsIndex [expr $afsIndex >> 4]
    
    set resultList [list]

    set dePtr 8

    # OPH: Don't process the 16 bits of crc at the end.
    while {$dePtr < [expr $sdcLength*8-16]} {

	# Extact data of data entity
	binary scan [binary format "B8" 0[string range $bitString $dePtr [expr $dePtr+6]]] "c" length

	# Check for data end marker
	if {$length == 0x00} {
	    break
	}

	set version [string range $bitString [expr $dePtr+7] [expr $dePtr+7]]
	binary scan [binary format "B8" 0000[string range $bitString [expr $dePtr+8] [expr $dePtr+11]]] "c" type

	set dataBits [string range $bitString [expr $dePtr+12] [expr $dePtr+11+$length*8+4]]
	binary scan [binary format "B8" 000000[string range $dataBits 0 1]] "c" shortId

	# Store result
	lappend resultList [list $type\_$shortId]

	# Advance pointer
	incr dePtr [expr 12+$length*8+4]
    }	

    return $resultList
}


proc SDCExtractor {sdcString wantedDe} {

    # Decodes SDC and returns the data contained in the the data entity identifyed by <wantedDe>
    # in the appropriate format
    # Always works on the data with CRC removed

    set sdcLength [string length $sdcString]

    for {set i 0} {$i<$sdcLength} {incr i} {
	binary scan [string range $sdcString $i $i] "B8" byte
	append bitString $byte
    }

    # AFS index
    binary scan [binary format "B8" [string range $bitString 0 7]] "c" afsIndex
    set afsIndex [expr $afsIndex >> 4]
    
    set dePtr 8

    # 16 bits of CRC at the end have already been removed by SDCCompressor.
    while {$dePtr < [expr $sdcLength*8]} {

	# Extact data of data entity
	binary scan [binary format "B8" 0[string range $bitString $dePtr [expr $dePtr+6]]] "c" length

	# Check for data end marker
	if {$length == 0x00} {
	    break
	}

	set version [string range $bitString [expr $dePtr+7] [expr $dePtr+7]]
	binary scan [binary format "B8" 0000[string range $bitString [expr $dePtr+8] [expr $dePtr+11]]] "c" type
	set dataBits [string range $bitString [expr $dePtr+12] [expr $dePtr+11+$length*8+4]]
	binary scan [binary format "B8" 000000[string range $dataBits 0 1]] "c" shortId

	# Store dataBits if <type> corresponds to <wantedDe>
	if {$wantedDe == "$type\_$shortId"} {
	    set storedBits $dataBits
	}
	
	# Advance pointer
	incr dePtr [expr 12+$length*8+4]
	
#	puts [expr $length*8]\t$dePtr\t[expr $sdcLength*8]\t$resultList
    }

    # Check whether storedBits contains information; if it does not, return
    if ![info exists storedBits] {
    	return ""
    }

    # Here, if <storedBits> contain information, we know that both DE ID and the first 2 bits (normally short id) 
    # are matched.

    # If the data entity was identified..
    # puts \n$wantedDe\t$storedBits

    # Init result
    set resultList [list]

    # Type 0: Multiplex
    if [regexp {^0} $wantedDe] {

	if {[string length $storedBits] != 28} {
	    # puts "Error incorrect length of SDC type 0: [string length $storedBits]. Ignoring"
	    # return
	}

	binary scan [binary format "B8" 000000[string range $storedBits 0 1]] "c" protectionA
	binary scan [binary format "B8" 000000[string range $storedBits 2 3]] "c" protectionB

	set resultList [list $protectionA $protectionB]

	set dePtr 4
	set streamId 0
            set storedBitLen [string length $storedBits]
	while {$dePtr < [string length $storedBits]} {
	    binary scan [binary format "B16" 0000[string range $storedBits $dePtr [expr $dePtr+11]]] "S" lengthA
	    binary scan [binary format "B16" 0000[string range $storedBits [expr $dePtr+12] [expr $dePtr+23]]] "S" lengthB

	    lappend resultList [list $lengthA $lengthB]

	    incr dePtr 24
	    incr streamId 
	    # puts $streamId\t$dePtr\t[string length $storedBits]
	}
    }

   # Type 1: Label
    if [regexp {^1} $wantedDe] {

	binary scan [binary format "B8" 000000[string range $storedBits 0 1]] "c" shortId
	binary scan [binary format "B8" 000000[string range $storedBits 2 3]] "c" rfu

	set dePtr 4
	set label ""
	while {$dePtr != [string length $storedBits]} {
	    set character [binary format "B8" [string range $storedBits $dePtr [expr $dePtr+7]]]
            if {$character != "\0"} {
		    append label $character
		}
	    incr dePtr 8
	    # puts $streamId\t$dePtr\t[string length $storedBits]
	}
	
	# Assemble result
	set resultList [list $shortId  $rfu]
	lappend resultList $label

    }

    # Type 9: Audio information
    if [regexp {^9} $wantedDe] {

	binary scan [binary format "B8" 000000[string range $storedBits 0 1]] "c" shortId
	binary scan [binary format "B8" 000000[string range $storedBits 2 3]] "c" streamId
	binary scan [binary format "B8" 000000[string range $storedBits 4 5]] "c" audioCoding
	set sbrFlag [string range $storedBits 6 6]
	binary scan [binary format "B8" 000000[string range $storedBits 7 8]] "c" audioMode
	binary scan [binary format "B8" 00000[string range $storedBits 9 11]] "c" samplingRate
	set textFlag [string range $storedBits 12 12]
	set enhancementFlag [string range $storedBits 13 13]
	binary scan [binary format "B8" 000[string range $storedBits 14 18]] "c" codeField	    
	set rfa [string range $storedBits 19 19]
	
	# Assemble result
	set resultList [list $shortId $streamId]
	lappend resultList $audioCoding $sbrFlag $audioMode $samplingRate $textFlag $enhancementFlag $codeField $rfa
    }


    return $resultList
}

proc SDCCompressor {sdcString} {

    # Decodes SDC and retruns the part that contains data. CRC is ignored

    set sdcLength [string length $sdcString]

    for {set i 0} {$i<$sdcLength} {incr i} {
	binary scan [string range $sdcString $i $i] "B8" byte
	append bitString $byte
    }

    # AFS index
    binary scan [binary format "B8" [string range $bitString 0 7]] "c" afsIndex
    set afsIndex [expr $afsIndex >> 4]
    
    set resultList [list]

    set dePtr 8

    # OPH: Don't process the 16 bits of CRC
    while {$dePtr < [expr $sdcLength*8-16]} {

	# Extact data of data entity
	binary scan [binary format "B8" 0[string range $bitString $dePtr [expr $dePtr+6]]] "c" length

	# Check for data end marker
	if {$length == 0x00} {
	    break
	}

	set version [string range $bitString [expr $dePtr+7] [expr $dePtr+7]]
	binary scan [binary format "B8" 0000[string range $bitString [expr $dePtr+8] [expr $dePtr+11]]] "c" type
	set dataBits [string range $bitString [expr $dePtr+12] [expr $dePtr+11+$length*8+4]]
	binary scan [binary format "B8" 000000[string range $dataBits 0 1]] "c" shortId

	# Advance pointer
	incr dePtr [expr 12+$length*8+4]
    }	

    # Caclulate byte length
    set length [expr $dePtr/8]

    # Retrun compressed string
    #return [string range $sdcString 0 $length]
    # OPH: miss off the AFS index. length includes only up to the end of the last DE, no padding, no CRC
    return [string range $sdcString 1 [expr $length-1]]
}

proc ConvertList2Bin {byteList} {

    # Converts bytes in <byteList> into a binary string
    foreach byte $byteList  {
	append out [binary format "c" $byte ]
    }

    return $out
}


proc BuildTagItemGPS {gpsList} {
	set latDeg [lindex $gpsList 0]
	set longDeg [lindex $gpsList 1]

	set latDD [expr ( int(floor($latDeg)))]
	set r [expr ($latDeg - $latDD) * 60.0]
	set latM [expr ( int(floor($r)) )]
	set r [expr ($r - $latM) * 65536.0]
	set latmm [expr int((floor($r)) )]

	set longDD [expr int(( floor($longDeg) ))]
	set r [expr ($longDeg - $longDD) * 60.0]
	set longM [expr int(( floor($r) ))]
	set r [expr ($r - $longM) * 65536.0]
	set longmm [expr int((floor($r) ))]

	set source 0
	set satellites 0
	set altAA 0xFFFF
	set alta 0xFF
	set timeH 0xFF
	set timeM 0xFF
	set timeS 0xFF
	set dateYY 0xFFFF
	set dateM 0xFF
	set dateD 0xFF
	set speed 0xFF
	set heading 0xFF
	
	set tagBody [binary format "ccScSScSSccccSccSS" $source $satellites $latDD $latM \
			$latmm $longDD $longM $longmm $altAA $alta $timeH $timeM $timeS $dateYY $dateM $dateD $speed $heading]

    	return [BuildTagLayer "rgps" $tagBody]
}

# TAG body for dB TAG items like rmer, rwmf etc
proc BuildTagBodydB {val} {
      set formatVal [expr int(floor($val * 256.0))]
      return [binary format "S" $formatVal]
}

proc BuildTagItemStatsdB {nFrames merList statList tagName} {
      set nStats [llength $merList]
      set tagBody [binary format "Sc" $nFrames $nStats]
      for {set i 0} {$i<$nStats} {incr i} {
        append tagBody [binary format "c" [lindex $statList $i]]
        append tagBody [BuildTagBodydB [lindex $merList $i]]
      }
      return [BuildTagLayer $tagName $tagBody]
}

proc BuildTagItemAudioStats {audioStats} {
      set tagBody [binary format "SS" [lindex $audioStats 0] [lindex $audioStats 1]]
      return [BuildTagLayer "rast" $tagBody]
}
