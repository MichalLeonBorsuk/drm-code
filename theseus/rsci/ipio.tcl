
package provide ipio 1.0
package require rsci_crc 1.0

# Create the namespace
namespace eval ::ipio {
    # Export commands
    namespace export ReadSUDPPacket BuildSUDPLayer ReadPacket BuildLayer
}

proc ipio::ReadPacket {tty} {

    set id "IO"

    global UDP_PACKAGE
    global STATUS_PROTOCOL

    set firstCharacterWritten 0
    set firstChar [string range $id 0 0]
    set lastChar [string range $id 1 1]
					      
    while {1} {
	# Search first character and output characters that don't fit into the protocol (error messages)
	while {[set readChar [read $tty 1]] != $firstChar} {
	    
	    # Start new line of first character of error message
	    if {!$firstCharacterWritten} {
		puts ""
		set firstCharacterWritten 1
	    }
	    if [regexp {[a-zA-Z0-9\n\r -_]} $readChar] {
		puts -nonewline $readChar
		flush stdout
	    }
	}
    
	# Search for second character
	if {[read $tty 1] == $lastChar} {
	    set firstCharacterWritten 0
	    break
	}
    }

    set header [read $tty 10]


    # Decode header
    binary scan $header "ccSSSS" rev rf len ci tp st

    # Convert to unsigned numbers
    set rev [expr ($rev + 0x100)%0x100]
    set rf [expr ($rf + 0x100)%0x100]
    set len [expr ($len + 0x10000)%0x10000]
    set ci [expr ($ci + 0x10000)%0x10000]
    set tp [expr ($tp + 0x10000)%0x10000]
    set st [expr ($st + 0x10000)%0x10000]

#    puts [format "IPIO: %2x\t%2x\t%4x\t%4x\t%4x\t%4x" $rev $rf $len $ci $tp $st]

    # If the length is greater than 10000 bytes, there is an error
    if {$len > 10000} {
	puts "Error! Packet size is long: $len bytes. Ignoring."
	return -1
    }

    # Read <len> payload bytes
    set payload [read $tty $len]
                          
    if {$STATUS_PROTOCOL == "UDP"} {
       if {$UDP_PACKAGE != "DP"} {
          #andrewm, 18/01/07 - read anything that's left (eg. the 2 CRC bytes!), otherwise the TCLUDP library blocks
          read $tty 10000
       }
    }
   
    # Return payload
    return [list $id $header $payload]
}

proc ipio::BuildLayer {data} {

    # Builds IPIO layer around <data> (which should be the tag layer)

    global buildIpioLayerCI

    set id "IO"
    set rev 0x10
    set rf 0x00
    set length [string length $data]
    if ![info exists buildIpioLayerCI] {
	set buildIpioLayerCI 0xffff
    }
    set buildIpioLayerCI [expr ($buildIpioLayerCI+1)%0x10000]
    set tp 0x0101
    set st 0x0000

    set result $id
    append result [binary format "c" $rev]
    append result [binary format "c" $rf]
    append result [binary format "S" $length]
    append result [binary format "S" $buildIpioLayerCI]
    append result [binary format "S" $tp]
    append result [binary format "S" $st]
    append result $data

    return $result
}
