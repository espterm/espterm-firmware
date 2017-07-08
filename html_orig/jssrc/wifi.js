/** Wifi page */
(function () {
	var authStr = ['Open', 'WEP', 'WPA', 'WPA2', 'WPA/WPA2'];
	var curSSID;

	/** Update display for received response */
	function onScan(resp, status) {
		if (status != 200) {
			// bad response
			rescan(5000); // wait 5sm then retry
			return;
		}

		resp = JSON.parse(resp);

		var done = !bool(resp.result.inProgress) && (resp.result.APs.length > 0);
		rescan(done ? 15000 : 1000);
		if (!done) return; // no redraw yet

		// clear the AP list
		var $list = $('#ap-list');
		// remove old APs
		$('.AP').remove();

		$list.toggle(done);
		$('#ap-loader').toggle(!done);

		// scan done
		resp.result.APs.sort(function (a, b) {
				return b.rssi - a.rssi;
			}).forEach(function (ap) {
				ap.enc = intval(ap.enc);

				if (ap.enc > 4) return; // hide unsupported auths

				var item = document.createElement('div');

				var $item = $(item)
					.data('ssid', ap.essid)
					.data('pwd', ap.enc != 0)
					.addClass('AP');

				// mark current SSID
				if (ap.essid == curSSID) {
					$item.addClass('selected');
				}

				var inner = document.createElement('div');
				$(inner).addClass('inner')
					.htmlAppend('<div class="rssi">{0}</div>'.format(ap.rssi_perc))
					.htmlAppend('<div class="essid" title="{0}">{0}</div>'.format($.htmlEscape(ap.essid)))
					.htmlAppend('<div class="auth">{0}</div>'.format(authStr[ap.enc]));

				$item.on('click', function () {
					var $th = $(this);

					// populate the form
					$('#conn-essid').val($th.data('ssid'));
					$('#conn-passwd').val(''); // clear

					if ($th.data('pwd')) {
						// this AP needs a password
						Modal.show('#psk-modal');
					} else {
						Modal.show('#reset-modal');
						$('#conn-form').submit();
					}
				});


				item.appendChild(inner);
				$list[0].appendChild(item);
			});
	}

	/** Ask the CGI what APs are visible (async) */
	function scanAPs() {
		$.get('http://'+_root+'/wifi/scan', onScan);
	}

	function rescan(time) {
		setTimeout(scanAPs, time);
	}

	/** Set up the WiFi page */
	window.wifiInit = function (obj) {
		//var ap_json = {
		//	"result": {
		//		"inProgress": "0",
		//		"APs": [
		//			{"essid": "Chlivek", "bssid": "88:f7:c7:52:b3:99", "rssi": "204", "enc": "4", "channel": "1"},
		//			{"essid": "TyNikdy", "bssid": "5c:f4:ab:0d:f1:1b", "rssi": "164", "enc": "3", "channel": "1"},
		//		]
		//	}
		//};

		// Hide what should be hidden in this mode
		$('.x-hide-'+obj.mode).addClass('hidden');
		obj.mode = +obj.mode;

		// Channel writable only in AP mode
		if (obj.mode != 2) $('#channel').attr('readonly', 1);

		curSSID = obj.staSSID;

		// add SSID to the opmode field
		if (curSSID) {
			var box = $('#opmodebox');
			box.html(box.html() + ' (' + curSSID + ')');
		}

		// hide IP if IP not received
		if (!obj.staIP) $('.x-hide-noip').addClass('hidden');

		// scan if not AP
		if (obj.mode != 2) {
			scanAPs();
		}

		$('#modeswitch').html([
			'<a class="button" href="/wifi/set?opmode=3">Client+AP</a>&nbsp;<a class="button" href="/wifi/set?opmode=2">AP only</a>',
			'<a class="button" href="/wifi/set?opmode=3">Client+AP</a>',
			'<a class="button" href="/wifi/set?opmode=1">Client only</a>&nbsp;<a class="button" href="/wifi/set?opmode=2">AP only</a>'
		][obj.mode-1]);
	};

	window.wifiConn = function () {
		var xhr = new XMLHttpRequest();
		var abortTmeo;

		function getStatus() {
			xhr.open("GET", 'http://'+_root+"/wifi/connstatus");
			xhr.onreadystatechange = function () {
				if (xhr.readyState == 4 && xhr.status >= 200 && xhr.status < 300) {
					clearTimeout(abortTmeo);
					var data = JSON.parse(xhr.responseText);
					var done = false;
					var msg = '...';

					if (data.status == "idle") {
						msg = "Preparing to connect";
					}
					else if (data.status == "success") {
						msg = "Connected! Received IP " + data.ip + ".";
						done = true;
					}
					else if (data.status == "working") {
						msg = "Connecting to selected AP";
					}
					else if (data.status == "fail") {
						msg = "Connection failed, check your password and try again.";
						done = true;
					}

					$("#status").html(msg);

					if (done) {
						$('#backbtn').removeClass('hidden');
						$('.anim-dots').addClass('hidden');
					} else {
						window.setTimeout(getStatus, 1000);
					}
				}
			};

			abortTmeo = setTimeout(function () {
				xhr.abort();
				$("#status").html("Telemetry lost, try reconnecting to the AP.");
				$('#backbtn').removeClass('hidden');
				$('.anim-dots').addClass('hidden');
			}, 4000);

			xhr.send();
		}

		getStatus();
	};
})();
