<?php
	
	function getUserIP(){
    	$client  = @$_SERVER['HTTP_CLIENT_IP'];
    	$forward = @$_SERVER['HTTP_X_FORWARDED_FOR'];
    	$remote  = $_SERVER['REMOTE_ADDR'];

    	if(filter_var($client, FILTER_VALIDATE_IP)){
        	$ip = $client;
    	}
    	elseif(filter_var($forward, FILTER_VALIDATE_IP)){
        	$ip = $forward;
    	} else {
        	$ip = $remote;
    	}
    	return $ip;
	}
	
	$user_ip = ip2long(getUserIP());
	
	//create data arrays
	$deviceID = $_GET['id'];
	$senAir = $_GET['Air'];
	$senHum = $_GET['Hum'];
	$senTemp = $_GET['Temp'];
	$senLat = $_GET['Lat'];
	$senLng = $_GET['Lng'];
	$senAlt = $_GET['Alt'];
	$senDate = $_GET['gpsDate'];
	$senUPT = $_GET['UPT'];
	
	$senAir = explode(",", $senAir);
	$senHum = explode(",", $senHum);
	$senTemp = explode(",", $senTemp);
	$senLat = explode(",", $senLat);
	$senLng = explode(",", $senLng);
	$senAlt = explode(",", $senAlt);
	$senDate = explode(",", $senDate);
	$senUPT = explode(",", $senUPT);
	
	$link = new mysqli($dbhost, $dbuser, $dbpass, $dbname);
	
	if (mysqli_connect_errno()){
  		printf("Failed to connect to database.");
  	}
  	
	$active = 1;
	$respo = 0;
	
	$deviceCHK = "SELECT * FROM devices WHERE deviceID =".$deviceID." AND active =".$active;
	$rstCHK = mysqli_query($link,$deviceCHK);
	
	if ( mysqli_num_rows($rstCHK) != 0 ){
		for($i=0; $i<sizeof($senAir); $i++){
			$sql = "INSERT INTO sensor_".$deviceID." (deviceID, air, hum, temp, lat, lng, alt, gDate, UPT, userIP) VALUES (".$deviceID.",".$senAir[$i].",".$senHum[$i].",".$senTemp[$i].",".$senLat[$i].",".$senLng[$i].",".$senAlt[$i].",".$senDate[$i].",".$senUPT[$i].",".$user_ip.")";
			$results = mysqli_query($link,$sql);
			if ( mysqli_error($link) ){
				$respo = 1;		  
			} 
		}
		if ( $respo == 1 ){
			printf("Database error.");
		} else { 
			printf("Data upload successful.");
		}
		
	} else { 
		$regCHK = "SELECT * FROM devices WHERE deviceID = ".$deviceID;
		$rsgCHK = mysqli_query($link,$regCHK);
		if (mysqli_num_rows($rsgCHK) == 0){
			$sqlCT = "CREATE TABLE sensor_".$deviceID." (eventID INT(10) UNSIGNED AUTO_INCREMENT PRIMARY KEY, deviceID BIGINT(20) NOT NULL, air INT(4) NOT NULL, hum FLOAT(6,2) NOT NULL, temp FLOAT(6,2) NOT NULL, lat FLOAT(10,7) NOT NULL, lng FLOAT(10,7) NOT NULL, alt FLOAT(6,2) NOT NULL, UPT INT(10) UNSIGNED NOT NULL, gDate DATETIME NOT NULL, userIP INT(11) UNSIGNED NOT NULL, timestamp TIMESTAMP NOT NULL)";
			if (mysqli_query($link, $sqlCT)) {
						
				$sqlDevice = "INSERT INTO devices (deviceID, email, active, regDate) VALUES (".$deviceID.", 'DEVICE REGISTERED', 1, ".$senDate[0].")";
				$devCHK = mysqli_query($link,$sqlDevice);
				if ( mysqli_error($link) ){
					printf("Device registration failed.");		  
				} else { 
					printf("Device registration successful.");
				}
			} else {
				printf ("Device registration failed.");
			} 
		} else {
			$sqlDEN = "INSERT INTO invdev (userIP, deviceID) VALUES (".$user_ip.",x'".$deviceID."')";
			$results = mysqli_query($link,$sqlDEN);
			printf("Device: ".$deviceID." deactivated.");	
		}
	} 
?>