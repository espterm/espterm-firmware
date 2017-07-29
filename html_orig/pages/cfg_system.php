<div class="Box str mobcol">
	<h2 tabindex=0><?= tr('system.save_restore') ?></h2>

	<div class="Row explain nomargintop">
		<?= tr('system.explain_persist') ?>
	</div>

	<div class="Row buttons2">
		<a class="button icn-restore"
		   onclick="return confirm('<?= tr('system.confirm_restore') ?>');"
		   href="<?= e(url('restore_defaults')) ?>">
			<?= tr('system.restore_defaults') ?>
		</a>
	</div>

	<div class="Row buttons2">
		<a onclick="writeDefaults(); return false;" href="#"><?= tr('system.write_defaults') ?></a>
	</div>

	<div class="Row buttons2">
		<a onclick="return confirm('<?= tr('system.confirm_restore_hard') ?>');"
		   href="<?= e(url('restore_hard')) ?>">
			<?= tr('system.restore_hard') ?>
		</a>
	</div>
</div>


<form class="Box str mobcol" action="<?= e(url('system_set')) ?>" method="GET" id="form-1">
	<h2 tabindex=0><?= tr('system.uart') ?></h2>

	<div class="Row explain">
		<?= tr('system.explain_uart') ?>
	</div>

	<div class="Row">
		<label for="uart_baud"><?= tr('uart.baud') ?><span class="mq-phone">&nbsp;(bps)</span></label>
		<select name="uart_baud" id="uart_baud" class="short">
			<?php foreach([
				300, 600, 1200, 2400, 4800, 9600, 19200, 38400,
				57600, 74880, 115200, 230400, 460800, 921600, 1843200, 3686400,
			] as $b):
				?><option value="<?=$b?>"><?= number_format($b, 0, ',', '.') ?></option>
			<?php endforeach; ?>
		</select>
		<span class="mq-no-phone">&nbsp;bps</span>
	</div>

	<div class="Row">
		<label for="uart_parity"><?= tr('uart.parity') ?></label>
		<select name="uart_parity" id="uart_parity" class="short">
			<?php foreach([
					2 => tr('uart.parity.none'),
				    1 => tr('uart.parity.odd'),
		            0 => tr('uart.parity.even'),
			] as $k => $label):
				?><option value="<?=$k?>"><?=$label?></option>
			<?php endforeach; ?>
		</select>
	</div>

	<div class="Row">
		<label for="uart_stopbits"><?= tr('uart.stop_bits') ?></label>
		<select name="uart_stopbits" id="uart_stopbits" class="short">
			<?php foreach([
				              1 => tr('uart.stop_bits.one'),
				              2 => tr('uart.stop_bits.one_and_half'),
				              3 => tr('uart.stop_bits.two'),
			              ] as $k => $label):
				?><option value="<?=$k?>"><?=$label?></option>
			<?php endforeach; ?>
		</select>
	</div>

	<div class="Row buttons">
		<a class="button icn-ok" href="#" onclick="qs('#form-1').submit()"><?= tr('apply') ?></a>
	</div>
</form>

<script>
	function writeDefaults() {
		var pw = prompt('<?= tr('system.confirm_store_defaults') ?>');
		if (!pw) return;
		location.href = <?=json_encode(url('write_defaults')) ?> + '?pw=' + pw;
	}

	$('#uart_baud').val(%uart_baud%);
	$('#uart_parity').val(%uart_parity%);
	$('#uart_stopbits').val(%uart_stopbits%);
</script>
