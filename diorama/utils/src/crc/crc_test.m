
load crc8_testsequence.mat
isValid = ~crc8( data_8 )

load crc16_testsequence.mat
isValid = ~crc16( data_16 )
