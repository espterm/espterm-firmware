<div class="Box">
	<a href="<?= e(url('reset_screen')) ?>"><?= tr('term.reset_screen') ?></a>
</div>

<form class="Box mobopen str" action="<?= e(url('term_set')) ?>" method="GET" id='form-1'>
	<h2><?= tr('term.defaults') ?></h2>

	<div class="Row explain">
		<?= tr('term.explain_initials') ?>
	</div>

	<div class="Row">
		<label for="theme"><?= tr("term.theme") ?></label>
		<select name="theme" id="theme" class="short" onchange="showColor()">
			<option value="0">Tango</option>
			<option value="1">Linux</option>
			<option value="2">XTerm</option>
			<option value="3">Rxvt</option>
			<option value="4">Ambience</option>
			<option value="5">Solarized</option>
		</select>
	</div>

	<div class="Row color-preview">
		<div class="colorprev">
			<span data-fg=0 class="bg0 fg0">30</span><!--
			--><span data-fg=1 class="bg0 fg1">31</span><!--
			--><span data-fg=2 class="bg0 fg2">32</span><!--
			--><span data-fg=3 class="bg0 fg3">33</span><!--
			--><span data-fg=4 class="bg0 fg4">34</span><!--
			--><span data-fg=5 class="bg0 fg5">35</span><!--
			--><span data-fg=6 class="bg0 fg6">36</span><!--
			--><span data-fg=7 class="bg0 fg7">37</span>
		</div>

		<div class="colorprev">
			<span data-fg=8 class="bg0 fg8">90</span><!--
			--><span data-fg=9 class="bg0 fg9">91</span><!--
			--><span data-fg=10 class="bg0 fg10">92</span><!--
			--><span data-fg=11 class="bg0 fg11">93</span><!--
			--><span data-fg=12 class="bg0 fg12">94</span><!--
			--><span data-fg=13 class="bg0 fg13">95</span><!--
			--><span data-fg=14 class="bg0 fg14">96</span><!--
			--><span data-fg=15 class="bg0 fg15">97</span>
		</div>

		<div class="colorprev">
			<span data-bg=0 class="bg0 fg15">40</span><!--
			--><span data-bg=1 class="bg1 fg15">41</span><!--
			--><span data-bg=2 class="bg2 fg15">42</span><!--
			--><span data-bg=3 class="bg3 fg0">43</span><!--
			--><span data-bg=4 class="bg4 fg15">44</span><!--
			--><span data-bg=5 class="bg5 fg15">45</span><!--
			--><span data-bg=6 class="bg6 fg15">46</span><!--
			--><span data-bg=7 class="bg7 fg0">47</span>
		</div>

		<div class="colorprev">
			<span data-bg=8 class="bg8 fg15">100</span><!--
			--><span data-bg=9 class="bg9 fg0">101</span><!--
			--><span data-bg=10 class="bg10 fg0">102</span><!--
			--><span data-bg=11 class="bg11 fg0">103</span><!--
			--><span data-bg=12 class="bg12 fg0">104</span><!--
			--><span data-bg=13 class="bg13 fg0">105</span><!--
			--><span data-bg=14 class="bg14 fg0">106</span><!--
			--><span data-bg=15 class="bg15 fg0">107</span>
		</div>
	</div>

	<div class="Row color-preview">
		<div style="
		" id="color-example">
			<?= tr("term.example") ?>
		</div>
	</div>

	<div class="Row">
		<label><?= tr("term.default_fg_bg") ?></label>
		<select name="default_fg" id="default_fg" class="short" onchange="showColor()">
			<?php for($i=0; $i<16; $i++): ?>
			<option value="<?=$i?>"><?= tr("color.$i") ?></option>
			<?php endfor; ?>
		</select>&nbsp;<!--
		--><select name="default_bg" id="default_bg" class="short" onchange="showColor()">
			<?php for($i=0; $i<16; $i++): ?>
				<option value="<?=$i?>"><?= tr("color.$i") ?></option>
			<?php endfor; ?>
		</select>
	</div>

	<div class="Row">
		<label for="term_width"><?= tr('term.term_width') ?></label>
		<input type="number" step=1 min=1 max=255 name="term_width" id="term_width" value="%term_width%" required>&nbsp;<!--
		--><input type="number" step=1 min=1 max=255 name="term_height" id="term_height" value="%term_height%" required>
	</div>

	<div class="Row">
		<label for="term_title"><?= tr('term.term_title') ?></label>
		<input type="text" name="term_title" id="term_title" value="%h:term_title%" required>
	</div>

	<div class="Row">
		<label><?= tr("term.buttons") ?></label>
		<input class="short" type="text" name="btn1" id="btn1" value="%h:btn1%">&nbsp;
		<input class="short" type="text" name="btn2" id="btn2" value="%h:btn2%">&nbsp;
		<input class="short" type="text" name="btn3" id="btn3" value="%h:btn3%">&nbsp;
		<input class="short" type="text" name="btn4" id="btn4" value="%h:btn4%">&nbsp;
		<input class="short" type="text" name="btn5" id="btn5" value="%h:btn5%">
	</div>

	<div class="Row buttons">
		<a class="button icn-ok" href="#" onclick="qs('#form-1').submit()"><?= tr('apply') ?></a>
	</div>
