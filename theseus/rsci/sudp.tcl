
package provide sudp 1.0
package require rsci_crc 1.0

# Create the namespace
namespace eval ::sudp {
    # Export commands
    namespace export ReadPacket BuildLayer
}

proc sudp::ReadPacket {tty} {

    # Reads next SUDP packet from serial port and returns payload

    # Search for "SP" = serial packet
    set idString "SP"
    
    set firstCharacterWritten 0

    set firstChar [string range $idString 0 0]
    set lastChar [string range $idString 1 1]
					      

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
    
    set in [read $tty 10]
    binary scan $in "SSccSS" crc len fill ci src dst
    
    # Convert to unsigned numbers
    set crc [expr ($crc + 0x10000)%0x10000]
    set len [expr ($len + 0x10000)%0x10000]
    set ci [expr ($ci + 0x100)%0x100]
    set src [expr ($src + 0x10000)%0x10000]
    set dst [expr ($dst + 0x10000)%0x10000]

    # If the length is greater than 10000 bytes, there is an error
    if {$len > 10000} {
	puts "Error! Packet size is long: $len bytes. Ignoring."
	return -1
    }

    # Read <len> payload bytes
    set payload [read $tty $len]

    return $payload
}

proc sudp::BuildLayer {data source destination} {

    # Build SUDP layer around <data> (which should be the IPIO layer)
    # <source > and <destination> are 16-bit words

    global buildSudpLayerCI

    # Init individual fields
    set id "SP"

    set length [string length $data]
    set fill 0x00
    if ![info exists buildSudpLayerCI] {
	set buildSudpLayerCI 0x00
    }
    set buildSudpLayerCI [expr ($buildSudpLayerCI+1)%0x100]

    # Construct header after CRC + data
    set result [binary format "S" $length]
    append result [binary format "c" $fill]
    append result [binary format "c" $buildSudpLayerCI]
    append result [binary format "S" $source]
    append result [binary format "S" $destination]
    append result $data

    # Calculate CRC
    set crc [RS_CRCGenerator $result 0x11021]
    if {$crc == 0x0000} {
	set crc 0x5555
    }
# set crc 0x01010
    # Add Id and CRC to the front of the packet
    set out $id
    append out [binary format "S" $crc]
    append out $result
    
    return $out
}