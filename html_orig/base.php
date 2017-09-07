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

define('ESP_DEMO', (bool)getenv('ESP_DEMO'));
if (ESP_DEMO) {
	define('DEMO_SCREEN', '"S\u0019\u0001Q\u0001\u0017\u0001K\u0001\u0015\u0004\u0003\b\u0001 \u0002P\u0001┌ESPTerm─Demo─\u0002\u0002\u0001\u0003\u0002\u000131\u0003\u0003\u000132\u0003\u0004\u00013\u0002\u0002\u0001\u0003\u0005\u000134\u0003\u0006\u000135\u0003\u0007\u000136\u0003\b\u000137\u0003\t\u000190\u0003\n\u000191\u0003\u000b\u000192\u0003\f\u000193\u0003\r\u000194\u0003\u000e\u000195\u0003\u000f\u000196\u0003\u0010\u000197\u0003\b\u0001─\u0002\r\u0001┐ \u0002\u0015\u0001│ \u00029\u0001│ \u0002\u0004\u0001│\u0002\t\u0001 \u0002\b\u0001│\u0004\u0002\u0001Bold \u0004\u0003\u0001F\u0004\u0003\u0001a\u0004\u0003\u0001i\u0004\u0003\u0001n\u0004\u0003\u0001t\u0004\u0003\u0001 \u0004\u0005\u0001I\u0004\u0005\u0001t\u0004\u0005\u0001a\u0004\u0005\u0001l\u0004\u0005\u0001i\u0004\u0005\u0001c\u0004\u0005\u0001 \u0004\t\u0001U\u0004\t\u0001n\u0004\t\u0001d\u0004\t\u0001e\u0004\t\u0001r\u0004\t\u0001l\u0004\t\u0001i\u0004\t\u0001n\u0004\t\u0001e\u0004\u0001\u0001 \u0004\u0011\u0001B\u0004\u0011\u0001l\u0004\u0011\u0001i\u0004\u0011\u0001n\u0004\u0011\u0001k\u0004\u0011\u0001 \u0001q\u0001\u0001Inverse\u0003\b\u0001 \u0004A\u0001S\u0004A\u0001t\u0004A\u0001r\u0004A\u0001i\u0004A\u0001k\u0004A\u0001e\u0004\u0001\u0001 \u0004!\u0001F\u0004!\u0001r\u0004!\u0001a\u0004!\u0001k\u0004!\u0001t\u0004!\u0001u\u0004!\u0001r\u0004\u0001\u0001 │ \u0002\u0002\u0001─\u0002\u0002\u0001\u0003\n\u0002 \u0002\t\u0001\u0003\b\u0001─\u0002\u0002\u0001 \u0002\u0006\u0001│ \u00029\u0001│ \u0002\u0002\u0001─\u0002\u0002\u0001\u0003\n\u0002 \u0003\u0002\u0002ESP826\u0002\u0002\u0001\u0003\n\u0002 \u0003\b\u0001─\u0002\u0002\u0001 \u0002\u0006\u0001└─\u00029\u0001┤ \u0002\u0002\u0001─\u0002\u0002\u0001\u0003\n\u0002 \u0002\t\u0001\u0003\b\u0001─\u0002\u0002\u0001 \u0002@\u0001│ \u0002\u0002\u0001─\u0002\u0002\u0001\u0003\n\u0002 \u0003\u0002\u0002(@)#\u0002\u0004\u0001\u0003\n\u0002 \u0003\b\u0001─\u0002\u0002\u0001 \u0002\u0007\u0001\u0003O\u0001 This is a static demo of the ESPTerm Web Interface \u0002\u0004\u0001\u0003\b\u0001 \u0002\u0002\u0001│ \u0002\u0002\u0001─\u0002\u0002\u0001\u0003\n\u0002 \u0002\t\u0001\u0003\b\u0001─\u0002\u0002\u0001 \u0002\u0007\u0001\u0003O\u0001 \u00027\u0001\u0003\b\u0001 \u0002\u0002\u0001│ \u0002\u0004\u0001│\u0002\t\u0001 \u0002\t\u0001\u0003O\u0001 Try the links beneath this scre\u0002\u0002\u0001n to browse the menu. \u0003\b\u0001 \u0002\u0002\u0001♦ \u0002\u0016\u0001\u0003O\u0001 \u00027\u0001\u0003\b\u0001 \u0002\u0019\u0001\u0003O\u0001 <°)\u0002\u0003\u0001>< ESPTerm ful\u0002\u0002\u0001y sup\u0002\u0002\u0001orts UTF-8 お は よ ー \u0002\u0002\u0001><(\u0002\u0003\u0001°> \u0003\b\u0001 \u0002\u0019\u0001\u0003O\u0001 \u00027\u0001\u0003\b\u0001 \u0002i\u0001\u0003\u000b\u0001Other interesting features:\u0003\b\u0001 \u0002\u0018\u0001↓ \u0002n\u0001\u0003\u0003\u0001- Almost ful\u0002\u0002\u0001 VT10\u0002\u0002\u0001 emulation \u0003\b\u0001 \u0003\u0006\u0001()\u0003\b\u0001 \u0003\u0006\u0001()\u0003\b\u0001 \u0002\b\u0001Funguje tu čeština! \u0002\u0011\u0001\u0003\u0005\u0001- Xterm-like mouse tracking\u0003\b\u0001 \u0002\u0003\u0001=\u0002\u0002\u0001\u0003\t\u0002°.°\u0003\b\u0001=\u0002\u0002\u0001 \u0003\u0006\u0001<-\u0002\u0003\u0001, \u0003\b\u0001 \u0002$\u0001\u0003\u0004\u0001- File upload utility\u0003\b\u0001 \u0002\n\u0001\'\u0002\u0002\u0001 \'\u0002\u0002\u0001 \u0002\u0002\u0001\u0003\u0006\u0001 \u0002\u0004\u0001mouse\u0003\b\u0001 \u0002!\u0001\u0003\u0002\u0001- User-friendly config interface\u0003\b\u0001 \u00020\u0001\u0003\u000e\u0001-\u0003\u0002\u0001 \u0003\u000e\u0001Advanced WiFi & network set\u0002\u0002\u0001ings\u0003\b\u0001 \u0002\u0011\u0001\u0003\f\u0001Try ESPTerm today!\u0003\b\u0001 \u0002\u000b\u0001- Built-in help page \u0002\u001a\u0001\u0003\u0007\u0001-\u0002\u0002\u0001>\u0003\b\u0001 \u0002\u0002\u0001\u0003\f\u0001Pre-built binaries\u0003\b\u0001 \u0003\f\u0001are\u0003\b\u0001 \u0002\"\u0001\u0003\u0007\u0001link on the About page \u0002\u0002\u0001\u0003\f\u0001available on GitHub! \u0003\b\u0001 \u0002U\u0001"');
	define('DEMO_APS', <<<APS
{
 "result": {
  "inProgress": 0,
  "APs": [
   {"essid": "Cisco", "bssid": "88:f7:c7:52:b3:99", "rssi": 205, "rssi_perc": 100, "enc": 4, "channel": 7},
   {"essid": "UPC Wi-Free", "bssid": "8a:f7:c7:52:b3:9b", "rssi": 203, "rssi_perc": 100, "enc": 5, "channel": 1},
   {"essid": "UPC Wi-Free", "bssid": "0a:95:2a:0c:84:31", "rssi": 166, "rssi_perc": 32, "enc": 5, "channel": 1},
   {"essid": "MujO2Internet_2EEB96", "bssid": "d0:60:8c:2e:eb:96", "rssi": 174, "rssi_perc": 48, "enc": 4, "channel": 4},
   {"essid": "Internet", "bssid": "38:72:c0:32:bd:0d", "rssi": 164, "rssi_perc": 28, "enc": 2, "channel": 10},
   {"essid": "MyO2Internet_08C850", "bssid": "78:c1:a7:08:c8:50", "rssi": 186, "rssi_perc": 72, "enc": 4, "channel": 11},
   {"essid": "UPC Wi-Free", "bssid": "06:7c:34:9a:6f:7c", "rssi": 167, "rssi_perc": 34, "enc": 0, "channel": 11},
   {"essid": "Internet_B0", "bssid": "5c:f4:ab:11:3b:b3", "rssi": 175, "rssi_perc": 50, "enc": 3, "channel": 13},
   {"essid": "UPC5716805", "bssid": "08:95:2a:0c:84:3f", "rssi": 165, "rssi_perc": 30, "enc": 4, "channel": 1}
    ]
 }
}
APS
);
}

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
	if (ESP_DEMO) return "$name.html";
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

function je($s)
{
	return htmlspecialchars(json_encode($s), ENT_HTML5);
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
