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
$root = DEBUG ? json_encode(ESP_IP) : 'location.host';
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

if (!function_exists('utf8')) {
	function utf8($num)
	{
		if($num<=0x7F)       return chr($num);
		if($num<=0x7FF)      return chr(($num>>6)+192).chr(($num&63)+128);
		if($num<=0xFFFF)     return chr(($num>>12)+224).chr((($num>>6)&63)+128).chr(($num&63)+128);
		if($num<=0x1FFFFF)   return chr(($num>>18)+240).chr((($num>>12)&63)+128).chr((($num>>6)&63)+128).chr(($num&63)+128);
		return '';
	}
}

if (!function_exists('load_esp_charsets')) {
	function load_esp_charsets() {
		$chsf = __DIR__ . '/../user/character_sets.h';
		$re_table = '/\/\/ %%BEGIN:(.)%%\s*(.*?)\s*\/\/ %%END:\1%%/s';
		preg_match_all($re_table, file_get_contents($chsf), $m_tbl);

		$re_bounds = '/#define CODEPAGE_(.)_BEGIN\s+(\d+)\n#define CODEPAGE_\1_END\s+(\d+)/';
		preg_match_all($re_bounds, file_get_contents($chsf), $m_bounds);

		$cps = [];

		foreach ($m_tbl[2] as $i => $str) {
			$name = $m_tbl[1][$i];
			$start = intval($m_bounds[2][$i]);
			$table = [];
			$str = preg_replace('/,\s*\/\/[^\n]*/', '', $str);
			$rows = explode("\n", $str);
			$rows = array_map('trim', $rows);

			foreach($rows as $j => $v) {
				if (strpos($v, '0x') === 0) {
					$v = substr($v, 2);
					$v = hexdec($v);
				} else {
					$v = intval($v);
				}
				$ascii = $start+$j;
				$table[] = [
					$ascii,
					chr($ascii),
					utf8($v==0? $ascii :$v),
				];
			}
			$cps[$name] = $table;
		}
		return $cps;
	}
}

if (!function_exists('tplSubs')) {
	function tplSubs($str, $reps)
	{
		return preg_replace_callback('/%(j:|js:|h:|html:)?([a-z0-9-_.]+)%/i', function ($m) use ($reps) {
			$key = $m[2];
			if (array_key_exists($key, $reps)) {
				$val = $reps[$key];
			} else {
				$val = '';
			}
			switch ($m[1]) {
				case 'j:':
				case 'js:':
					$v = json_encode($val);
					return substr($v, 1, strlen($v) - 2);
				case 'h:':
				case 'html:':
					return htmlspecialchars($val);
				default:
					return $val;
			}
		}, $str);
	}
}
