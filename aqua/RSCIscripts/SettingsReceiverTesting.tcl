#***************************************************************************************
# General settings
#***************************************************************************************

# Specify here, which version of the SE127 protocol the receiver provides.
# Options are: 
# 2	Version 2.x
# 3	Version 3.x
set SE127_VERSION 3
set SE127_PROTOCOL DCP

# Constants
set TX_FRAME_LENGTH 0.4; # Transmission frame length in s *** temporary ***
set NAPT 10; # Number of audio frames per transmission frame *** temporary ***
set DATA_BLOCK_LENGTH 60.0; # in seconds
set AGC_INTERVAL 100; # time in ms between agc measurements
set DATA_PROCESSING_TIME 300; # Time in seconds needed to process data and send it away
set AGC_DBUV_OFFSET 50; # Add this offset to AGC values in dBuV to obtain data entity values
set MIN_FADING_DEPTH 3; # Variation in field strength [in dB] required to be counted as 1 fade

# Set IP address of DRM receiver
#set DRM_RX_IP_ADDRESS 132.185.130.104
#set DRM_RX_IP_ADDRESS localhost
set DRM_RX_CTRL_IP_ADDRESS 132.185.131.189
set DRM_RX_STATUS_IP_ADDRESS 132.185.131.189
#set DRM_RX_CTRL_IP_ADDRESS 132.185.131.189
#set DRM_RX_STATUS_IP_ADDRESS 132.185.131.189
#set DRM_RX_CTRL_IP_ADDRESS 172.29.144.62
#set DRM_RX_STATUS_IP_ADDRESS 172.29.144.62

# set whether to use TCL-DP, "DP" or TCL-UDP, "UDP"
set UDP_PACKAGE UDP

# Dream:
set CONTROL_PROTOCOL UDP
set STATUS_PROTOCOL UDP

#BBC
#set CONTROL_PROTOCOL UDP
#set STATUS_PROTOCOL UDP

#FHG
#set CONTROL_PROTOCOL UDP
#set STATUS_PROTOCOL UDP

# Setting for communication with TC_SE127-compliant DRM receiver
set STATUS_PORT 60001
set CONTROL_PORT 60002

#** TCP/IP port numbers for communication between the scripts. Don't  change unless you change it in schedule.tcl as well!
set TCP_PORT_RECORDER 30000

#set AUDIO_FILE_PATH "AudioFiles/sweep400ms"

set AQUA_PATH ".."
set AAC_FILE_PATH "${AQUA_PATH}/AACFiles/ktestchirp50h5k2ssquaremono"

#***************************************************************************************
# OS-specific functions
# Select the OS by uncommenting the source
#***************************************************************************************

set OS_CODE windows.tcl
#set OS_CODE unix.tcl

# needed to avoid error in include.tcl
set USE_EXTERNAL_SIGNAL_STRENGTH 0