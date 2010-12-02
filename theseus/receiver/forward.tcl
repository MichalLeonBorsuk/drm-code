set config_dir [file dirname $argv0]
source [file join $config_dir "settings.tcl"]

# Include support for UDP
package require udp



# connect to host or ip on port through the SOCKS4(a) proxy. authenticate with username (default empty)
 # returns a socket that can be used to send and receive traffic from the remote host

 proc socks4connect {proxy_host proxy_port host port {username {}}} {
	set connect_request \x04\x01
	append connect_request [binary format S $port]
	if {[regexp {[0-9]+.[0-9]+.[0-9]+.[0-9]+} $host]} {
		set use_host false
	        append connect_request [binary format c4 [split $host .]]
	} else {
		# ip address 0.0.0.x is always invalid and signals that a hostname is specified
		set use_host true
		append connect_request [binary format c4 [split 0.0.0.1 .]]
	}
	append connect_request $username
	append connect_request \x00
	if {$use_host} {
		append connect_request $host
		append connect_request \x00
	}

	set s [socket $proxy_host $proxy_port]
	fconfigure $s -translation binary -buffering none
        puts -nonewline $s $connect_request

	set response [read $s 8]
	if {[string index $response 1] ne "\x5a"} {
		error "connection request rejected by proxy"
        } else {
		return $s
	}
 }

proc ConnectToServer {} {

    global SERVER_ADDRESS
    global SERVER_PORT
    global PROXY_ADDRESS
    global PROXY_PORT

    set result 1
    while {$result != 0} {
	if {[info exists PROXY_ADDRESS]} {
		set result [catch {set tty [socks4connect $PROXY_ADDRESS $PROXY_PORT $SERVER_ADDRESS $SERVER_PORT]}]
	} else {
		set result [catch {set tty [socket $SERVER_ADDRESS $SERVER_PORT]}]
	}
	syslog "info" "socket returned $result"
 
    }
	
    fconfigure $tty -translation binary 
    fconfigure $tty -blocking 0

    return $tty
}

proc ConfigureStatusPort {port} {

    # Configures connection for incoming status data.
    # File id is returned

    set tty [udp_open $port]
    fconfigure $tty -buffering none -translation binary
    fconfigure $tty -blocking 1
    syslog "info" "fetching RSCI from UDP port $port"
    return $tty
}

proc ConfigureControlPort {addr port} {
	set tty [udp_open]
	fconfigure $tty -remote [list $addr $port]
	fconfigure $tty -buffering none -translation binary
	syslog "info" "controlling receiver on addr $addr UDP port $port"
	return $tty
}

proc ProcessStatus {serverPort rxStatusPort} {
	set packet [read $rxStatusPort]
	syslog "info" "got status packet length [string length $packet]"
	if [catch {puts -nonewline $serverPort $packet}] {
		syslog "notice" "server port can't be written to - trying to reconnect"
		catch {close $serverPort}
		set serverPort [ConnectToServer]	
		fileevent $serverPort readable [list ProcessControl $serverPort $rxControlPort]
	}
	flush $serverPort
}

proc ProcessControl {serverPort rxControlPort} {
	syslog "info" "server readable"
	if {[eof $serverPort]} {
		syslog "notice" "EOF on server port - trying to reconnect"
		catch {close $serverPort}
		set serverPort [ConnectToServer]
		fileevent $serverPort readable [list ProcessControl $serverPort $rxControlPort]
		return
	}
	if {[catch {set packet [read $serverPort]}] != 0} {
		syslog "notice" "server port can't be read - trying to reconnect"
		catch {close $serverPort}
		set serverPort [ConnectToServer]
		fileevent $serverPort readable [list ProcessControl $serverPort $rxControlPort]
		return
	}
	syslog "info" "got control packet length [string length $packet]"	
	puts -nonewline $rxControlPort $packet
}

proc ServerWritable {serverPort} {

	syslog "notice" "Server port became writeable"
	set serverPortReady 1
	# don't call me again!
	fileevent $serverPort writable ""
}

# main program

syslog "notice" "configuring status port"
set rxStatusPort [ConfigureStatusPort $FORWARD_STATUS_PORT]
syslog "notice" "configuring control port"
set rxControlPort [ConfigureControlPort $RECEIVER_ADDRESS $FORWARD_CONTROL_PORT]
syslog "notice" "connecting to server"
set serverPort [ConnectToServer]
set serverPortReady 0

fileevent $rxStatusPort readable [list ProcessStatus $serverPort $rxStatusPort]
fileevent $serverPort readable [list ProcessControl $serverPort $rxControlPort]
# fileevent $serverPort writable [list ServerWritable $serverPort] # totally useless

while {1} {
   
    # Wait 100 ms
    set flag 0
    after 1000 {set flag 1}
    vwait flag
    
}
