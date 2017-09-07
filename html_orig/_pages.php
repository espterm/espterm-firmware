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

pg('cfg_term',        'cfg', 'terminal', '/cfg/term');
pg('term_set',        'api', '', '/cfg/term/set');

pg('cfg_wifi',        'cfg', 'wifi', '/cfg/wifi');
pg('cfg_wifi_conn',   '',    '', '/cfg/wifi/connecting');
pg('wifi_connstatus', 'api', '', '/cfg/wifi/connstatus');
pg('wifi_set',        'api', '', '/cfg/wifi/set');
pg('wifi_scan',       'api', '', '/cfg/wifi/scan');

pg('cfg_network',     'cfg', 'network', '/cfg/network');
pg('network_set',     'api', '', '/cfg/network/set');

pg('cfg_system',        'cfg', 'configure', '/cfg/system');
pg('system_set',        'api', '', '/cfg/system/set');

pg('write_defaults',   'api', '', '/cfg/system/write_defaults');
pg('restore_defaults', 'api', '', '/cfg/system/restore_defaults');
pg('restore_hard',     'api', '', '/cfg/system/restore_hard');

pg('help',  'cfg page-help',  'help', '/help');
pg('about', 'cfg page-about', 'about', '/about');
pg('term',  'term',           '', '/', 'title.term');

pg('reset_screen',  'api',     '', '/system/cls', 'title.term');

pg('index',  'api',           '', '/', '');

// ajax API

return $pages;
