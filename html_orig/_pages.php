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

pg('cfg_wifi',        'cfg', '/cfg/wifi');
pg('cfg_wifi_conn',   '',    '/cfg/wifi/connecting');
pg('wifi_connstatus', 'api', '/cfg/wifi/connstatus');
pg('wifi_set',        'api', '/cfg/wifi/set');
pg('wifi_scan',       'api', '/cfg/wifi/scan');

pg('cfg_network',     'cfg', '/cfg/network');
pg('network_set',     'api', '/cfg/network/set');

pg('cfg_app',        'cfg', '/cfg/app');
pg('app_set',        'api', '/cfg/app/set');

pg('help',  'cfg page-help',  '/help');
pg('about', 'cfg page-about', '/about');
pg('term',  'term',           '/', 'title.term');

// ajax API

return $pages;
