<script>
	// Workaround for badly loaded page
	setTimeout(function() {
		if (typeof termInit == 'undefined' || typeof $ == 'undefined') {
			console.error("Page load failed, refreshing…");
			location.reload(true);
		}
	}, 2000);
</script>

<h1>%term_title%</h1>

<div id="termwrap">
	<div id="screen" class="theme-%theme%"></div>

	<div id="buttons">
		<button data-n="1" class="btn-blue">%btn1%</button><!--
		--><button data-n="2" class="btn-blue">%btn2%</button><!--
		--><button data-n="3" class="btn-blue">%btn3%</button><!--
		--><button data-n="4" class="btn-blue">%btn4%</button><!--
		--><button data-n="5" class="btn-blue">%btn5%</button>
	</div>
</div>

<input id="softkb-input" autocomplete="off" autocorrect="off" autocapitalize="off" spellcheck="false">

<nav id="botnav">
	<a href="#" onclick="toggleSoftKb(true); return false" class="icn-keyboard mq-tablet-max"></a><!--
	--><a href="<?= url('cfg_wifi') ?>"><?= tr('menu.settings') ?></a><!--
	--><a href="<?= url('help') ?>">Help</a><!--
	--><a href="<?= url('about') ?>">About</a>
</nav>

<script>
	// TODO cleanup
	try {
		termInit("%screenData%");

		// auto-clear the input box
		$('#softkb-input').on('input', function(e) {
			setTimeout(function(){$('#softkb-input').val('');}, 1);
		});
	} catch(e) {
		console.error(e);
		console.error("Fail, reloading…");
		location.reload(true);
	}

	function toggleSoftKb(yes) {
		qs('#softkb-input')[yes ? 'focus' : 'blur']();
		qs('.icn-keyboard').blur();
	}
</script>
