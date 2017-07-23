<form class="Box str mobcol" action="<?= e(url('wifi_set')) ?>" method="GET" id="form-1">
	<h2 tabindex=0><?= tr('wifi.ap') ?></h2>

	<div class="Row checkbox">
		<label><?= tr('wifi.enable') ?></label><!--
		--><span class="box" tabindex=0></span>
		<input type="hidden" name="ap_enable" value="%ap_enable%">
	</div>

	<div class="Row">
		<label for="ap_ssid"><?= tr('wifi.ap_ssid') ?></label>
		<input type="text" name="ap_ssid" id="ap_ssid" value="%ap_ssid%" required>
	</div>

	<div class="Row">
		<label for="ap_password"><?= tr('wifi.ap_password') ?></label>
		<input type="text" name="ap_password" id="ap_password" value="%ap_password%">
	</div>

	<div class="Row">
		<label for="ap_channel"><?= tr('wifi.ap_channel') ?></label>
		<input type="number" name="ap_channel" id="ap_channel" min=1 max=14 value="%ap_channel%" required>
	</div>

	<div class="Row range">
		<label for="tpw">
			<?= tr('wifi.tpw') ?>
			<span class="display x-disp1 mq-phone"></span>
		</label>
		<input type="range" name="tpw" id="tpw" step=1 min=0 max=82 value="%tpw%">
		<span class="display x-disp2 mq-no-phone"></span>
	</div>

	<div class="Row checkbox">
		<label><?= tr('wifi.ap_hidden') ?></label><!--
		--><span class="box" tabindex=0></span>
		<input type="hidden" name="ap_hidden" value="%ap_hidden%">
	</div>

	<div class="Row buttons">
		<a class="button icn-ok" href="#" onclick="qs('#form-1').submit()"><?= tr('apply') ?></a>
	</div>
</form>

<form class="Box str mobcol" action="<?= e(url('wifi_set')) ?>" method="GET" id="form-2">
	<h2 tabindex=0><?= tr('wifi.sta') ?></h2>

	<div class="Row checkbox">
		<label><?= tr('wifi.enable') ?></label><!--
		--><span class="box" tabindex=0></span>
		<input type="hidden" name="sta_enable" value="%sta_enable%">
	</div>

	<input type="hidden" name="sta_ssid" id="sta_ssid" value="">
	<input type="hidden" name="sta_password" id="sta_password" value="">

	<div class="Row sta-info">
		<label><?= tr('wifi.sta_info') ?></label>
		<div class="AP-preview hidden" id="sta-nw">
			<div class="wrap">
				<div class="inner">
					<div class="essid"></div>
					<div class="passwd"><?= tr('wifi.sta_active_pw') ?>&nbsp;<span class="x-passwd"></span></div>
					<div class="nopasswd"><?= tr('wifi.sta_active_nopw') ?></div>
					<div class="ip"></div>
				</div>
				<a class="forget" href="#" id="forget-sta">×</a>
			</div>
		</div>
		<div class="AP-preview-nil" id="sta-nw-nil">
			<?= tr('wifi.sta_none') ?>
		</div>
	</div>

	<div id="ap-box">
		<label><?= tr('wifi.select_ssid') ?></label>
		<div id="ap-scan"><a href="#" onclick="startScanning(); return false"><?= tr('wifi.scan_now') ?></a></div>
		<div id="ap-loader" class="hidden"><?= tr('wifi.scanning') ?><span class="anim-dots">.</span></div>
		<div id="ap-noscan" class="hidden"><?= tr('wifi.cant_scan_no_sta') ?></div>
		<div id="ap-list" class="hidden"></div>
	</div>

	<div class="Row buttons">
		<a class="button icn-ok" href="#" onclick="qs('#form-2').submit()"><?= tr('apply') ?></a>
	</div>
</form>

<div class="Modal hidden" id="psk-modal">
	<div class="Dialog">
		<form id="conn-form" onsubmit="submitPskModal(); return false;">
			<input type="hidden" id="conn-ssid"><!--
			--><label for="conn-passwd"><?= tr('wifi.sta_password') ?></label><!--
			--><input type="text" id="conn-passwd"><!--
			--><input type="submit" value="<?= tr('confirm') ?>">
		</form>
	</div>
</div>

<script>
	// Get XX % for a slider input
	function rangePt(inp) {
		return Math.round(((inp.value / inp.max)*100)) + '%';
	}

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
		$('#sta-nw .ip').html(ip.length>0 ? '<?=tr('wifi.connected_ip_is')?>'+ip : '<?=tr('wifi.not_conn')?>');
	}

	selectSta('%sta_ssid%', '%sta_password%', '%sta_active_ip%');

	var authStr = ['Open', 'WEP', 'WPA', 'WPA2', 'WPA/WPA2'];
	var curSSID = '%sta_active_ssid%';

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
		$.get('http://'+_root+'<?= url('wifi_scan', true) ?>', onScan);
	}

	function rescan(time) {
		setTimeout(scanAPs, time);
	}

	/** Set up the WiFi page */
	function wifiInit(obj) {
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
		obj.mode = +obj.mode;

		$('#ap-noscan').toggleClass('hidden', obj.mode != 2);
		$('#ap-scan').toggleClass('hidden', obj.mode == 2);

//		// scan if not AP
//		if (obj.mode != 2) {
//			scanAPs();
//		}
	}

	wifiInit({mode: '%opmode%'});
</script>
