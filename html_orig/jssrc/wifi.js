(function(w) {
	var authStr = ['Open', 'WEP', 'WPA', 'WPA2', 'WPA/WPA2'];
	var curSSID;

	// Get XX % for a slider input
	function rangePt(inp) {
		return Math.round(((inp.value / inp.max)*100)) + '%';
	}

	// Display selected STA SSID etc
	function selectSta(name, password, ip) {
		$('#sta_ssid').val(name);
		$('#sta_password').val(password);

		$('#sta-nw').toggleClass('hidden', name.length == 0);
		$('#sta-nw-nil').toggleClass('hidden', name.length > 0);

		$('#sta-nw .essid').html(e(name));
		var nopw = undef(password) || password.length == 0;
		$('#sta-nw .x-passwd').html(e(password));
		$('#sta-nw .passwd').toggleClass('hidden', nopw);
		$('#sta-nw .nopasswd').toggleClass('hidden', !nopw);
		$('#sta-nw .ip').html(ip.length>0 ? tr('wifi.connected_ip_is')+ip : tr('wifi.not_conn'));
	}

	function submitPskModal(e, open) {
		var passwd = $('#conn-passwd').val();
		var ssid = $('#conn-ssid').val();

		if (open || passwd.length) {
			$('#sta_password').val(passwd);
			$('#sta_ssid').val(ssid);
			selectSta(ssid, passwd, '');
		}

		if (e) e.preventDefault();
		Modal.hide('#psk-modal');
		return false;
	}

	/** Update display for received response */
	function onScan(resp, status) {
		//var ap_json = {
		//	"result": {
		//		"inProgress": "0",
		//		"APs": [
		//			{"essid": "Chlivek", "bssid": "88:f7:c7:52:b3:99", "rssi": "204", "enc": "4", "channel": "1"},
		//			{"essid": "TyNikdy", "bssid": "5c:f4:ab:0d:f1:1b", "rssi": "164", "enc": "3", "channel": "1"},
		//		]
		//	}
		//};

		if (status != 200) {
			// bad response
			rescan(5000); // wait 5sm then retry
			return;
		}

		try {
			resp = JSON.parse(resp);
		} catch (e) {
			console.log(e);
			rescan(5000);
			return;
		}

		var done = !bool(resp.result.inProgress) && (resp.result.APs.length > 0);
		rescan(done ? 15000 : 1000);
		if (!done) return; // no redraw yet

		// clear the AP list
		var $list = $('#ap-list');
		// remove old APs
		$('#ap-list .AP').remove();

		$list.toggleClass('hidden', !done);
		$('#ap-loader').toggleClass('hidden', done);

		// scan done
		resp.result.APs.sort(function (a, b) {
			return b.rssi - a.rssi;
		}).forEach(function (ap) {
			ap.enc = parseInt(ap.enc);

			if (ap.enc > 4) return; // hide unsupported auths
			WiFi.scan_url = '/cfg/wifi/scan';

			var item = mk('div');

			var $item = $(item)
				.data('ssid', ap.essid)
				.data('pwd', ap.enc)
				.attr('tabindex', 0)
				.addClass('AP');

			// mark current SSID
			if (ap.essid == curSSID) {
				$item.addClass('selected');
			}

			var inner = mk('div');
			$(inner).addClass('inner')
				.htmlAppend('<div class="rssi">{0}</div>'.format(ap.rssi_perc))
				.htmlAppend('<div class="essid" title="{0}">{0}</div>'.format($.htmlEscape(ap.essid)))
				.htmlAppend('<div class="auth">{0}</div>'.format(authStr[ap.enc]));

			$item.on('click', function () {
				var $th = $(this);

				var ssid = $th.data('ssid');

				$('#conn-ssid').val(ssid);
				$('#conn-passwd').val('');

				if (+$th.data('pwd')) {
					// this AP needs a password
					Modal.show('#psk-modal');
					$('#conn-passwd')[0].focus();
				} else {
					//Modal.show('#reset-modal');
					submitPskModal(null, true);
				}
			});


			item.appendChild(inner);
			$list[0].appendChild(item);
		});
	}

	function startScanning() {
		$('#ap-loader').removeClass('hidden');
		$('#ap-scan').addClass('hidden');
		$('#ap-loader .anim-dots').html('.');
		scanAPs();
	}

	/** Ask the CGI what APs are visible (async) */
	function scanAPs() {
		$.get('http://'+_root+w.scan_url, onScan);
	}

	function rescan(time) {
		setTimeout(scanAPs, time);
	}

	/** Set up the WiFi page */
	function wifiInit(cfg) {
		// Hide what should be hidden in this mode
		cfg.mode = +cfg.mode;

		$('#ap-noscan').toggleClass('hidden', cfg.mode != 2);
		$('#ap-scan').toggleClass('hidden', cfg.mode == 2);

		// Update slider value displays
		$('.Row.range').forEach(function(x) {
			var inp = x.querySelector('input');
			var disp1 = x.querySelector('.x-disp1');
			var disp2 = x.querySelector('.x-disp2');
			var t = rangePt(inp);
			$(disp1).html(t);
			$(disp2).html(t);
			$(inp).on('input', function() {
				t = rangePt(inp);
				$(disp1).html(t);
				$(disp2).html(t);
			});
		});

		// Forget STA credentials
		$('#forget-sta').on('click', function() {
			selectSta('', '', '');
			return false;
		});

		selectSta(cfg.sta_ssid, cfg.sta_password, cfg.sta_active_ip);
		curSSID = cfg.sta_active_ssid;
	}

	w.init = wifiInit;
	w.startScanning = startScanning;
})(window.WiFi = {});
