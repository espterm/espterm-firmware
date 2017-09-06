<div class="Box fold">
	<h2>Tips & Troubleshooting</h2>

	<div class="Row v">
		<ul>
			<li>*Communication UART (Rx, Tx)* can be configured in the <a href="<?= url('cfg_system') ?>">System Settings</a>.

			<li>*Boot log and debug messages* are available on pin *GPIO2* (P2) at 115200\,baud, 1 stop bit, no parity.
				Those messages may be disabled through compile flags.

			<li>*Loopback test*: Connect the Rx and Tx pins with a piece of wire. Anything you type in the browser should
				appear on the screen. Set _Parser Timeout = 0_ in <a href="<?= url('cfg_term') ?>">Terminal Settings</a>
				to be able to manually enter escape sequences.

			<li>There is very little RAM available to the webserver, and it can support at most 4 connections at the same time.
				Each terminal session (open window with the terminal screen) uses one persistent connection for screen updates.
				*Avoid leaving unused windows open*, or either the RAM or connections may be exhausted.

			<li>*For best performance*, use the module in Client mode (connected to external network) and minimize the number
				of simultaneous connections. Enabling AP consumes extra RAM because the DHCP server and Captive Portal
				DNS server are started.

			<li>In AP mode, *check that the WiFi channel used is clear*; interference may cause flaky connection.
				A good mobile app to use for this is
				<a href="https://play.google.com/store/apps/details?id=com.farproc.wifi.analyzer">WiFi Analyzer (Google Play)</a>.
				Adjust the hotspot strength and range using the _Tx Power setting_.

			<li>Hold the BOOT button (GPIO0 to GND) for ~1 second to force enable AP. Hold it for ~6 seconds to restore default settings.
				(This is indicated by the blue LED rapidly flashing). Default settings can be overwritten in the
				<a href="<?= url('cfg_system') ?>">System Settings</a>.
		</ul>
	</div>
</div>
