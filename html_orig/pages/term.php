<script>
	// Workaround for badly loaded page
	setTimeout(function() {
		if (typeof termInit == 'undefined' || typeof $ == 'undefined') {
			console.error("Page load failed, refreshing…");
			location.reload(true);
		}
	}, 3000);
</script>

<h1><!-- Screen title gets loaded here by JS --></h1>

<div id="term-wrap">
	<div id="screen" class="theme-%theme%"></div>

	<div id="action-buttons">
		<button data-n="1" class="btn-blue"></button><!--
		--><button data-n="2" class="btn-blue"></button><!--
		--><button data-n="3" class="btn-blue"></button><!--
		--><button data-n="4" class="btn-blue"></button><!--
		--><button data-n="5" class="btn-blue"></button>
	</div>
</div>

<textarea id="softkb-input" autocomplete="off" autocorrect="off" autocapitalize="off" spellcheck="false"></textarea>

<nav id="term-nav">
	<a href="#" onclick="toggleSoftKb(true); return false" class="icn-keyboard mq-tablet-max"></a><!--
	--><a href="<?= url('cfg_term') ?>" class="x-term-conf-btn"><?= tr('term_nav.config') ?></a><!--
	--><a href="<?= url('cfg_wifi') ?>" class="x-term-conf-btn"><?= tr('term_nav.wifi') ?></a><!--
	--><a href="<?= url('help') ?>" class="x-term-conf-btn"><?= tr('term_nav.help') ?></a><!--
	--><a href="<?= url('about') ?>" class="x-term-conf-btn"><?= tr('term_nav.about') ?></a>
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

	function toggleSoftKb(yes) {
		var i = qs('#softkb-input');
		if (yes) i.focus();
		else i.blur();
	}
</script>
