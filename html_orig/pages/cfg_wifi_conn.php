<h1><?= tr('menu.cfg_wifi_conn') ?></h1>

<div class="Box">
	<p><b><?= tr('wifi.conn.status') ?></b> <span id="status"></span><span class="anim-dots">.</span></p>
	<a href="<?= e(url('cfg_wifi')) ?>" id="backbtn" class="button"><?= tr('wifi.conn.back_to_config') ?></a>
</div>

<script>
	var xhr = new XMLHttpRequest();
	var abortTmeo;

	var messages = <?= json_encode([
		'idle' => tr('wifi.conn.idle'),
		'success' => tr('wifi.conn.success'),
		'working' => tr('wifi.conn.working'),
		'fail' => tr('wifi.conn.fail'),
	]) ?>;

	function getStatus() {
		xhr.open("GET", 'http://'+_root+'<?= url('wifi_connstatus', true) ?>');
		xhr.onreadystatechange = function () {
			if (xhr.readyState == 4 && xhr.status >= 200 && xhr.status < 300) {
				clearTimeout(abortTmeo);
				var data = JSON.parse(xhr.responseText);
				var done = false;
				var msg = messages[data.status] || '...';
				if (data.status == 'success') msg += data.ip;

				$("#status").html(msg);

				if (done) {
//						$('#backbtn').removeClass('hidden');
					$('.anim-dots').addClass('hidden');
				} else {
					window.setTimeout(getStatus, 1000);
				}
			}
		};

		abortTmeo = setTimeout(function () {
			xhr.abort();
			$("#status").html(<?= json_encode(tr('wifi.conn.telemetry_lost')) ?>);
//				$('#backbtn').removeClass('hidden');
			$('.anim-dots').addClass('hidden');
		}, 4000);

		xhr.send();
	}

	getStatus();
</script>
