<?php
$rxID = $_GET['rxID'];
$freq = $_GET['freq'];
$mode = $_GET['mode'];

$address = '127.0.0.1';
$port = 12002;
$command = "<cfre rxID=\"" . $rxID . "\" mode=\"" . $mode . "\">" . $freq . "</cfre>";
$len = strlen($command);

if (($socket = socket_create(AF_INET, SOCK_DGRAM, SOL_UDP)) === false)
{
   die("socket_create() failed: reason: " .
   socket_strerror(socket_last_error()) . "\n");
}

socket_sendto($socket, $command, $len, 0, $address, $port);
socket_close($socket);
header('HTTP/1.0 204 No Content');
?>
