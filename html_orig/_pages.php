<?php

$pages = [];

if (! function_exists('pg')) {
	/** Add a page */
	function pg($key, $bc, $icon, $path, $titleKey = null)
	{
		global $pages;
		$pages[$key] = (object) [
			'key' => $key,
			'bodyclass' => $bc,
			'path' => $path,
			'icon' => $icon ? "icn-$icon" : '',
			'label' => tr("menu.$key"),
			'title' => $titleKey ? tr($titleKey) : tr("menu.$key"),
		];
	}
}

pg('cfg_wifi',        'cfg', 'wifi', '/cfg/wifi');
pg('cfg_wifi_conn',   '',    '', '/cfg/wifi/connecting');
pg('wifi_connstatus', 'api', '', '/cfg/wifi/connstatus');
pg('wifi_set',        'api', '', '/cfg/wifi/set');
pg('wifi_scan',       'api', '', '/cfg/wifi/scan');

pg('cfg_network',     'cfg', 'network', '/cfg/network');
pg('network_set',     'api', '', '/cfg/network/set');

pg('cfg_app',         'cfg', 'terminal', '/cfg/app');
pg('app_set',         'api', '', '/cfg/app/set');

pg('cfg_admin',        'cfg', 'persist', '/cfg/admin');
pg('write_defaults',   'api', '', '/cfg/admin/write_defaults');
pg('restore_defaults', 'api', '', '/cfg/admin/restore_defaults');
pg('restore_hard',     'api', '', '/cfg/admin/restore_hard');

pg('help',  'cfg page-help',  'help', '/help');
pg('about', 'cfg page-about', 'about', '/about');
pg('term',  'term',           '', '/', 'title.term');

// ajax API

return $pages;
