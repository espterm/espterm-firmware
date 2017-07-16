<form class="Box str mobcol" action="<?= e(url('wifi_set')) ?>" method="POST">
	<h2><?= tr('box.ap') ?></h2>

	<div class="Row buttons mq-phone">
		<input type="submit" value="<?= tr('wifi.submit') ?>">
	</div>

	<div class="Row checkbox">
		<label><?= tr('wifi.enable') ?></label><!--
		--><span class="box"></span>
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
		--><span class="box"></span>
		<input type="hidden" name="ap_hidden" value="%ap_hidden%">
	</div>

	<div class="Row buttons mq-no-phone">
		<input type="submit" value="<?= tr('wifi.submit') ?>">
	</div>
</form>

<form class="Box str mobcol" action="<?= e(url('wifi_set')) ?>" method="POST">
	<h2><?= tr('box.sta') ?></h2>

	<div class="Row buttons mq-phone">
		<input type="submit" value="<?= tr('wifi.submit') ?>">
	</div>

	<div class="Row checkbox">
		<label><?= tr('wifi.enable') ?></label><!--
		--><span class="box"></span>
		<input type="hidden" name="sta_enable" value="%sta_enable%">
	</div>

	<input type="hidden" name="sta_ssid" id="sta_ssid" value="%sta_ssid%">
	<input type="hidden" name="sta_password" id="sta_password" value="%sta_password%">

	<div class="Row sta-info">
		<label><?= tr('wifi.sta_info') ?></label>
		<div class="AP-preview hidden" id="sta-nw">
			<div class="wrap">
				<div class="inner">
					<div class="essid"></div>
					<div class="passwd"></div>
					<div class="ip"></div>
				</div>
				<a class="forget" id="forget-sta">×</a>
			</div>
		</div>
		<div class="AP-preview-nil" id="sta-nw-nil">
			<?= tr('wifi.sta_none') ?>
		</div>
	</div>

	<div class="Row buttons mq-no-phone">
		<input type="submit" value="<?= tr('wifi.submit') ?>">
	</div>
</form>

<script>
	$('.Row.checkbox').forEach(function(x) {
		var inp = x.querySelector('input');
		var box = x.querySelector('.box');

		$(box).toggleClass('checked', inp.value);

		$(x).on('click', function() {
			inp.value = 1 - inp.value;
			$(box).toggleClass('checked', inp.value)
		});
	});

	$('.Box.mobcol').forEach(function(x) {
		var h = x.querySelector('h2');
		$(h).on('click', function() {
			$(x).toggleClass('expanded');
		});
	});

	function rangePt(inp) {
		return Math.round(((inp.value / inp.max)*100)) + '%';
	}

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

	$('#forget-sta').on('click', function() {
		$('#sta_ssid').val('');
		$('#sta_password').val('');

		wifiShowSelected('', '', '');
	});

	function wifiShowSelected(name, password, ip) {
		$('#sta-nw').toggleClass('hidden', name.length == 0);
		$('#sta-nw-nil').toggleClass('hidden', name.length > 0);

		$('#sta-nw .essid').html(e(name));
		var nopw = undef(password) || password.length == 0;
		$('#sta-nw .passwd').html(e(password)).toggleClass('hidden', nopw);
		$('#sta-nw .ip').html(ip.length>0 ? ip : '<?=tr('wifi.not_conn')?>');
	}

	wifiShowSelected('%sta_ssid%', '%sta_password%', '%sta_active_ip%');

</script>
