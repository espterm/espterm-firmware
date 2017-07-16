<?php

$pages = [];

/** Add a page */
function pg($key, $bc, $path) {
	global $pages;
	$pages[$key] = (object) [
		'bodyclass' => $bc,
		'path' => $path,
		'label' => tr("menu.$key"),
	];
}

pg('cfg_wifi', 'cfg', '/cfg/wifi');
pg('cfg_network', 'cfg', '/cfg/network');
pg('cfg_term', 'cfg', '/cfg/term');
pg('about', 'cfg', '/about');
pg('help', 'cfg', '/help');
pg('term', 'term', '/');

// technical
pg('wifi_set', '', '/wifi/set');//'/cfg/wifi/set');
pg('wifi_scan', '', '/wifi/scan');//'/cfg/wifi/scan');

return $pages;
