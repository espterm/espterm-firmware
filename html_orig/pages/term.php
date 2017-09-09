<script>
	// Workaround for badly loaded page
	setTimeout(function() {
		if (typeof termInit == 'undefined' || typeof $ == 'undefined') {
			console.error("Page load failed, refreshing…");
			location.reload(true);
		}
	}, 3000);
</script>

<div class="Modal light hidden" id="fu_modal">
	<div id="fu_form" class="Dialog">
		<div class="fu-content">
			<h2>Text Upload</h2>
			<p>
				<label for="fu_file">Load a text file:</label>
				<input type="file" id="fu_file" accept="text/*" /><br>
				<textarea id="fu_text"></textarea>
			</p>
			<p>
				<label for="fu_crlf">Line Endings:</label>
				<select id="fu_crlf">
					<option value="CR">CR (Enter key)</option>
					<option value="CRLF">CR LF (Windows)</option>
					<option value="LF">LF (Linux)</option>
				</select>
			</p>
			<p>
				<label for="fu_delay">Line Delay (ms):</label>
				<input id="fu_delay" type="number" value=1 min=0>
			</p>
		</div>
		<div class="fu-buttons">
			<button onclick="TermUpl.start()" class="icn-ok x-fu-go">Start</button>&nbsp;
			<button onclick="TermUpl.close()" class="icn-cancel x-fu-cancel">Cancel</button>&nbsp;
			<i class="fu-prog-box">Upload: <span id="fu_prog"></span></i>
		</div>
	</div>
</div>

<h1><!-- Screen title gets loaded here by JS --></h1>

<div id="term-wrap">
	<div id="screen" class="theme-%theme%"></div>

	<div id="action-buttons">
		<button data-n="1"></button><!--
		--><button data-n="2"></button><!--
		--><button data-n="3"></button><!--
		--><button data-n="4"></button><!--
		--><button data-n="5"></button>
	</div>
</div>

<textarea id="softkb-input" autocomplete="off" autocorrect="off" autocapitalize="off" spellcheck="false"></textarea>

<nav id="term-nav">
	<a href="#" onclick="kbOpen(true);return false" class="mq-tablet-max"><i class="icn-keyboard"></i><span><?= tr('term_nav.keybd') ?></span></a><!--
	--><a href="#" onclick="TermUpl.open();return false"><i class="icn-download"></i><span><?= tr('term_nav.upload') ?></span></a><!--
	--><a href="<?= url('cfg_term') ?>" class="x-term-conf-btn"><i class="icn-configure"></i><span><?= tr('term_nav.config') ?></span></a><!--
	--><a href="<?= url('cfg_wifi') ?>" class="x-term-conf-btn"><i class="icn-wifi"></i><span><?= tr('term_nav.wifi') ?></span></a><!--
	--><a href="<?= url('help') ?>" class="x-term-conf-btn"><i class="icn-help"></i><span><?= tr('term_nav.help') ?></span></a><!--
	--><a href="<?= url('about') ?>" class="x-term-conf-btn"><i class="icn-about"></i><span><?= tr('term_nav.about') ?></span></a>
</nav>

<script>
	try {
		window.noAutoShow = true;
		termInit(); // the screen will be loaded via ajax
		Screen.load('%j:labels_seq%');

		// auto-clear the input box
		$('#softkb-input').on('input', function(e) {
			setTimeout(function(){
				var str = $('#softkb-input').val();
				$('#softkb-input').val('');
				Input.sendString(str);
			}, 1);
		});
	} catch(e) {
		console.error(e);
		console.error("Fail, reloading in 3s…");
		setTimeout(function() {
			location.reload(true);
		}, 3000);
	}

	function kbOpen(yes) {
		var i = qs('#softkb-input');
		if (yes) i.focus();
		else i.blur();
	}
</script>
