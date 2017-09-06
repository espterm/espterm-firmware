<form class="Box str mobcol" action="<?= e(url('wifi_set')) ?>" method="GET" id="form-1">
	<h2 tabindex=0><?= tr('wifi.ap') ?></h2>

	<div class="Row checkbox x-ap-toggle">
		<label><?= tr('wifi.enable') ?></label><!--
		--><span class="box" tabindex=0></span>
		<input type="hidden" id="ap_enabled" name="ap_enable" value="%ap_enable%">
	</div>

	<div class="Row x-ap-on">
		<label for="ap_ssid"><?= tr('wifi.ap_ssid') ?></label>
		<input type="text" name="ap_ssid" id="ap_ssid" value="%h:ap_ssid%" required>
	</div>

	<div class="Row x-ap-on">
		<label for="ap_password"><?= tr('wifi.ap_password') ?></label>
		<input type="text" name="ap_password" id="ap_password" value="%h:ap_password%">
	</div>

	<div class="Row x-ap-on">
		<label for="ap_channel"><?= tr('wifi.ap_channel') ?></label>
		<input type="number" name="ap_channel" id="ap_channel" min=1 max=14 value="%ap_channel%" required>
	</div>

	<div class="Row range x-ap-on">
		<label for="tpw">
			<?= tr('wifi.tpw') ?>
			<span class="display x-disp1 mq-phone"></span>
		</label>
		<input type="range" name="tpw" id="tpw" step=1 min=0 max=82 value="%tpw%">
		<span class="display x-disp2 mq-no-phone"></span>
	</div>

	<div class="Row checkbox x-ap-on">
		<label><?= tr('wifi.ap_hidden') ?></label><!--
		--><span class="box" tabindex=0></span>
		<input type="hidden" name="ap_hidden" value="%ap_hidden%">
	</div>

	<div class="Row buttons">
		<a class="button icn-ok" href="#" onclick="qs('#form-1').submit()"><?= tr('apply') ?></a>
	</div>
</form>

<form class="Box str mobcol expanded" action="<?= e(url('wifi_set')) ?>" method="GET" id="form-2">
	<h2 tabindex=0><?= tr('wifi.sta') ?></h2>

	<div class="Row checkbox x-sta-toggle">
		<label><?= tr('wifi.enable') ?></label><!--
		--><span class="box" tabindex=0></span>
		<input type="hidden" id="sta_enabled" name="sta_enable" value="%sta_enable%">
	</div>

	<div class="Row explain nomargintop x-sta-on">
		<span class="spacer"></span>
		<?= tr("wifi.sta_explain") ?>
	</div>

	<input type="hidden" name="sta_ssid" id="sta_ssid" value="">
	<input type="hidden" name="sta_password" id="sta_password" value="">

	<div class="Row sta-info x-sta-on">
		<label><?= tr('wifi.sta_info') ?></label>
		<div class="AP-preview hidden" id="sta-nw">
			<div class="wrap">
				<div class="inner">
					<div class="essid"></div>
					<div class="passwd"><?= tr('wifi.sta_active_pw') ?></div>
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

	<div id="ap-box" class="x-sta-on">
		<label><?= tr('wifi.select_ssid') ?></label>
		<div id="ap-scan"><a href="#" onclick="WiFi.startScanning(); return false"><?= tr('wifi.scan_now') ?></a></div>
		<div id="ap-loader" class="hidden"><?= tr('wifi.scanning') ?><span class="anim-dots">.</span></div>
		<div id="ap-list" class="hidden"></div>
	</div>

	<div class="Row buttons">
		<a class="button icn-ok" href="#" onclick="qs('#form-2').submit()"><?= tr('apply') ?></a>
	</div>
</form>

<script>
	WiFi.scan_url = '<?= url('wifi_scan', true) ?>';
	WiFi.init({
		sta_ssid: '%j:sta_ssid%',
		sta_password: '%j:sta_password%',
		sta_active_ip: '%j:sta_active_ip%',
		sta_active_ssid: '%j:sta_active_ssid%',
	});

	function updateApDisp() {
		var a = !!parseInt($('#ap_enabled').val());
		$('.x-ap-on').toggleClass('hidden', !a);
	}
	$('.x-ap-toggle').on('click', function() {
		setTimeout(function() {
			updateApDisp();
		}, 0)
	});

	function updateStaDisp() {
		var a = !!parseInt($('#sta_enabled').val());
		$('.x-sta-on').toggleClass('hidden', !a);
	}
	$('.x-sta-toggle').on('click', function() {
		setTimeout(function() {
			updateStaDisp();
		}, 0)
	});

	updateApDisp();
	updateStaDisp();
</script>
