<?php

$pages = [];

if (! function_exists('pg')) {
	/** Add a page */
	function pg($key, $bc, $path, $titleKey = null)
	{
		global $pages;
		$pages[$key] = (object) [
			'key' => $key,
			'bodyclass' => $bc,
			'path' => $path,
			'label' => tr("menu.$key"),
			'title' => $titleKey ? tr($titleKey) : tr("menu.$key"),
		];
	}
}

pg('cfg_wifi', 'cfg', '/cfg/wifi');
pg('cfg_wifi_conn', '', '/wifi/connecting'); // page without menu that tries to show the connection progress
pg('cfg_network', 'cfg', '/cfg/network');
//pg('cfg_term', 'cfg', '/cfg/term');
pg('help', 'cfg page-help', '/help');
pg('about', 'cfg page-about', '/about');
pg('term', 'term', '/', 'title.term');

// ajax API
pg('wifi_set', 'api', '/wifi/set');//'/cfg/wifi/set');
pg('wifi_scan', 'api', '/wifi/scan');//'/cfg/wifi/scan');
pg('wifi_connstatus', 'api', '/wifi/connstatus');

return $pages;
