<?php

/**
 * The common stuff required by both index.php and build_html.php
 * this must be required_once
 */

if (defined('BASE_INITED')) return;
define('BASE_INITED', true);

if (!empty($argv[1])) {
	parse_str($argv[1], $_GET);
}

if (!file_exists(__DIR__ . '/_env.php')) {
	die("Copy <b>_env.php.example</b> to <b>_env.php</b> and check the settings inside!");
}

require_once __DIR__ . '/_env.php';

$prod = defined('STDIN');
define('DEBUG', !$prod);
$root = DEBUG ? json_encode(ESP_IP) : 'window.location.href';
define('JS_WEB_ROOT', $root);

define('LOCALE', isset($_GET['locale']) ? $_GET['locale'] : 'en');

$_messages = require(__DIR__ . '/lang/' . LOCALE . '.php');
$_pages = require(__DIR__ . '/_pages.php');

define('APP_NAME', 'ESPTerm');

/** URL (dev or production) */
function url($name, $relative = false)
{
	global $_pages;
	if ($relative) return $_pages[$name]->path;

	if (DEBUG) return "/index.php?page=$name";
	else return $_pages[$name]->path;
}

/** URL label for a button */
function label($name)
{
	global $_pages;
	return $_pages[$name]->label;
}

function e($s)
{
	return htmlspecialchars($s, ENT_HTML5 | ENT_QUOTES);
}

function tr($key)
{
	global $_messages;
	return isset($_messages[$key]) ? $_messages[$key] : ('??' . $key . '??');
}

/** Like eval, but allows <?php and ?> */
function include_str($code)
{
	$tmp = tmpfile();
	$tmpf = stream_get_meta_data($tmp);
	$tmpf = $tmpf ['uri'];
	fwrite($tmp, $code);
	$ret = include($tmpf);
	fclose($tmp);
	return $ret;
}
