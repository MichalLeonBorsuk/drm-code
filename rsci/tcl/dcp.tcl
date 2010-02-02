
package provide dcp 1.0
package require rsci_crc 1.0

# Create the namespace
namespace eval ::dcp {
    # Export commands
    namespace export ReadPacket BuildLayer
}

proc dcp::ReadPacket {tty} {
    set id "AF"

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
		set firstCharacterWritten 1
	    }
	    if [regexp {[a-zA-Z0-9\n\r -_]} $readChar] {
		# puts -nonewline $readChar
		flush stdout
	    }
	    if {[eof $tty]} {
	      return -1;
	    }
	}
    
	# Search for second character
	if {[read $tty 1] == $lastChar} {
	    set firstCharacterWritten 0
	    break
	}
    }

    set header [read $tty 8]

    # Decode header
    binary scan $header "IScA" len seq ar pt

    # Convert to unsigned numbers
    set len [expr ($len + 0x1000000)%0x1000000]
    set seq [expr ($seq + 0x10000)%0x10000]
    set ar [expr ($ar + 0x100) % 0x100]
       set crc [expr (($ar & 0x80) >> 7)]
       set maj [expr (($ar & 0x70) >> 4)]
       set min [expr (($ar & 0x0F) >> 0)]

#    puts [format "DCP: len: %4x\t seq: %4x\tcrc: %1x\tversion: %x.%x %s" $len $seq $crc $maj $min $pt]

    # If the length is greater than 10000 bytes, there is an error
    if {$len > 10000} {
	puts "Error! Packet size is long: $len bytes. Ignoring."
	return -1
    }

    # Read <len> payload bytes
    set payload [read $tty $len]
    
    # Read the CRC bytes
    set crc [read $tty 2]
    
    if {$STATUS_PROTOCOL == "UDP"} {
       if {$UDP_PACKAGE != "DP"} {
          #andrewm, 18/01/07 - read anything that's left, otherwise the TCLUDP library blocks
          #OPH - it's not enough to exactly exhaust the buffer - need to read past the end
          read $tty 10000
       }
    }

    # Return payload
    return [list $id $header $payload $crc]
}
proc dcp::BuildLayer {data} {

    # Mod by Boy Kentrop, 20031222
    # Builds DCP layer around <data> (which should be the tag layer)

    global buildIpioLayerCI
    global USE_AF_CRC

    set id "AF"
    set len [string length $data]
    set seq 0xffff

    if [info exists USE_AF_CRC] {
      set crcflag $USE_AF_CRC
    } else {
       set crcflag 0
    }
       # Init CRC with 0xffff
       set crc 0xffff
       set maj 1
       set min 0
       set ar 0x00000000
       set ar [expr ($ar + ($crcflag << 7 & 0x80))]
       set ar [expr ($ar + ($maj << 4 & 0x70))]
       set ar [expr ($ar + ($min << 0 & 0x0F))]

    set pt T

    set result $id
    append result [binary format "I" $len]
    append result [binary format "S" $seq]
    append result [binary format "c" $ar]
    append result [binary format "A" $pt]
    append result $data

    # Polynom is x16+x12+x5+1 = 69665 = 0x11021 (CCITT)
    # Calculate CRC according to polynom and bit-reverse it (0xffff-CRC)

    if {$crcflag == 1} {
       set crc [RS_CRCGenerator $result 0x11021]
       set crc [expr (0xffff-$crc)]
    } else {
       set crc 0x0000
    }
    append result [binary format "S" $crc]

    return $result
}
