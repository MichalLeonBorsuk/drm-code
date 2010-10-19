<?php
set_time_limit(0);
ob_implicit_flush();
//ob_start();
$receiver_serial_number = $_GET['receiver'];
$receiver_serial_number .= "\n";

$address = '127.0.0.1';
$port = 12000;

if (($socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP)) === false)
{
   die("socket_create() failed: reason: " .
   socket_strerror(socket_last_error()) . "\n");
}

if (socket_connect($socket, $address, $port) === false)
{
   print "socket_connect() failed on " . $address . ":" . $port . "\n";
}

//send serial number of receiver to central script so if knows which one to dish out
socket_write($socket, $receiver_serial_number, strlen($receiver_serial_number));

header('Content-Type: application/octet-stream');
//header('HTTP/1.0 200 OK');
//header('Content-Type: text/html');
header('Pragma: no-cache');
//header('Content-Length: 200');
//don't try to use chunked transfer, seems to happen implicitly and forcing it isn't compatible with squid on www-cache.rd
//header('Transfer-Coding: Chunked');
//header('Content-Type: image/jpeg');

header('Cache-Control: no-cache, must-revalidate'); // HTTP/1.1
header('Expires: Mon, 26 Jul 1997 05:00:00 GMT'); // Date in the past

$chunk_size = 1024;

while (($buffer = socket_read($socket, $chunk_size, PHP_BINARY_READ)))
{
        echo $buffer;
}

socket_close($socket);
?>

