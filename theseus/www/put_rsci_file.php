<?php


/* Put an rsci file */
try {
	$rsci_base = "/var/www/rsci";
	header('Content-type: text/plain');

	require_once("../connect.php");

	if(isset($_GET['filename']))
	{
      	 $filename = $_GET['filename'];
	}
	else
	{
	  print("Filename not specified<br/>");
	  exit;
	}

	// example filename xxxxxxxxxxBBCtst_2010-10-20_03-15-01_00000000.rsA
	if (!preg_match('/^(.{10})(.{6})_(\d{4}-\d{2}-\d{2})_(\d{2}-\d{2}-\d{2})_(\d{8})\.rs(.)/',$filename,$matches))
	{
		print("Filename $filename not in expected format<br/>");
		exit;
	}
	$rxType = $matches[1];
	$id = $matches[2];
	$startDate = $matches[3];
	$startTime = $matches[4];
	$frequency = $matches[5];
	$profile = $matches[6];

	$destDir = "$rsci_base/$id/$startDate";
	if (!file_exists($destDir))
	{
		mkdir($destDir,0755,true);
	}
	$rsciFilename = "$destDir/$filename";
	$rsciFile = fopen($rsciFilename,"w");

	$putdata = fopen("php://input", "r");

	while (!feof($putdata)) {
		$data = fread($putdata, 8192);
		fwrite($rsciFile, $data);
	}

	fclose($putdata);
	fclose($rsciFile);


        $putdata = fopen("php://input", "r");

        $query = "UPDATE rsci_recordings SET state='A' WHERE filename='$filename'";

        $count = $dbh->exec($query);

        print "Affected rows: $count<BR>";

        $dbh = null;

} catch (PDOException $e) {
    print "Error!: " . $e->getMessage() . "<br/>";
    die();
}



?>
