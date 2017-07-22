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
pg('cfg_wifi_conn', '', '/wifi/connecting'); // page without menu that tries to show the connection progress
pg('cfg_network', 'cfg', '/cfg/network');
pg('cfg_term', 'cfg', '/cfg/term');
pg('about', 'cfg page-about', '/about');
pg('help', 'cfg page-help', '/help');
pg('term', 'term', '/');

// ajax API
pg('wifi_set', '', '/wifi/set');//'/cfg/wifi/set');
pg('wifi_scan', '', '/wifi/scan');//'/cfg/wifi/scan');
pg('wifi_connstatus', '', '/wifi/connstatus');

return $pages;
