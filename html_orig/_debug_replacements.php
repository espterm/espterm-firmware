<?php

/**
 * Those replacements are done by the development server to test it locally
 * without esphttpd. This is needed mainly for places where the replacements
 * are given to JavaScript, to avoid syntax errors with
 */

$vers = '???';
$f = file_get_contents(__DIR__ . '/../user/version.h');
preg_match_all('/#define FW_V_.*? (\d+)/', $f, $vm);
#define FW_V_MAJOR 1
#define FW_V_MINOR 0
#define FW_V_PATCH 0

$vers = $vm[1][0].'.'.$vm[1][1].'.'.$vm[1][2];

return [
	'term_title' => ESP_DEMO ? 'ESPTerm Web UI Demo' : 'ESPTerm local debug',

	'btn1' => 'OK',
	'btn2' => 'Cancel',
	'btn3' => '',
	'btn4' => '',
	'btn5' => 'Help',
	'bm1' => '01,'.ord('y'),
	'bm2' => '01,'.ord('n'),
	'bm3' => '',
	'bm4' => '',
	'bm5' => '05',
	'labels_seq' => ESP_DEMO ? 'TESPTerm Web UI DemoOKCancelHelp' : 'TESPTerm local debugOKCancelHelp',

	'parser_tout_ms' => 10,
	'display_tout_ms' => 15,
	'display_cooldown_ms' => 35,
	'fn_alt_mode' => '1',

	'opmode' => '2',
	'sta_enable' => '1',
	'ap_enable' => '1',

	'tpw' => '60',
	'ap_channel' => '7',
	'ap_ssid' => 'TERM-027451',
	'ap_password' => '',
	'ap_hidden' => '0',

	'sta_ssid' => 'Cisco',
	'sta_password' => 'Passw0rd!',
	'sta_active_ip' => ESP_DEMO ? '192.168.82.66' : '192.168.0.19',
	'sta_active_ssid' => 'Cisco',

	'vers_fw' => $vers,
	'date' => date('Y-m-d'),
	'time' => date('G:i'),
	'vers_httpd' => '0.4',
	'vers_sdk' => '010502',
	'githubrepo' => 'https://github.com/MightyPork/esp-vt100-firmware',

	'ap_dhcp_time' => '120',
	'ap_dhcp_start' => '192.168.4.100',
	'ap_dhcp_end' => '192.168.4.200',
	'ap_addr_ip' => '192.168.4.1',
	'ap_addr_mask' => '255.255.255.0',

	'sta_dhcp_enable' => '1',
	'sta_addr_ip' => '192.168.0.33',
	'sta_addr_mask' => '255.255.255.0',
	'sta_addr_gw' => '192.168.0.1',

	'sta_mac' => '5c:cf:7f:02:74:51',
	'ap_mac' => '5e:cf:7f:02:74:51',

	'term_width' => '80',
	'term_height' => '25',
	'default_bg' => '0',
	'default_fg' => '7',
	'show_buttons' => '1',
	'show_config_links' => '1',

	'uart_baud' => 115200,
	'uart_stopbits' => 1,
	'uart_parity' => 2,

	'theme' => 0,
];
