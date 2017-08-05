<div class="Box">
	<h2>Troubleshooting</h2>

	<ul>
		<li>ESPTerm communicates with external µC via UART on pins <b>Rx</b> and <b>Tx</b>.
			It can be configured in <a href="<?= url('cfg_system') ?>">System Settings</a>.
			Make sure you're using the correct baud rate and other settings on the other side.

		<li>Boot log and debug messages are available on pin <b>GPIO2</b> (P2) at 115200\,bps, 1 stop bit, no parity.

		<li>ESPTerm uses 3.3\,V logic. For communication with 5\,V logic (eg. Aurdino), 470\,R
			protection resistors are recommended. The ESPTerm breakout board already includes those resistors.

		<li>If the Low Voltage Detector ("LVD") LED on the ESPTerm breakout lights up, your power connections have too high
			resistance or the power supply is insufficient. ESP8266 can consume in short spikes up to 500\,A and
			the supply voltage on the module must not drop below about 2.7\,V. Avoid needless breadboard jumpers and too thin wires.

		<li>Connect Rx and Tx with a piece of wire to verify that the terminal works correctly; you should then immediately
			see what you type in the browser. <i>This won't work if your ESP8266 board has a built-in USB-serial
			converter connected to those pins.</i>

		<li>For best performance, use the module in Client mode (connected to external network). Enabling AP mode consumes
			extra RAM and cycles, since the captive portal DNS server and DHCP server are be loaded.

		<li>In AP mode, check that the channel used by ESPTerm is clear; interference may cause flaky connection. A good mobile app to use for this is <a href="https://play.google.com/store/apps/details?id=com.farproc.wifi.analyzer">WiFi Analyzer (Google Play)</a>

		<li>The terminal browser display needs WebSockets for the real-time updating to work.
			It will not work with some old browsers (Android < 4.4, IE < 10, Opera Mini)
			- see the <a href="https://caniuse.com/websockets/embed/">compatibility chart</a>.

		<li>At most 4 clients can be connected to the AP at the same time. This is a limitation of the ESP8266 SDK.
	</ul>
</div>

<div class="Box">
	<h2>Screen</h2>

	<ul>
		<li>Most ANSI escape sequences are supported.

		<li>The initial screen size and other terminal options can be configured in
			<a href="<?= url('cfg_term') ?>">Terminal Settings</a>

		<li>Display updates are sent to the browser after 20 ms of inactivity on the communication UART.
			This is to avoid hogging the server with tiny updates during large screen repaints.
	</ul>
</div>

