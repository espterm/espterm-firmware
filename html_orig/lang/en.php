<?php

return [
	'appname' => 'ESPTerm',

	'menu.cfg_wifi' => 'WiFi Settings',
	'menu.cfg_network' => 'Network Configuration',
	'menu.cfg_app' => 'Terminal Settings',
	'menu.about' => 'About ESPTerm',
	'menu.help' => 'Quick Reference',
	'menu.term' => 'Back to Terminal',
	'menu.cfg_admin' => 'Reset & Restore',
	'menu.cfg_wifi_conn' => 'Connecting to External Network',

	'title.term' => 'Terminal',

	'net.ap' => 'DHCP Server (AP)',
	'net.sta' => 'DHCP Client (Station)',
	'net.sta_mac' => 'Station MAC',
	'net.ap_mac' => 'AP MAC',
	'net.details' => 'MAC addresses',

	'app.defaults' => 'Initial settings',
	'app.explain_initials' => '
		Those are the initial settings used after ESPTerm restarts, and they
		will also be applied immediately after you submit this form.
		They can be subsequently changed by ESC commands, but those changes
		aren\'t persistent and will be lost when the device powers off.',

	'app.term_title' => 'Header text',
	'app.term_width' => 'Screen width',
	'app.term_height' => 'Screen height',
	'app.default_fg' => 'Base text color',
	'app.default_bg' => 'Base background',
	'app.btn1' => 'Button 1 text',
	'app.btn2' => 'Button 2 text',
	'app.btn3' => 'Button 3 text',
	'app.btn4' => 'Button 4 text',
	'app.btn5' => 'Button 5 text',

	'color.0' => 'Black',
	'color.1' => 'Dark Red',
	'color.2' => 'Dark Green',
	'color.3' => 'Dim Yellow',
	'color.4' => 'Deep Blue',
	'color.5' => 'Dark Violet',
	'color.6' => 'Dark Cyan',
	'color.7' => 'Silver',
	'color.8' => 'Gray',
	'color.9' => 'Light Red',
	'color.10' => 'Light Green',
	'color.11' => 'Light Yellow',
	'color.12' => 'Light Blue',
	'color.13' => 'Light Violet',
	'color.14' => 'Light Cyan',
	'color.15' => 'White',

	'net.explain_sta' => '
		Those settings affect the built-in DHCP client used for 
		connecting to an external network. Switching DHCP (dynamic IP) off
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

	'net.sta_dhcp_enable' => 'Use dynamic IP',
	'net.sta_addr_ip' => 'ESPTerm static IP',
	'net.sta_addr_mask' => 'Subnet mask',
	'net.sta_addr_gw' => 'Gateway IP',

	'wifi.ap' => 'Built-in Access Point',
	'wifi.sta' => 'Connect to External Network',

	'wifi.enable' => 'Enabled',
	'wifi.tpw' => 'Transmit power',
	'wifi.ap_channel' => 'Channel',
	'wifi.ap_ssid' => 'AP SSID',
	'wifi.ap_password' => 'Password',
	'wifi.ap_hidden' => 'Hide SSID',
	'wifi.sta_info' => 'Selected',

	'wifi.not_conn' => 'Not connected.',
	'wifi.sta_none' => 'None',
	'wifi.sta_active_pw' => '🔒',
	'wifi.sta_active_nopw' => '🔓 Open access',
	'wifi.connected_ip_is' => 'Connected, IP is ',
	'wifi.sta_password' => 'Password:',

	'wifi.scanning' => 'Scanning',
	'wifi.scan_now' => 'Start scanning!',
	'wifi.cant_scan_no_sta' => 'Can\'t scan with Client mode disabled.',
	'wifi.select_ssid' => 'Available networks:',

	'wifi.conn.status' => 'Status:',
	'wifi.conn.back_to_config' => 'Back to WiFi config',
	'wifi.conn.telemetry_lost' => 'Telemetry lost, something went wrong. Try again...',

	'wifi.conn.disabled' =>"Station mode is disabled.",
	'wifi.conn.idle' =>"Idle, not connected and with no IP.",
	'wifi.conn.success' => "Connected! Received IP ",
	'wifi.conn.working' => "Connecting to selected AP",
	'wifi.conn.fail' => "Connection failed, check settings & try again. Cause: ",

	'admin.confirm_restore' => 'Restore all settings to their default values?',
	'admin.confirm_restore_hard' =>
		'Restore to firmware default settings? This will reset ' .
		'all active settings and switch to AP mode with the default SSID.',
	'admin.confirm_store_defaults' =>
		'Enter admin password to confirm you want to store the current settings as defaults.',
	'admin.password' => 'Admin password:',
	'admin.restore_defaults' => 'Reset to default settings',
	'admin.write_defaults' => 'Save current settings as default',
	'admin.restore_hard' => 'Reset to firmware default settings',
	'admin.explain' => '
		ESPTerm contains two persistent memory banks, one for default and 
		one for active settings. Active settings can be stored as defaults 
		by the administrator. Use the following button to revert all 
		active settings to their stored default values.  
		',

	'apply' => 'Apply!',
	'enabled' => 'Enabled',
	'disabled' => 'Disabled',
	'yes' => 'Yes',
	'no' => 'No',
	'confirm' => 'OK',
	'form_errors' => 'Validation errors for:',
];
