
<nav id="menu">
	<div id="brand" onclick="$('#menu').toggleClass('expanded')"><?= tr('appname') ?></div>
	<?php
	// generate the menu
	foreach($_pages as $k => $page) {
		if ($page->bodyclass !== 'cfg') continue;
		$sel = (CUR_PAGE == $k) ? ' class="selected"' : '';
		$text = $page->label;
		$url = e(url($k));
		echo "<a href=\"$url\"$sel>$text</a>";
	}
	?><a href="<?= e(url('term')) ?>"><?= tr('menu.term') ?></a>

</nav>
