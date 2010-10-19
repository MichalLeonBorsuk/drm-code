<?php

if(isset($_GET['filename']))
{
        $fileName = $_GET['filename'];
} else {
	exit;
}


/* Change the state of an entry in 'rsci_recordings' table of the theseus database from "Remote" to "reQuested" */
try {
	header('Content-type: text/plain');

	require_once("../connect.php");
	
	$putdata = fopen("php://input", "r");

	$query = "UPDATE rsci_recordings SET state='Q' WHERE filename='$fileName'"; 

	$count = $dbh->exec($query);

	print "Affected rows: $count<BR>";

	$dbh = null;
	fclose($putdata);
} catch (PDOException $e) {
    print "Error!: " . $e->getMessage() . "<br/>";
    die();
}



?>
