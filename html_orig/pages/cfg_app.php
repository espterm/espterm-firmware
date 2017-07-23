<form class="Box mobopen str" action="<?= e(url('app_set')) ?>" method="GET" id='form-1'>
	<h2><?= tr('app.defaults') ?></h2>

	<div class="Row explain">
		<?= tr('app.explain_initials') ?>
	</div>

	<div class="Row">
		<label for="term_width"><?= tr('app.term_width') ?></label>
		<input type="number" step=1 min=1 max=255 name="term_width" id="term_width" value="%term_width%" required>
	</div>

	<div class="Row">
		<label for="term_height"><?= tr('app.term_height') ?></label>
		<input type="number" step=1 min=1 max=255 name="term_height" id="term_height" value="%term_height%" required>
	</div>

	<div class="Row">
		<label for="default_fg"><?= tr("app.default_fg") ?></label>
		<select name="default_fg" id="default_fg">
			<?php for($i=0; $i<16; $i++): ?>
			<option value="<?=$i?>"><?= tr("color.$i") ?></option>
			<?php endfor; ?>
		</select>
		<script>qs('#default_fg').selectedIndex = %default_fg%;</script>
	</div>

	<div class="Row">
		<label for="default_bg"><?= tr("app.default_bg") ?></label>
		<select name="default_bg" id="default_bg">
			<?php for($i=0; $i<16; $i++): ?>
				<option value="<?=$i?>"><?= tr("color.$i") ?></option>
			<?php endfor; ?>
		</select>
		<script>qs('#default_bg').selectedIndex = %default_bg%;</script>
	</div>

	<div class="Row">
		<label for="term_title"><?= tr('app.term_title') ?></label>
		<input type="text" name="term_title" id="term_title" value="%term_title%" required>
	</div>

	<div class="Row">
		<label for="btn1"><?= tr("app.btn1") ?></label>
		<input class="short" type="text" name="btn1" id="btn1" value="%btn1%">
	</div>

	<div class="Row">
		<label for="btn2"><?= tr("app.btn2") ?></label>
		<input class="short" type="text" name="btn2" id="btn2" value="%btn2%">
	</div>

	<div class="Row">
		<label for="btn3"><?= tr("app.btn3") ?></label>
		<input class="short" type="text" name="btn3" id="btn3" value="%btn3%">
	</div>

	<div class="Row">
		<label for="btn4"><?= tr("app.btn4") ?></label>
		<input class="short" type="text" name="btn4" id="btn4" value="%btn4%">
	</div>

	<div class="Row">
		<label for="btn5"><?= tr("app.btn5") ?></label>
		<input class="short" type="text" name="btn5" id="btn5" value="%btn5%">
	</div>

	<div class="Row buttons">
		<a class="button icn-ok" href="#" onclick="qs('#form-1').submit()"><?= tr('apply') ?></a>
	</div>
</form>
