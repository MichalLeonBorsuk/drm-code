# copies an RSCI or MDI multicast stream to a unicast destination, dropping duplicate packets
# Call with: tclsh mrelay.tcl 

set source_port 9998
set source_ip 233.11.2.0
set dest_port 1234
set dest_host 127.0.0.1
set dest_sock 0

set last_seq 0

package require udp

proc udpEventHandler {sock} {
    global last_seq
    global dest_sock
    set pkt [read $sock]
    set peer [fconfigure $sock -peer]
    binary scan $pkt "a2IScA" sync len seq ar pt
    if {$seq != $last_seq} {
        puts "$sync $seq"
        set last_seq $seq
        puts -nonewline $dest_sock $pkt
    }
    return
}

proc udp_listen {group port} {
    set s [udp_open $port]
    fconfigure $s -buffering none -blocking 0
    fconfigure $s -mcastadd $group -remote [list $group $port]
    fileevent $s readable [list ::udpEventHandler $s]
    puts "Listening on udp port: [fconfigure $s -myport]"
    return $s
}

proc udp_talk {host port} {
    set sock [udp_open]
    fconfigure $sock -remote [list $host $port] -buffering none -translation binary
    return $sock
}

set dest_sock [udp_talk $dest_host $dest_port]
set source_sock [udp_listen $source_ip $source_port]
vwait forever
close $source_sock
close $dest_sock
