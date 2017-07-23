<script>
	// Workaround for badly loaded page
	setTimeout(function() {
		if (typeof termInit == 'undefined' || typeof $ == 'undefined') {
			location.reload(true);
		}
	}, 2000);
</script>

<h1>%term_title%</h1>

<div id="termwrap">
	<div id="screen"></div>

	<div id="buttons">
		<button data-n="1" class="btn-blue">%btn1%</button><!--
		--><button data-n="2" class="btn-blue">%btn2%</button><!--
		--><button data-n="3" class="btn-blue">%btn3%</button><!--
		--><button data-n="4" class="btn-blue">%btn4%</button><!--
		--><button data-n="5" class="btn-blue">%btn5%</button>
	</div>
</div>

<nav id="botnav">
	<a href="<?= url('cfg_wifi') ?>"><?= tr('menu.settings') ?></a><!--
		--><a href="<?= url('help') ?>">Help</a><!--
		--><a href="<?= url('about') ?>">About</a>
</nav>

<script>
	try {
		termInit(%screenData%);
	} catch(e) {
		console.error("Fail, reloading...");
		location.reload(true);
	}
</script>
