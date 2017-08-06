<h1><?= tr('menu.cfg_wifi_conn') ?></h1>

<div class="Box">
	<p><b><?= tr('wifi.conn.status') ?></b> <span id="status"></span><span class="anim-dots">.</span></p>
	<a href="<?= e(url('cfg_wifi')) ?>" id="backbtn" class="button"><?= tr('wifi.conn.back_to_config') ?></a>
</div>

<div class="Box">
	<p><?= tr('wifi.conn.explain_android_sucks') ?></p>
	<p><?= tr('wifi.conn.explain_reset') ?></p>
</div>

<script>
	var xhr = new XMLHttpRequest();
	var abortTmeo;
	var failCounter = 0;

	var messages = <?= json_encode([
		'disabled' => tr('wifi.conn.disabled'),
		'idle' => tr('wifi.conn.idle'),
		'success' => tr('wifi.conn.success'),
		'working' => tr('wifi.conn.working'),
		'fail' => tr('wifi.conn.fail'),
	]) ?>;

	function onFail() {
		$("#status").html(<?= json_encode(tr('wifi.conn.telemetry_lost')) ?>);
		$('.anim-dots').addClass('hidden');
	}

	function getStatus() {
		xhr.open("GET", 'http://'+_root+'<?= url('wifi_connstatus', true) ?>');
		xhr.onreadystatechange = function () {
			if (xhr.readyState == 4) {
				if (xhr.status == 200) {
					clearTimeout(abortTmeo);

					try {
						var data = JSON.parse(xhr.responseText);
						var done = false;
						var msg = messages[data.status] || '...';

						if (data.status == 'success') {
							msg += data.ip;
							done = true;
						}

						if (data.status == 'fail') {
							msg += data.cause;
							done = true;
						}

						$("#status").html(msg);

						if (done) {
//					        $('#backbtn').removeClass('hidden');
							$('.anim-dots').addClass('hidden');
						} else {
							// ask again after a short delay
							window.setTimeout(getStatus, 1000);
						}
					} catch(e) {
						failCounter++;
						console.log(e);
						// repeat
						if (failCounter > 5) {
							onFail();
						}
						else {
							window.setTimeout(getStatus, 1000);
						}
					}
				} else {
					onFail();
				}
			}
		};

		// XHR timeout
		abortTmeo = setTimeout(function () {
			xhr.abort();
			onFail();
		}, 4000);

		xhr.send();
	}

	getStatus();
</script>
