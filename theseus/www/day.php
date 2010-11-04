<html>
<head>
<meta http-equiv="refresh" content="10">
<link rel="shortcut icon" href="images/favicon.ico" type="image/vnd.microsoft.icon">
<link rel=StyleSheet href="/style.css" type="text/css">
<title>THESEUS Remote Monitoring System</title>
</head>
<body>
<center><img src="images/banner.png" width="990" height="40" border="0" vspace="0" hspace="0"></center>


<?php
   if(isset($_GET["date"]))
       $date = $_GET["date"];
   else
       $date = date("Y-m-d");

   require_once("../connect.php");
   require_once("file_functions.php");

   if(isset($_GET["action"]))
   {
	switch($_GET["action"])
	{
	case "request":
		if(isset($_GET['filename']))
		{
			request_file($dbh, $_GET['filename']);
		}
		break;
	default:
	}
   }

   show_archive($dbh, $date);

   $dbh = null;

?>

</body>
</html>
