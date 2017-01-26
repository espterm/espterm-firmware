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

					var ssid = $th.data('ssid');
					var pass = '';

					if ($th.data('pwd')) {
						// this AP needs a password
						pass = prompt("Password for \""+ssid+"\":");
						if (pass === null) {
							return;
						}
					}

					$.post('http://'+_root+'/wifi/connect', null, {
						data: {
							essid: ssid,
							passwd: pass
						}
					});
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

		curSSID = obj.curSSID;

		scanAPs();
	};
})();
