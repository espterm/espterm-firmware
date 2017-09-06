<?php

/* This script is run on demand to generate JS version of tr() */

require_once __DIR__ . '/base.php';

$selected = [
	'wifi.connected_ip_is',
	'wifi.not_conn',
	'wifi.enter_passwd',
	'wifi.passwd_saved',
];

$out = [];
foreach ($selected as $key) {
	$out[$key] = $_messages[$key];
}

file_put_contents(__DIR__. '/jssrc/lang.js',
	"// Generated from PHP locale file\n" .
	'var _tr = ' . json_encode($out, JSON_PRETTY_PRINT|JSON_UNESCAPED_UNICODE) . ";\n\n" .
	"function tr(key) { return _tr[key] || '?'+key+'?'; }\n"
);
