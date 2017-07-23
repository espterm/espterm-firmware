<?php
$ipmask='pattern="^([0-9]{1,3}\.){3}[0-9]{1,3}$"';
?>

<form class="Box str mobcol" action="<?= e(url('network_set')) ?>" method="GET">
	<h2><?= tr('net.ap') ?></h2>

	<div class="Row buttons">
		<input type="submit" value="<?= tr('apply') ?>">
	</div>

	<div class="Row explain">
		<?= tr('net.explain_ap') ?>
	</div>

	<div class="Row">
		<label for="ap_dhcp_time"><?= tr('net.ap_dhcp_time') ?><span class="mq-phone">&nbsp;(min)</span></label>
		<input type="number" step=1 min=1 max=2880 name="ap_dhcp_time" id="ap_dhcp_time" value="%ap_dhcp_time%" required>
		<span class="mq-no-phone">&nbsp;(min)</span>
	</div>

	<div class="Row">
		<label for="ap_dhcp_start"><?= tr('net.ap_dhcp_start') ?></label>
		<input type="text" name="ap_dhcp_start" id="ap_dhcp_start" value="%ap_dhcp_start%" <?=$ipmask?> required>
	</div>

	<div class="Row">
		<label for="ap_dhcp_end"><?= tr('net.ap_dhcp_end') ?></label>
		<input type="text" name="ap_dhcp_end" id="ap_dhcp_end" value="%ap_dhcp_end%" <?=$ipmask?> required>
	</div>

	<div class="Row">
		<label for="ap_addr_ip"><?= tr('net.ap_addr_ip') ?></label>
		<input type="text" name="ap_addr_ip" id="ap_addr_ip" value="%ap_addr_ip%" <?=$ipmask?> required>
	</div>

	<div class="Row">
		<label for="ap_addr_mask"><?= tr('net.ap_addr_mask') ?></label>
		<input type="text" name="ap_addr_mask" id="ap_addr_mask" value="%ap_addr_mask%" <?=$ipmask?> required>
	</div>
</form>

<form class="Box str mobcol" action="<?= e(url('network_set')) ?>" method="GET">
	<h2><?= tr('net.sta') ?></h2>

	<div class="Row buttons">
		<input type="submit" value="<?= tr('apply') ?>">
	</div>

	<div class="Row explain">
		<?= tr('net.explain_sta') ?>
	</div>

	<div class="Row checkbox">
		<label><?= tr('net.sta_dhcp_enable') ?></label><!--
		--><span class="box"></span>
		<input type="hidden" name="sta_dhcp_enable" value="%sta_dhcp_enable%">
	</div>

	<div class="Row">
		<label for="sta_addr_ip"><?= tr('net.sta_addr_ip') ?></label>
		<input type="text" name="sta_addr_ip" id="sta_addr_ip" value="%sta_addr_ip%" <?=$ipmask?> required>
	</div>

	<div class="Row">
		<label for="sta_addr_mask"><?= tr('net.sta_addr_mask') ?></label>
		<input type="text" name="sta_addr_mask" id="sta_addr_mask" value="%sta_addr_mask%" <?=$ipmask?> required>
	</div>

	<div class="Row">
		<label for="sta_addr_gw"><?= tr('net.sta_addr_gw') ?></label>
		<input type="text" name="sta_addr_gw" id="sta_addr_gw" value="%sta_addr_gw%" <?=$ipmask?> required>
	</div>
</form>

<div class="Box mobcol">
	<h2><?= tr('net.details') ?></h2>

	<div class="Row">
		<label><?= tr('net.sta_mac') ?></label><input type="text" readonly value="%sta_mac%">
	</div>
	<div class="Row">
		<label><?= tr('net.ap_mac') ?></label><input type="text" readonly value="%ap_mac%">
	</div>
</div>
