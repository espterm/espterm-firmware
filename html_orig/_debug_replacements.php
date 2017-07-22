<?php

/**
 * Those replacements are done by the development server to test it locally
 * without esphttpd. This is needed mainly for places where the replacements
 * are given to JavaScript, to avoid syntax errors with %%
 */
return [
	'%term_title%' => 'ESP8266 Wireless Terminal',

	'%b1%' => '1',
	'%b2%' => '2',
	'%b3%' => '3',
	'%b4%' => '4',
	'%b5%' => '5',
	'%screenData%' => '{
		 "w": 26, "h": 10,
		 "x": 0, "y": 0,
		 "cv": 1,
		 "screen": "70 t259"
	}',

	'%ap_enable%' => '1',
	'%tpw%' => '60',
	'%ap_channel%' => '7',
	'%ap_ssid%' => 'ESP-123456',
	'%ap_password%' => 'Passw0rd!',
	'%ap_hidden%' => '0',
	'%sta_ssid%' => 'Chlivek',
	'%sta_password%' => 'windows XP is The Best',
	'%sta_active_ip%' => '1.2.3.4',
	'%sta_active_ssid%' => 'Chlivek',

	'%sta_enable%' => '1',
	'%opmode%' => '3',
	'%vers_fw%' => '1.2.3',
	'%date%' => date('Y-m-d'),
	'%time%' => date('G:i'),
	'%vers_httpd%' => '4.5.6',
	'%vers_sdk%' => '1.52',
	'%githubrepo%' => 'https://github.com/MightyPork/esp-vt100-firmware',

	'%ap_dhcp_time%' => '120',
	'%ap_dhcp_start%' => '192.168.4.100',
	'%ap_dhcp_end%' => '192.168.4.200',
	'%ap_addr_ip%' => '192.168.4.1',
	'%ap_addr_mask%' => '255.255.255.0',

	'%sta_dhcp_enable%' => '1',
	'%sta_addr_ip%' => '192.168.0.33',
	'%sta_addr_mask%' => '255.255.255.0',
	'%sta_addr_gw%' => '192.168.0.1',
];