</form>

<form class="Box fold str" action="<?= e(url('term_set')) ?>" method="GET" id='form-2'>
	<h2><?= tr('term.expert') ?></h2>

	<div class="Row explain">
		<?= tr('term.explain_expert') ?>
	</div>

	<div class="Row">
		<label for="parser_tout_ms"><?= tr('term.parser_tout_ms') ?><span class="mq-phone">&nbsp;(ms)</span></label>
		<input type="number" step=1 min=0 name="parser_tout_ms" id="parser_tout_ms" value="%parser_tout_ms%" required>
		<span class="mq-no-phone">&nbsp;ms</span>
	</div>

	<div class="Row">
		<label for="display_tout_ms"><?= tr('term.display_tout_ms') ?><span class="mq-phone">&nbsp;(ms)</span></label>
		<input type="number" step=1 min=0 name="display_tout_ms" id="display_tout_ms" value="%display_tout_ms%" required>
		<span class="mq-no-phone">&nbsp;ms</span>
	</div>

	<div class="Row">
		<label for="display_cooldown_ms"><?= tr('term.display_cooldown_ms') ?><span class="mq-phone">&nbsp;(ms)</span></label>
		<input type="number" step=1 min=0 name="display_cooldown_ms" id="display_cooldown_ms" value="%display_cooldown_ms%" required>
		<span class="mq-no-phone">&nbsp;ms</span>
	</div>

	<div class="Row">
		<label><?= tr("term.button_msgs") ?></label>
		<input class="short" type="text" name="bm1" id="bm1" value="%h:bm1%">&nbsp;
		<input class="short" type="text" name="bm2" id="bm2" value="%h:bm2%">&nbsp;
		<input class="short" type="text" name="bm3" id="bm3" value="%h:bm3%">&nbsp;
		<input class="short" type="text" name="bm4" id="bm4" value="%h:bm4%">&nbsp;
		<input class="short" type="text" name="bm5" id="bm5" value="%h:bm5%">
	</div>

	<div class="Row checkbox" >
		<label><?= tr('term.fn_alt_mode') ?></label><!--
		--><span class="box" tabindex=0 role=checkbox></span>
		<input type="hidden" id="fn_alt_mode" name="fn_alt_mode" value="%fn_alt_mode%">
	</div>

	<div class="Row checkbox" >
		<label><?= tr('term.show_buttons') ?></label><!--
		--><span class="box" tabindex=0 role=checkbox></span>
		<input type="hidden" id="show_buttons" name="show_buttons" value="%show_buttons%">
	</div>

	<div class="Row checkbox" >
		<label><?= tr('term.show_config_links') ?></label><!--
		--><span class="box" tabindex=0 role=checkbox></span>
		<input type="hidden" id="show_config_links" name="show_config_links" value="%show_config_links%">
	</div>

	<div class="Row checkbox" >
		<label><?= tr('term.loopback') ?></label><!--
		--><span class="box" tabindex=0 role=checkbox></span>
		<input type="hidden" id="loopback" name="loopback" value="%loopback%">
	</div>

	<div class="Row buttons">
		<a class="button icn-ok" href="#" onclick="qs('#form-2').submit()"><?= tr('apply') ?></a>
	</div>
</form>

<script>
	$('#default_fg').val(%default_fg%);
	$('#default_bg').val(%default_bg%);
	$('#theme').val(%theme%);

	function showColor() {
		var ex = qs('#color-example');
		ex.className = '';
		ex.classList.add('fg'+$('#default_fg').val());
		ex.classList.add('bg'+$('#default_bg').val());
		var th = $('#theme').val();
		$('.color-preview').forEach(function(e) {
			e.className = 'Row color-preview theme-'+th;
		});
	}
	showColor();

	$('.colorprev span').on('click', function() {
		var fg = this.dataset.fg;
		var bg = this.dataset.bg;
		if (typeof fg != 'undefined') $('#default_fg').val(fg);
		if (typeof bg != 'undefined') $('#default_bg').val(bg);
		showColor()
	});
</script>
