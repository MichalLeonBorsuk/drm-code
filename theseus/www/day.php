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
			request_file($dbh, $date, $_GET['filename']);
		}
		break;
	default:
	}
   }

   show_archive($dbh, $date);

?>
