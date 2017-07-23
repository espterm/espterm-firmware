<div class="Box">
	<div class="Row explain">
		<?= tr('admin.explain') ?>
	</div>

	<div class="Row buttons">
		<a class="button icn-restore"
		   onclick="return confirm('<?= tr('admin.confirm_restore') ?>');"
		   href="<?= e(url('restore_defaults')) ?>"
		>
			<?= tr('admin.restore_defaults') ?>
		</a>
	</div>

	<div class="Row buttons">
		<a onclick="writeDefaults(); return false;" href="#"><?= tr('admin.write_defaults') ?></a>
	</div>

	<div class="Row buttons">
		<a onclick="return confirm('<?= tr('admin.confirm_restore_hard') ?>');"
		   href="<?= e(url('restore_hard')) ?>"
		>
			<?= tr('admin.restore_hard') ?>
		</a>
	</div>
</div>

<script>
	function writeDefaults() {
		var pw = prompt('<?= tr('admin.confirm_store_defaults') ?>');
		if (!pw) return;
		location.href = <?=json_encode(url('write_defaults')) ?> + '?pw=' + pw;
	}
</script>
