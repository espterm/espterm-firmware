<!DOCTYPE html>
<html>
<head>
	<meta charset="utf-8">
	<meta name="viewport" content="width=device-width,shrink-to-fit=no,user-scalable=no,initial-scale=1.0,maximum-scale=1.0,minimum-scale=1.0">

	<title>Connecting…</title>

	<link rel="stylesheet" href="css/app.css">
	<script src="js/app.js"></script>
</head>
<body>

<h1>Connecting to network</h1>

<div class="Box">
	<p><b>Status:</b><br><span id="status"></span><span class="anim-dots">.</span></p>
	<a href="/wifi" id="backbtn" class="hidden button">Back to WiFi config</a>
</div>

<script>
	_root = window.location.host;
	wifiConn();
</script>
</body>
</html>
