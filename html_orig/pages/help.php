<div class="Box">
	<a href="#" onclick="hpfold(1);return false">Expand all</a>&nbsp;|&nbsp;<a href="#" onclick="hpfold(0);return false">Collapse all</a>
</div>

<?php require __DIR__ . "/help/troubleshooting.php"; ?>
<?php require __DIR__ . "/help/nomenclature.php"; ?>
<?php require __DIR__ . "/help/screen_behavior.php"; ?>
<?php require __DIR__ . "/help/input.php"; ?>
<?php require __DIR__ . "/help/charsets.php"; ?>
<?php require __DIR__ . "/help/sgr_styles.php"; ?>
<?php require __DIR__ . "/help/sgr_colors.php"; ?>
<?php require __DIR__ . "/help/cmd_cursor.php"; ?>
<?php require __DIR__ . "/help/cmd_screen.php"; ?>
<?php require __DIR__ . "/help/cmd_system.php"; ?>

<script>
	function hpfold(yes) {
		$('.fold').toggleClass('expanded', !!yes);
	}
</script>
