<!doctype html>
<html>
<head>
	<meta charset="utf-8">
	<meta name="viewport" content="width=device-width,shrink-to-fit=no,user-scalable=no,initial-scale=1.0,maximum-scale=1.0,minimum-scale=1.0">

	<title>WiFi Settings - ESP8266 Remote Terminal</title>

	<link rel="stylesheet" href="/css/app.css">
	<script src="/js/app.js"></script>
</head>
<body class="page-wifi">

<img src="/img/loader.gif" alt="Loading…" id="loader">

<h1 onclick="location.href='/'">WiFi settings</h1>

<div class="Box" id="wificonfbox">
	<table>
		<tr>
			<th>WiFi mode</th>
			<td id="opmodebox">%WiFiMode%</td>
		</tr>
		<tr class="x-hide-noip x-hide-2">
			<th>IP</th>
			<td>%StaIP%</td>
		</tr>
		<tr>
			<th>Switch to</th>
			<td id="modeswitch"></td>
		</tr>
		<tr class="x-hide-1">
			<th><label for="channel">AP channel</label></th>
			<td>
				<form action="/wifi/setchannel" method="GET">
					<input name="ch" id="channel" type="number" step=1 min=1 max=14 value="%WiFiChannel%"><!--
					--><input type="submit" value="Set" class="narrow btn-green x-hide-3">
				</form>
			</td>
		</tr>
		<tr class="x-hide-1">
			<th><label for="channel">AP name</label></th>
			<td>
				<form action="/wifi/setname" method="GET">
					<input name="name" type="text" value="%APName%"><!--
					--><input type="submit" value="Set" class="narrow btn-green">
				</form>
			</td>
		</tr>
		<tr><td colspan=2 style="white-space: normal;">
			<p>Some changes require a reboot, dropping connection. It can take a while to re-connect.</p>
			<p>
				<b>If you lose access</b>, connect GPIO0 to GND for	5 seconds to enter Client+AP mode.
				If that fails, try the UART factory reset command "<code>\e]FR\a</code>".
			<p>
		</td></tr>
	</table>
</div>

<div class="Box" id="ap-box">
	<h2>Select AP to join</h2>
	<div id="ap-loader" class="x-hide-2">Scanning<span class="anim-dots">.</span></div>
	<div id="ap-noscan" class="x-hide-1 x-hide-3">Can't scan in AP-only mode.</div>
	<div id="ap-list" style="display:none"></div>
</div>

<div class="Modal hidden" id="psk-modal">
	<div class="Dialog">
		<form action="/wifi/connect" method="post" id="conn-form">
			<input type="hidden" id="conn-essid" name="essid"><!--
			--><label for="conn-passwd">Password:</label><!--
			--><input type="password" id="conn-passwd" name="passwd"><!--
			--><input type="submit" value="Connect!">
		</form>
	</div>
</div>

<script>
	_root = window.location.host;
	wifiInit({staSSID: '%StaSSID%', staIP: '%StaIP%', mode: '%WiFiModeNum%'});
</script>
</body>
</html>
