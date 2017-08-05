<?php

/**
 * Those replacements are done by the development server to test it locally
 * without esphttpd. This is needed mainly for places where the replacements
 * are given to JavaScript, to avoid syntax errors with %%
 */
return [
	'%term_title%' => 'ESPTerm local debug',

	'%btn1%' => '1',
	'%btn2%' => '2',
	'%btn3%' => '3',
	'%btn4%' => '',
	'%btn5%' => '5',
	'%labels_seq%' => 'TESPTerm local debug1235',

	'%screenData%' => 'HELLOxNRE3',//'\u000b\u0001\u001b\u0001\u0001\u0001\u0001\u0001\f\u0005\u0001\u0010\u0003HELLOx\u0002\u000b\u0001\u0001N\u0001RE\u00023\u0001', //,
	/*'{
		 "w": 26, "h": 10,
		 "x": 10, "y": 5,
		 "cv": 1, "fg": 10, "bg": 2,
		 "screen": ""
	}',//"70 t259"*/

	'%opmode%' => '2',
	'%sta_enable%' => '0',
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

	'%sta_mac%' => 'ab:cd:ef:01:23:45',
	'%ap_mac%' => '01:23:45:ab:cd:ef',

	'%term_width%' => '26',
	'%term_height%' => '10',
	'%default_bg%' => '0',
	'%default_fg%' => '7',

	'%uart_baud%' => 115200,
	'%uart_stopbits%' => 1,
	'%uart_parity%' => 2,

	'%theme%' => 5,
];
