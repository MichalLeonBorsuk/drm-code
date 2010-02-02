# Reads data from serial port and forwards it onto the TCP port if someone is connected.
# Inversely, control data from a second TCP port is passed down the serial line.
# Start with TCL 8.3!

package require sudp 1.0

# Server to receive control commands
proc StartControlServer {port} {
	socket -server AcceptControlConnection $port
	puts "TCP control server started on port $port."
}

proc AcceptControlConnection {socket addr port} {

	global controlId

	fconfigure $socket -blocking 0
	fconfigure $socket -translation binary
	fileevent $socket readable [list ProcessControlData]
	puts "Connection established with $addr:$port"
	set controlId $socket
}


# Server to send status information
proc StartStatusServer {port} {
	socket -server AcceptStatusConnection $port
	puts "TCP status server started on port $port."
}

proc AcceptStatusConnection {socket addr port} {

	global statusId

	fconfigure $socket -blocking 0
	fconfigure $socket -translation binary
	fileevent $socket readable [list ProcessStatusProblem]
	puts "Connection established with $addr:$port"
	set statusId $socket
}



proc ProcessStatusData {serialId} {

	# Read status SUDP packet from serial port and feed payload into the tcp port
	# (if someone is connected)

	global statusId

	set sudpPayload [sudp::ReadPacket $serialId]

	# Retuen if error occurred
	if {$sudpPayload == -1} {
		return
	}

#	 puts "Received [string length $sudpPayload] bytes."

	if [info exists statusId] {
		puts -nonewline $statusId $sudpPayload
		flush $statusId
#		puts "Sending packet of length [string length $sudpPayload]"
	}
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
	
	
	

proc ProcessControlData {} {

	# Read control data from TCP port, adds a SUDP header and forwards it to the serial line

	global controlId
	global serialId

	set ipioPacket [read $controlId]

	# Close socket if connection breaks
	if [eof $controlId] {
		catch {close $controlId}  
		puts "Control connection lost"
		unset controlId
		return
	}

#	puts "Received control packet of length [string length $ipioPacket]."

	# Build SUDP backet
	set sudpPacket [sudp::BuildLayer $ipioPacket 0x1 0x02]

#       DisplayBinary "$sudpPacket\n"

	# Write to serial line
	puts -nonewline $serialId $sudpPacket
	flush $serialId
	
}

proc DisplayBinary {binaryString} {

    # Display binary string as sequence of hex values

    for {set i 0} {$i<[string length $binaryString]} {incr i} {

	binary scan [string range $binaryString $i $i] "c" byteValue
	set byteValue [expr ($byteValue+0x100)%0x100]
	puts -nonewline [format "%02x " $byteValue]

	if {[expr $i%16] == 15} {
	    puts ""
	}
    }
    puts ""
}

##### Main #####

if { $argc != 3 } {
        puts "usage: tclsh serial2tcp.tcl <status_port> <control_port> <com_port>"
        puts "For example, tclsh serial2tcp.tcl 30011 30002 COM1".
        puts "Please try again."
		exit
} else {
		set STATUS_PORT [lindex $argv 0]
		set CONTROL_PORT [lindex $argv 1]
		set serialPort [lindex $argv 2]
}

# Open serial port for reading and writing
set serialId [open $serialPort r+]
fconfigure $serialId -mode 115200,n,8,1 
fconfigure $serialId -blocking 1 -translation binary

# Open output file to store incoming data
#set storeInFileId [open "RAW_[clock format [clock seconds] -format "%Y-%m-%d_%H-%M-%S"].bin" w]
#fconfigure $storeInFileId -translation binary

# Start TCP server
StartControlServer $CONTROL_PORT
StartStatusServer $STATUS_PORT


# Configure incoming status data on serial port to be forwarded to STATUS_UDP_PORT
fileevent $serialId readable [list ProcessStatusData $serialId] 

	
while {1} {
	
	set waitVar 0
	after 1000 {set waitVar 1}
	vwait waitVar
}
