<?php
	function get_recordings($dbh, $id, $path)
	{
		switch(count($path))
		{
		case 1: // get list of years 
			$stmt = $dbh->prepare("SELECT DISTINCT YEAR(recording_start) AS date FROM rsci_recordings WHERE rx_id = ? ORDER BY date ASC");
			$stmt->setFetchMode(PDO::FETCH_ASSOC);
			$stmt->execute(array($id));
			$rxValues = $stmt->fetchAll();
			print "<ul>";
			foreach($rxValues as $row)
			{
				$date = $row["date"];
				print "<li><a href=\"/rx/$id/recordings/$date/\">$date</a></li>\n";
			}
			print "</ul>";
			break;
		case 2: // get list of recordings for a year
			$year = $path[0];
			$stmt = $dbh->prepare("SELECT DISTINCT DATE(recording_start) AS date FROM rsci_recordings WHERE rx_id = ? AND recording_start BETWEEN ? AND ? ORDER BY date ASC");
			$stmt->setFetchMode(PDO::FETCH_ASSOC);
			$stmt->execute(array($id, "$year-01-01", "$year-12-31"));
			$rxValues = $stmt->fetchAll();
			print "<ul>";
			foreach($rxValues as $row)
			{
				$date = $row["date"];
				print "<li><a href=\"/rx/$id/recordings/$year/$date/\">$date</a></li>\n";
			}
			print "</ul>";
			break;
		case 3: // get list of recordings for a day 
			$year = $path[0];
			$day = $path[1];
			$stmt = $dbh->prepare("SELECT filename FROM rsci_recordings WHERE rx_id = ? AND DATE(recording_start)=?");
			$stmt->setFetchMode(PDO::FETCH_ASSOC);
			$stmt->execute(array($id, $day));
			$rxValues = $stmt->fetchAll();
			print "<ul>";
			foreach($rxValues as $row)
			{
				$filename = $row["filename"];
				print "<li><a href=\"/rx/$id/recordings/$year/$day/$filename/\">$filename</a></li>\n";
			}
			print "</ul>";
			break;
		}
	}

	function get_receivers($dbh, $path)
	{
		//$stmt = $dbh->prepare("SELECT DISTINCT rx_id FROM rsci_recordings WHERE CAST(recording_start AS DATE) = ?");
		$stmt = $dbh->prepare("SELECT DISTINCT rx_id FROM rsci_recordings");
		$stmt->setFetchMode(PDO::FETCH_ASSOC);
		$stmt->execute();
		$rxValues = $stmt->fetchAll();
		$rx = array();
		foreach($rxValues as $row)
		{
			$rx[] = $row["rx_id"];
		}
		//print_r($rxValues);
		switch(count($path))
		{
		case 1: // get list of receivers
			print "<ul>";
			foreach($rx as $id)
			{
				print "<li><a href=\"/rx/$id/\">$id</a></li>\n";
			}
			print "</ul>";
			break;
		case 2: // get list of properties for this receiver 
			$id = $path[0];
			print $id;
			if(in_array($id, $rx))
			{
				print "<ul>";
				print "<li><a href=\"/rx/$id/recordings/\">recordings</a></li>\n";
				print "<li><a href=\"/rx/$id/location/\">location</a></li>\n";
				print "</ul>";
			}
			break;
		default: // get property stuff
			$id = $path[0];
			switch($path[1])
			{
			case "recordings":
				get_recordings($dbh, $id, array_slice($path,2));
				break;
			case "location":
				print "unknown";
				break;
			}
		}
	}

	function put_receivers($dbh, $path)
	{
		print_r($path);
	}

	require_once("../connect.php");
	$method = $_SERVER["REQUEST_METHOD"];
	$path = array_slice(explode("/", $_SERVER["REQUEST_URI"]),1);
	switch($method)
	{
	case "GET":
		switch($path[0])
		{
		case "rx":
			get_receivers($dbh, array_slice($path,1));
			break;
		default:
			print "Not known"; // TODO return 404 ?
		}
		break;
	case "POST":
		break;
	case "PUT":
		switch($path[0])
		{
		case "rx":
			put_receivers($dbh, array_slice($path,1));
			break;
		default:
			print "Not known"; // TODO return 404 ?
		}
		break;
	}
?>
