<?php
/* post the incoming data on available recordings into the 'available' table of the theseus database */
try {
	header('Content-type: text/plain');

	$user = "root";
	$pass = "bbc";
	
	$putdata = fopen("php://input", "r");

	$dbh = new PDO('mysql:host=localhost;dbname=theseus', $user, $pass);

	while (!feof($putdata)) {
		$line_of_text = trim(fgets($putdata));
//		$line_of_text = $_GET["line"];

		$filename = basename($line_of_text);

		if ($filename == "") {
			continue;
		}

		$rx_type = substr($filename, 0, 10);
		$rx_id = substr($filename, 10, 6);
		$stored_date = substr($filename, 17, 10);
		$stored_time = substr($filename, 28, 8);
		$stored_time = str_replace("-", ":", $stored_time);
		$stored_datetime = "$stored_date $stored_time";
		
		$frequency_khz = substr($filename, 37, 8);
		$rsci_profile = substr($filename, 48, 1);
		$state = "R";

//		print "$stored_unixtime<BR>";
//                print "$rx_type $rx_id $stored_date $stored_time $frequency_khz $rsci_profile<BR>";

		//check for duplicates

		$count = $dbh->exec("INSERT INTO available(rx_id, updated, rx_type, recording_start, frequency_khz, rsci_profile, filename, state) VALUES ('$rx_id', NOW(), '$rx_type', '$stored_datetime', '$frequency_khz', '$rsci_profile', '$filename', '$state')");

                print "$line_of_text\n";
//		print "Affected rows: $count<BR>";
	}

	$dbh = null;
	fclose($putdata);
} catch (PDOException $e) {
    print "Error!: " . $e->getMessage() . "<br/>";
    die();
}



?>
