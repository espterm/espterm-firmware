<?php

return [
	'appname' => 'ESPTerm',

	'menu.cfg_wifi' => 'WiFi Settings',
	'menu.cfg_network' => 'Network Configuration',
	'menu.cfg_term' => 'Terminal Settings',
	'menu.about' => 'About ESPTerm',
	'menu.help' => 'Quick Reference',
	'menu.term' => 'Back to Terminal',
	'menu.cfg_wifi_conn' => 'Connecting to External Network',

	'title.term' => 'Terminal',

	'net.ap' => 'Access Point DHCP Config',
	'net.sta' => 'Client IP Config',

	'net.explain_sta' => '
		Those settings affect the built-in DHCP client. Switching it off
		makes ESPTerm use the configured static IP. Please double-check
		those settings before submitting, setting them incorrectly may
		make it hard to access ESPTerm via the external network.',

	'net.explain_ap' => '
		Those settings affect the built-in DHCP server in AP mode.
		Please double-check those settings before submitting, setting them
		incorrectly may render ESPTerm inaccessible via the AP.',

	'net.ap_dhcp_time' => 'Lease time',
	'net.ap_dhcp_start' => 'Pool start IP',
	'net.ap_dhcp_end' => 'Pool end IP',
	'net.ap_addr_ip' => 'Own IP address',
	'net.ap_addr_mask' => 'Subnet mask',

	'net.sta_dhcp_enable' => 'Enable DHCP',
	'net.sta_addr_ip' => 'ESPTerm static IP',
	'net.sta_addr_mask' => 'Subnet mask',
	'net.sta_addr_gw' => 'Gateway IP',

	'wifi.ap' => 'Built-in Access Point',
	'wifi.sta' => 'Connect to External Network',

	'wifi.enable' => 'Enabled:',
	'wifi.tpw' => 'Transmit Power:',
	'wifi.ap_channel' => 'Channel:',
	'wifi.ap_ssid' => 'AP SSID:',
	'wifi.ap_password' => 'Password:',
	'wifi.ap_hidden' => 'Hide SSID:',
	'wifi.sta_info' => 'Selected:',

	'wifi.sta_ssid' => 'Network SSID:',
	'wifi.sta_password' => 'Password:',
	'wifi.not_conn' => 'Not connected.',
	'wifi.sta_none' => 'None',
	'wifi.sta_active_pw' => '🔒',
	'wifi.sta_active_nopw' => '🔓 Open access',
	'wifi.connected_ip_is' => 'Connected, IP is ',

	'wifi.scanning' => 'Scanning',
	'wifi.scan_now' => 'Start scanning!',
	'wifi.cant_scan_no_sta' => 'Can\'t scan with Client mode disabled.',
	'wifi.select_ssid' => 'Available networks:',

	'wifi.conn.status' => 'Status:',
	'wifi.conn.back_to_config' => 'Back to WiFi config',
	'wifi.conn.telemetry_lost' => 'Telemetry lost, something went wrong. Try again...',

	'wifi.conn.idle' =>"Preparing to connect",
	'wifi.conn.success' => "Connected! Received IP ",
	'wifi.conn.working' => "Connecting to selected AP",
	'wifi.conn.fail' => "Connection failed, check your password and try again.",

	'apply' => 'Apply!',
	'enabled' => 'Enabled',
	'disabled' => 'Disabled',
	'yes' => 'Yes',
	'no' => 'No',
	'confirm' => 'OK',
];
