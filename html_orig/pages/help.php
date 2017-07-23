<div class="Box">
	<h2>Wiring</h2>

	<ul>
		<li>Communication UART is on pins <b>Rx, Tx</b> at 115200-8-1-N. The baud rate can be changed in Terminal Settings.
		<li>Debug log is on pin <b>GPIO2</b> (P2) at 115200-8-1-N. This baud rate is fixed.
		<li>Compatible with 3.3&nbsp;V and 5&nbsp;V logic. For 5&nbsp;V, 470&nbsp;R protection resistors are recommended.
		<li>If the "LVD" LED on the ESPTerm module lights up, it doesn't get enough power to run correctly. Check your connections.
		<li>Connect Rx and Tx with a piece of wire to test the terminal alone, you should see what you type in the browser.
			<i>NOTE: This won't work if your ESP8266 board has a built-in USB-serial converter (like NodeMCU).</i>
		<li>For best performance, use the module in Client mode (connected to external network).
		<li>In AP mode, check that the channel used is clear; interference may cause a flaky connection.
	</ul>
</div>

<div class="Box">
	<h2>Screen</h2>

	<ul>
		<li>Most ANSI escape sequences are supported.</li>
		<li>The max screen size is 2000 characters (eg. <b>25x80</b>), default is <b>10x26</b>. Set using <code>\e]W&lt;rows&gt;;&lt;cols&gt;\a</code>.</li>
		<li>The screen will automatically scroll, can be used for log output.</li>
		<li>Display update is sent <b>after 20 ms of inactivity</b>.</li>
		<li>At most 4 clients can be connected at the same time.</li>
		<li>The browser display needs WebSockets for the real-time updating. It may not work on really old phones / browsers.</li>
	</ul>
</div>

<div class="Box">
	<h2>Color Reference</h2>

	<p>Color is set using <code>\e[&lt;c&gt;m</code> or <code>\e[&lt;c&gt;;&lt;c&gt;m</code> where "&lt;c&gt;" are numbers from the following tables:</p>

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
		<span class="bg7 fg8">90</span>
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
		<span class="bg0 fg15 nb">40</span>
		<span class="bg1 fg15 nb">41</span>
		<span class="bg2 fg15 nb">42</span>
		<span class="bg3 fg0">43</span>
		<span class="bg4 fg15 nb">44</span>
		<span class="bg5 fg15 nb">45</span>
		<span class="bg6 fg15 nb">46</span>
		<span class="bg7 fg0">47</span>
	</div>

	<div class="colorprev">
		<span class="bg8 fg15 nb">100</span>
		<span class="bg9 fg0">101</span>
		<span class="bg10 fg0">102</span>
		<span class="bg11 fg0">103</span>
		<span class="bg12 fg0">104</span>
		<span class="bg13 fg0">105</span>
		<span class="bg14 fg0">106</span>
		<span class="bg15 fg0">107</span>
	</div>

	<p>Bright foreground can also be set using the "bold" attribute 1 (eg. <code>\e[31;1m</code>). For more details, see the ANSI code reference below.</p>
</div>

<div class="Box">
	<h2>User Input</h2>

	<p>
		All the user types on their keyboard is sent as-is to the UART, including ESC, ANSI sequences for arrow keys and CR-LF for Enter.
		The input is limited to ASCII codes 32-126, backspace 8 and tab 9.
	</p>

	<p>The buttons under the screen send ASCII codes 1, 2, 3, 4, 5.</p>

	<p>
		<b>Mouse input</b> (click/tap) is sent as <code>\e[&lt;y&gt;;&lt;x&gt;M</code>. You can use this for on-screen buttons, menu navigation etc.
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
			<td>Go N line down, start of line</td>
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
			<td>Set screen size, maximum 25x80 (resp. total 2000 characters). This also clears the screen.</td>
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
			<td>\e]FR\a</td>
			<td>--</td>
			<td>"Factory Reset", emergency code when you mess up the WiFi, restores SSID to unique default, clears stored credentials & enters Client+AP mode.</td>
		</tr>
		<tr>
			<td>\e[5n</td>
			<td>--</td>
			<td>Query device status, replies with <code>\e[0n</code> "device is OK". Can be used to check if the UART works.</td>
		</tr>
		</tbody>
	</table>
</div>
