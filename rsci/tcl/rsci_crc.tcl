package provide rsci_crc 1.0

proc RS_CRCGenerator {buffer polynome} {

    # Calculates 16-bit CCITT  CIR

    set crc 0xffff
    
    set crc_data_length [string length $buffer]
    
    for {set i 0} {$i<$crc_data_length} {incr i} {

	set crc [expr 0xffff & ((0xff & ($crc >> 8)) | ($crc << 8))]
	binary scan [string range $buffer $i $i] "c" byte
	set byte [expr ($byte+0x100)%0x100]
	set crc [expr $crc ^ $byte]
	set crc [expr $crc ^ (0xffff & ((0xff & ($crc & 0xff)) >> 4))]
	set crc [expr $crc ^ (0xffff & (($crc << 8) << 4))]
	set crc [expr $crc ^ (0xffff & ((($crc & 0xff) << 4) << 1))]
    }
    
    if {$crc == 0x0000} {
	set crc 0x5555
    }
    
    return $crc
}
