<form class="Box mobopen str" action="<?= e(url('term_set')) ?>" method="GET" id='form-1'>
	<h2><?= tr('term.defaults') ?></h2>

	<div class="Row explain">
		<?= tr('term.explain_initials') ?>
	</div>

	<div class="Row">
		<div style="font-family:monospace; font-size: 16pt; padding: 5px;" id="color-example"><?= tr("term.example") ?></div>
	</div>

	<div class="Row">
		<label for="default_fg"><?= tr("term.default_fg") ?></label>
		<select name="default_fg" id="default_fg" class="short" onchange="showColor()">
			<?php for($i=0; $i<16; $i++): ?>
			<option value="<?=$i?>"><?= tr("color.$i") ?></option>
			<?php endfor; ?>
		</select>
	</div>

	<div class="Row">
		<label for="default_bg"><?= tr("term.default_bg") ?></label>
		<select name="default_bg" id="default_bg" class="short" onchange="showColor()">
			<?php for($i=0; $i<16; $i++): ?>
				<option value="<?=$i?>"><?= tr("color.$i") ?></option>
			<?php endfor; ?>
		</select>
	</div>

	<div class="Row">
		<label for="term_width"><?= tr('term.term_width') ?></label>
		<input type="number" step=1 min=1 max=255 name="term_width" id="term_width" value="%term_width%" required>
	</div>

	<div class="Row">
		<label for="term_height"><?= tr('term.term_height') ?></label>
		<input type="number" step=1 min=1 max=255 name="term_height" id="term_height" value="%term_height%" required>
	</div>

	<div class="Row">
		<label for="term_title"><?= tr('term.term_title') ?></label>
		<input type="text" name="term_title" id="term_title" value="%term_title%" required>
	</div>

	<div class="Row">
		<label for="btn1"><?= tr("term.btn1") ?></label>
		<input class="short" type="text" name="btn1" id="btn1" value="%btn1%">
	</div>

	<div class="Row">
		<label for="btn2"><?= tr("term.btn2") ?></label>
		<input class="short" type="text" name="btn2" id="btn2" value="%btn2%">
	</div>

	<div class="Row">
		<label for="btn3"><?= tr("term.btn3") ?></label>
		<input class="short" type="text" name="btn3" id="btn3" value="%btn3%">
	</div>

	<div class="Row">
		<label for="btn4"><?= tr("term.btn4") ?></label>
		<input class="short" type="text" name="btn4" id="btn4" value="%btn4%">
	</div>

	<div class="Row">
		<label for="btn5"><?= tr("term.btn5") ?></label>
		<input class="short" type="text" name="btn5" id="btn5" value="%btn5%">
	</div>

	<div class="Row buttons">
		<a class="button icn-ok" href="#" onclick="qs('#form-1').submit()"><?= tr('apply') ?></a>
	</div>
</form>

<script>
	$('#default_fg').val(%default_fg%);
	$('#default_bg').val(%default_bg%);

	function showColor() {
		var ex = qs('#color-example');
		ex.className = '';
		ex.classList.add('fg'+$('#default_fg').val());
		ex.classList.add('bg'+$('#default_bg').val());
	}
	showColor();
</script>
