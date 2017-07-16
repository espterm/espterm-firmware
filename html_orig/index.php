<?php

require '_env.php';
$prod = defined('STDIN');
define ('DEBUG', !$prod);
$root = DEBUG ? ESP_IP : '';
define ('LIVE_ROOT', $root);

define('CUR_PAGE', $_GET['page'] ?: 'term');
define('LOCALE', $_GET['locale'] ?: 'en');

$_messages = require(__DIR__ . '/messages/' . LOCALE . '.php');
$_pages = require('_pages.php');

define('APP_NAME', 'ESPTerm');
define('PAGE_TITLE', $_pages[CUR_PAGE]->label . ' :: ' . APP_NAME);
define('BODYCLASS', $_pages[CUR_PAGE]->bodyclass);

/** URL (dev or production) */
function url($name, $root=false) {
	global $_pages;
	if ($root) return LIVE_ROOT . $_pages[$name]->path;

	if (DEBUG) return "/index.php?page=$name";
	else return $_pages[$name]->path;
}

/** URL label for a button */
function label($name) {
	global $_pages;
	return $_pages[$name]->label;
}

function e($s) {
	return htmlspecialchars($s, ENT_HTML5|ENT_QUOTES);
}

function tr($key) {
	global $_messages;
	return $_messages[$key] ?: ('??'.$key.'??');
}

/** Like eval, but allows <?php and ?> */
function include_str($code) {
	$tmp = tmpfile();
	$tmpf = stream_get_meta_data($tmp);
	$tmpf = $tmpf ['uri'];
	fwrite($tmp, $code);
	$ret = include($tmpf);
	fclose($tmp);
	return $ret;
}

require 'pages/_head.php';
$_pf = 'pages/'.CUR_PAGE.'.php';
if (file_exists($_pf)) {
	$f = file_get_contents($_pf);
	$reps = require('_debug_replacements.php');
	$str = str_replace(array_keys($reps), array_values($reps), $f);
	include_str($str);
} else {
	echo "404";
}
require 'pages/_tail.php';