<div class="Box theme-0">
	<h2>Color Reference</h2>

	<p>Color is set using <code>\e[&lt;c&gt;m</code> where <code>&lt;c&gt;</code> are numbers separated by semicolons
		(and <code>\e</code> is ESC, code 27). The actual color representation depends on a color theme which
		can be selected in <a href="<?= url('cfg_term') ?>">Terminal Settings</a>.</p>

	<b>Foreground</b>

	<div class="colorprev">
		<span class="bg7 fg0">30</span>
		<span class="bg0 fg1">31</span>
		<span class="bg0 fg2">32</span>
		<span class="bg0 fg3">33</span>
		<span class="bg0 fg4">34</span>
		<span class="bg0 fg5">35</span>
		<span class="bg0 fg6">36</span>
		<span class="bg0 fg7">37</span>
	</div>

	<div class="colorprev">
		<span class="bg0 fg8">90</span>
		<span class="bg0 fg9">91</span>
		<span class="bg0 fg10">92</span>
		<span class="bg0 fg11">93</span>
		<span class="bg0 fg12">94</span>
		<span class="bg0 fg13">95</span>
		<span class="bg0 fg14">96</span>
		<span class="bg0 fg15">97</span>
	</div>

	<b>Background</b>

	<div class="colorprev">
		<span class="bg0 fg15">40</span>
		<span class="bg1 fg15">41</span>
		<span class="bg2 fg15">42</span>
		<span class="bg3 fg0">43</span>
		<span class="bg4 fg15">44</span>
		<span class="bg5 fg15">45</span>
		<span class="bg6 fg15">46</span>
		<span class="bg7 fg0">47</span>
	</div>

	<div class="colorprev">
		<span class="bg8 fg15">100</span>
		<span class="bg9 fg0">101</span>
		<span class="bg10 fg0">102</span>
		<span class="bg11 fg0">103</span>
		<span class="bg12 fg0">104</span>
		<span class="bg13 fg0">105</span>
		<span class="bg14 fg0">106</span>
		<span class="bg15 fg0">107</span>
	</div>

	<p>To make the text bold, use the "bold" attribute (eg. <code>\e[31;40;1m</code> for
		<span style="padding: 2px;" class="bg0 fg1 bold">bold red on black</span>).</p>

	<p>The colors are reset to default using <code>\e[0m</code>. For more info, see the tables below.</p>
</div>

<div class="Box">
	<h2>User Input</h2>

	<p>
		All the user types on their keyboard is sent as-is to the UART, including ESC, ANSI sequences for arrow keys and CR-LF for Enter.
	</p>

	<p>The buttons under the screen send ASCII codes 1, 2, 3, 4, 5. This choice was made to make their parsing as simple as possible.</p>

	<p>
		Mouse input (click/tap) is sent as <code>\e[&lt;y&gt;;&lt;x&gt;M</code>. You can use this for virtual on-screen buttons.
	</p>
</div>

<div class="Box">
	<h2>Supported ANSI Escape Sequences</h2>

	<p>Sequences are started by ASCII code 27 (ESC, <code>\e</code>, <code>\x1b</code>, <code>\033</code>)</p>

	<p>Instead of <code>\a</code> (BELL, ASCII 7) in the commands, you can use `\e\` (ESC + backslash). It's the Text Separator code.</p>

	<h3>Text Attributes</h3>

	<p>
		All text attributes are set using <code>\e[...m</code> where the dots are numbers separated by semicolons.
		You can combine up to 3 attributes in a single command.
	</p>

	<table class="ansiref">
		<thead>
		<tr>
			<th>Attribute</th>
			<th>Meaning</th>
		</tr>
		</thead>
		<tbody>
		<tr>
			<td>0</td>
			<td>Reset text attributes to default</td>
		</tr>
		<tr>
			<td>1</td>
			<td>Bold</td>
		</tr>
		<tr>
			<td>21 or 22</td>
			<td>Bold off</td>
		</tr>
		<tr>
			<td>7</td>
			<td>Inverse (fg/bg swap when printing)</td>
		</tr>
		<tr>
			<td>27</td>
			<td>Inverse OFF</td>
		</tr>
		<tr>
			<td>30-37, 90-97</td>
			<td>Foreground color, normal and bright</td>
		</tr>
		<tr>
			<td>40-47, 100-107</td>
			<td>Background color, normal and bright</td>
		</tr>
		</tbody>
	</table>

	<h3>Cursor</h3>

	<p>Movement commands scroll the screen if needed. The coordinates are 1-based, origin top left.</p>

	<table class="ansiref w100">
		<thead>
		<tr>
			<th>Code</th>
			<th>Params</th>
			<th>Meaning</th>
		</tr>
		</thead>
		<tbody>
		<tr>
			<td>\e[&lt;n&gt;A</td>
			<td>[count]</td>
			<td>Move cursor up</td>
		</tr>
		<tr>
			<td>\e[&lt;n&gt;B</td>
			<td>[count]</td>
			<td>Move cursor down</td>
		</tr>
		<tr>
			<td>\e[&lt;n&gt;C</td>
			<td>[count]</td>
			<td>Move cursor forward (right)</td>
		</tr>
		<tr>
			<td>\e[&lt;n&gt;D</td>
			<td>[count]</td>
			<td>Move cursor backward (left)</td>
		</tr>
		<tr>
			<td>\e[&lt;n&gt;E</td>
			<td>[count]</td>
			<td>Go N lines down, start of line</td>
		</tr>
		<tr>
			<td>\e[&lt;n&gt;F</td>
			<td>[count]</td>
			<td>Go N lines up, start of line</td>
		</tr>
		<tr>
			<td>\e[&lt;n&gt;G</td>
			<td>column</td>
			<td>Go to column</td>
		</tr>
		<tr>
			<td>\e[&lt;y&gt;;&lt;x&gt;G</td>
			<td>[row=1];[col=1]</td>
			<td>Go to row and column</td>
		</tr>
		<tr>
			<td>\e[6n</td>
			<td>--</td>
			<td>Query cursor position. Position is sent back as <code>\e[?;?R</code> with row;column.</td>
		</tr>
		<tr>
			<td>\e[s</td>
			<td>--</td>
			<td>Store position</td>
		</tr>
		<tr>
			<td>\e[u</td>
			<td>--</td>
			<td>Restore position</td>
		</tr>
		<tr>
			<td>\e7</td>
			<td>--</td>
			<td>Store position & attributes</td>
		</tr>
		<tr>
			<td>\e8</td>
			<td>--</td>
			<td>Restore position & attributes</td>
		</tr>
		<tr>
			<td>\e[?25l</td>
			<td>--</td>
			<td>Hide cursor</td>
		</tr>
		<tr>
			<td>\e[?25h</td>
			<td>--</td>
			<td>Show cursor</td>
		</tr>
		<tr>
			<td>\e[?7l</td>
			<td>--</td>
			<td>Disable cursor auto-wrap & auto-scroll at end of screen</td>
		</tr>
		<tr>
			<td>\e[?7h</td>
			<td>--</td>
			<td>Enable cursor auto-wrap & auto-scroll at end of screen</td>
		</tr>
		</tbody>
	</table>

	<h3>Screen</h3>

	<table class="ansiref w100">
		<thead>
		<tr>
			<th>Code</th>
			<th>Params</th>
			<th>Meaning</th>
		</tr>
		</thead>
		<tbody>
		<tr>
			<td>\e[&lt;m&gt;J</td>
			<td>[mode=0]</td>
			<td>Clear screen. Mode: 0 - from cursor, 1 - to cursor, 2 - all</td>
		</tr>
		<tr>
			<td>\e[&lt;m&gt;K</td>
			<td>[mode=0]</td>
			<td>Erase line. Mode: 0 - from cursor, 1 - to cursor, 2 - all</td>
		</tr>
		<tr>
			<td>\e[&lt;n&gt;S</td>
			<td>[lines]</td>
			<td>Scroll screen content up, add empty line at the bottom</td>
		</tr>
		<tr>
			<td>\e[&lt;n&gt;T</td>
			<td>[lines]</td>
			<td>Scroll screen content down, add empty line at the top</td>
		</tr>
		<tr>
			<td>\e]W&lt;r&gt;;&lt;c&gt;\a</td>
			<td>rows;cols</td>
			<td>Set screen size (max ~ 80x30). This also clears the screen. This is a custom ESPTerm OSC command.</td>
		</tr>
		</tbody>
	</table>

	<h3>System Commands</h3>

	<table class="ansiref w100">
		<thead>
		<tr>
			<th>Code</th>
			<th>Params</th>
			<th>Meaning</th>
		</tr>
		</thead>
		<tbody>
		<tr>
			<td>\ec</td>
			<td>--</td>
			<td>
				"Device Reset" - clear screen, reset attributes, show cursor & move it to 1,1.
				The screen size and WiFi settings stay unchanged.
			</td>
		</tr>
		<tr>
			<td>\e[5n</td>
			<td>--</td>
			<td>Query device status, replies with <code>\e[0n</code> "device is OK". Can be used to check if the UART works.</td>
		</tr>
		<tr>
			<td>\e]TITLE=…\a</td>
			<td>Title text</td>
			<td>Set terminal title. Avoid ", ', &lt; and &gt;. This is a custom ESPTerm OSC command.</td>
		</tr>
		<tr>
			<td>\e]BTNn=…\a</td>
			<td>1-5, label</td>
			<td>Set button label. Avoid ", ', &lt; and &gt;. This is a custom ESPTerm OSC command.</td>
		</tr>
		<tr>
			<td>ASCII 24 (18h)</td>
			<td></td>
			<td>This symbol is sent by ESPTerm when it becomes ready to receive commands. It can be used to wait before it becomes
				ready on power-up, or to detect a spontaneous ESPTerm restart - that could happen due to RAM exhaustion due
				to a memory leak, or perhaps a power outage.
			</td>
		</tr>
		</tbody>
	</table>
</div>
