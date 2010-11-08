#***************************************************************************************
# General settings
#***************************************************************************************

#** Receiver name
set RX_NAME "BBCtst"

set LATITUDE 51.511118
set LONGITUDE -0.119690

# Specify here, which version of the SE127 protocol the receiver provides.
# Options are: 
# 2	Version 2.x
# 3	Version 3.x
set SE127_VERSION 3
set SE127_PROTOCOL DCP
set USE_AF_CRC 0


#specifies whether to gzip dt2 files before sending    
set GZIP_DT2_FILES 0

# Specify whether data should be logged in realtime
# 1: Log data every 400 ms into ASCII log file in log/ directory (causes big files!)
# 0: Do not log data (default)
set REAL_TIME_LOG 0

# Specify whether IQ data should be recorded.
# 1: Record IQ file (takes a lot of disk space!)
# 0: Do no record IQ file (default)
set IQ_RECORD 0

# Specify whether binary SE127 data should be recorded in realtime
# 1: Binary data is written to harddisk every 400 ms into binary file in bin/ directory (causes big files!)
# 0: Do binary recording data (default)
set BINARY_RECORDING 0

# Spefify whether the computer should beep when an audio error occurs
set AFS_ERROR_BEEP 1

# Specify whether the computer should set the receiver time every 24h (1=yes, 0=no)
# Set to '0' if receiver is running on the same computer as the scripts!
set ADJUST_RECEIVER_TIME 1


# Setting connection protocol for control and status
# Thales:
# set CONTROL_PROTOCOL UDP
# set STATUS_PROTOCOL UDP

# FhG:
#set CONTROL_PROTOCOL UDP
#set STATUS_PROTOCOL UDP

# BBC:
#set CONTROL_PROTOCOL TCP
#set STATUS_PROTOCOL TCP
#set STATUS_PROTOCOL UDP 

# Dream:
set CONTROL_PROTOCOL UDP
set STATUS_PROTOCOL UDP


# Setting for communication with RSCI-compliant DRM receiver
set RECEIVER_ADDRESS 127.0.0.1
set PORT_BASE 30000
set RECORD_STATUS_PORT [expr $PORT_BASE + 1]
set RECORD_CONTROL_PORT [expr $PORT_BASE + 2]
set FORWARD_STATUS_PORT [expr $PORT_BASE + 101]
set FORWARD_CONTROL_PORT [expr $PORT_BASE + 102]

# Setting for communication with Theseus server
set SERVER_ADDRESS theseus.dyndns.ws
set SERVER_PORT 12001
#set PROXY_ADDRESS somehost
#set PROXY_PORT 1085

#** Process data on each program startup
#set PROCESS_ON_STARTUP 1

#** TCP/IP port numbers for communication between the scripts. Don't  change unless you change it in schedule.tcl as well!
set TCP_PORT_RECORDER [expr $PORT_BASE + 0]

#** Settings controlling the download for latest schedule.txt or even <RX_NAME>_schedule.txt
set AUTOMATIC_SCHEDULE_DOWNLOAD 0    		;# set to '1' to activate, '0' otherwise
set HTTP_PROXY_HOST "www-cache.rd.bbc.co.uk"    ;# adjust to your local network
set HTTP_PROXY_PORT 80				;# adjust to your local network
# set SCHEDULE_URL "http://pdis.rnw.nl/drm" ;# don't touch
#set SCHEDULE_URL "http://www.oliver.haffenden.dsl.pipex.com/drm" ;# don't touch
# set SCHEDULE_URL "http://www.geocities.com/bbcdrm/drm" ;# don't touch
#set SCHEDULE_URL "http://158.227.74.68/drm" ;# UPV
set SCHEDULE_URL "http://www.geocities.com/bbcdrm/mayflower" ;# don't touch

set SCHEDULE_FORMAT IBB
set SCHEDULE_FILENAME IBB_example.txt

set BANDSCAN_STEP_FREQ 10
set BANDSCAN_STEP_TIME 2

#** Specify here, at what GMT time you have an internet connection
# The format needs to be hh:mm:ss for both values.
set INTERNET_CONNECTION_START "00:00:00";# default is "00:00:00"
set INTERNET_CONNECTION_END "23:59:00";# default is "23:59:00"

#***************************************************************************************
# OS-specific functions
# Select the OS by uncommenting the source
#***************************************************************************************

#set OS_CODE windows.tcl
set OS_CODE unix.tcl

#***************************************************************************************
# Receiver-specific functions
# Select the receiver by uncommenting the source
#***************************************************************************************

# Here, you need to specify whether a signal strength should be recorded from 
# the DRM receiver (value: 0) or from the external source specified below (value: 1)
set USE_EXTERNAL_SIGNAL_STRENGTH 0

#** JRC NRD-535
# set RECEIVER_CODE nrd535.tcl
# set NRD535_SERIAL 59727
# set PORT /dev/ttyb

#** AOR AR-5000
#set RECEIVER_CODE aor5000.tcl
#set PORT /dev/ttya

#** Rohde & Schwarz ESMB-Receiver
# set RECEIVER_CODE esmb.tcl
# set ESMB_IP   194.55.30.220
# set ESMB_IP   132.185.130.17
# set ESMB_PORT 5555

# ICOM PCR-1000
#set RECEIVER_CODE pcr1000.tcl
#set PORT /dev/ttya

# Watkins Johnson WJ-8711
#set RECEIVER_CODE wj8711.tcl
#set PORT /dev/ttya


#***************************************************************************************
#** Thomcast receiver remote-control
#***************************************************************************************


#set THOMCAST_CTRL thomcast_ctrl.tcl
#set THCT_PORT com2



#***************************************************************************************
# Mail details
#***************************************************************************************

# Send email with datafile once that it is fully created (1). Don't send: 0
set ENABLE_EMAIL 0

# Name of the email program to be used
#set EMAIL_CLIENT "postie";# For Windows
set EMAIL_CLIENT "mpack";# For Unix

#** SMTP-server IP-address
set MAIL_SMTP pop3.rd.bbc.co.uk

#** Your e-mail address
set MAIL_FROM wsaudibility@gmail.com

#** Don't change this, destination e-mail address
set MAIL_TO drmdata@rd.bbc.co.uk

if {$USE_EXTERNAL_SIGNAL_STRENGTH == "1"} {
	if {$RECEIVER_CODE==""} { puts "No receiver specified... Exiting."; exit }
	if {[info exists RECEIVER_CODE]} { source $RECEIVER_CODE }
	if {[info exists THOMCAST_CTRL]} { source $THOMCAST_CTRL }
}

if {$OS_CODE==""} { puts "No operating system specified... Exiting."; exit }
if {[info exists OS_CODE]} { source $OS_CODE }


# Constants
set TX_FRAME_LENGTH 0.4; # Transmission frame length in s *** temporary ***
set NAPT 10; # Number of audio frames per transmission frame *** temporary ***
set DATA_BLOCK_LENGTH 60.0; # in seconds
set AGC_INTERVAL 100; # time in ms between agc measurements
set DATA_PROCESSING_TIME 300; # Time in seconds needed to process data and send it away
set AGC_DBUV_OFFSET 50; # Add this offset to AGC values in dBuV to obtain data entity values
set MIN_FADING_DEPTH 3; # Variation in field strength [in dB] required to be counted as 1 fade

# Database
set DBNAME "ft_longterm"
