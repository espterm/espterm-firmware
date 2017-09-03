<div class="Box fold">
	<h2>Tips & Troubleshooting</h2>

	<div class="Row v">
		<img src="/img/adapter.jpg" class="aside" alt="ESPTerm v2">
		<ul>
			<li><b>Communication UART (Rx, Tx)</b> can be configured in the <a href="<?= url('cfg_system') ?>">System Settings</a>.

			<li><b>Boot log and debug messages</b> are available on pin <b>GPIO2</b> (P2) at 115200\,baud, 1 stop bit, no parity.
				Those messages may be disabled through compile flags.

			<li><b>Loopback test</b>: Connect the Rx and Tx pins with a piece of wire. Anything you type in the browser should
				appear on the screen. Set Parser Timeout = 0 in <a href="<?= url('cfg_term') ?>">Terminal Settings</a>
				to be able to manually enter escape sequences.

			<li><b>For best performance</b>, use the module in Client mode (connected to external network) and minimize the number
				of simultaneous connections. Enabling AP consumes extra RAM and strains the processor.

			<li>In AP mode, <b>check that the WiFi channel used is clear</b>; interference may cause flaky connection.
				A good mobile app to use for this is
				<a href="https://play.google.com/store/apps/details?id=com.farproc.wifi.analyzer">WiFi Analyzer (Google Play)</a>.
				Adjust the hotspot strength and range using the Tx Power setting.

			<li>Hold the BOOT button (GPIO0 to GND) for ~1 second to force enable AP. Hold it for ~6 seconds to restore default settings.
				(This is indicated by the blue LED rapidly flashing). Default settings can be overwritten in the
				<a href="<?= url('cfg_system') ?>">System Settings</a>.
		</ul>
	</div>
</div>

<div class="Box fold">
	<h2>Escape Sequences Intro</h2>

	<div class="Row v">
		<img src="/img/vt100.jpg" class="aside" alt="VT102">

		<p>ESPTerm emulates VT102 (pictured) with some additions from later VT models and <code>xterm</code>.
			All commonly used attributes and commands are supported.
			ESPTerm is capable of displaying ncurses applications such as <i>Midnight Commander</i> using <i>agetty</i>.</p>

		<p>
			ESPTerm accepts UTF-8 characters received on the communication UART and displays them on the screen,
			interpreting some codes as Control Characters. Those are e.g. Carriage Return (13), Line Feed (10), Tab (9), Backspace (8) and Bell (7).
			Escape sequences start with the control character ESC (27), followed by any number of ASCII characters
			forming the body of the command. On this page, we'll use <code>\e</code> to indicate ESC.
		</p>

		<p>
			Escape sequences can be divided based on their first character and structure. Most common types are:
		</p>

		<ul>
			<li>CSI commands: <code>\e[</code> followed by 1 optional leading character, multiple numbers divided by
				semicolons, and one or two trailing characters. Those control the cursor, set attributes and manipulate screen content. E.g. <code>\e[?7;10h</code>, <code>\e[2J</code></li>
			<li>SGR commands: CSI terminated by <code>m</code>. Those set text attributes and colors. E.g. <code>\e[</code></li>
			<li>OSC commands: <code>\e]</code> followed by any number of UTF-8 characters (ESPTerm supports up to 64)
				terminated by <code>\e\\</code> (ESC and backslash) or Bell (7). Those are used to exchange text strings.</li>
			<li>There are a few other commands that don't follow this pattern, such as <code>\e7</code> or
				<code>\e#8</code>, mostly for historical reasons.</li>
		</ul>

		<p>A list of the most important escape sequences is presented in the following sections.</p>
	</div>
</div>

<div class="Box fold">
	<h2>Screen Behavior</h2>

	<div class="Row v">
		<p>The initial screen size, title text and button labels can be configured in <a href="<?= url('cfg_term') ?>">Terminal Settings</a>.</p>

		<p>
			Screen updates are sent to the browser through a WebSocket after some time of inactivity on the communication UART
			(called "Redraw Delay"). After an update is sent, at least a time of "Redraw Cooldown" must elapse before the next
			update can be sent. Those delays are used is to avoid burdening the server with tiny updates during a large screen
			repaint. If you experience issues (broken image due to dropped bytes), try adjusting those config options. It may also
			be useful to try different baud rates.
		</p>
	</div>
</div>

<div class="Box fold">
	<h2>Text Attributes</h2>

	<div class="Row v">
		<p>
			All text attributes are set using SGR commands like <code>\e[10;20;30m</code>, with up to 10 numbers separated by semicolons.
			To restore all attributes to their default states, use SGR 0: <code>\e[0m</code> or <code>\e[m</code>.
		</p>

		<p>Those are the supported text attributes SGR codes:</p>

		<table>
			<thead><tr><th>Style</th><th>Enable</th><th>Disable</th></tr></thead>
			<tbody>
			<tr><td><b>Bold</b></td><td>1</td><td>21, 22</td></tr>
			<tr><td style="opacity:.6">Faint</td><td>2</td><td>22</td></tr>
			<tr><td><i>Italic</i></td><td>3</td><td>23</td></tr>
			<tr><td><u>Underlined</u></td><td>4</td><td>24</td></tr>
			<tr><td>Blink</td><td>5</td><td>25</td></tr>
			<tr><td><span style="color:black;background:#ccc;">Inverse</span></td><td>7</td><td>27</td></tr>
			<tr><td><s>Striked</s></td><td>9</td><td>29</td></tr>
			<tr><td>𝔉𝔯𝔞𝔨𝔱𝔲𝔯</td><td>20</td><td>23</td></tr>
			</tbody>
		</table>
	</div>
</div>

<div class="Box fold theme-0">
	<h2>Colors</h2>

	<div class="Row v">
		<p>Colors are set using SGR commands (like <code>\e[10;20;30m</code>). The following tables list the SGR codes to use.
			Selected colors are used for any new text entered, as well as for empty space when using line and screen clearing commands.
			The configured default colors can be restored using SGR 39 for foreground and SGR 49 for background.</p>

		<p>The actual color representation depends on a color theme which
			can be selected in <a href="<?= url('cfg_term') ?>">Terminal Settings</a>.</p>

		<h3>Foreground colors</h3>

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

		<h3>Background colors</h3>

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
	</div>
</div>

<div class="Box fold">
	<h2>User Input</h2>

	<div class="Row v">
		<h3>Keyboard</h3>

		<p>
			The user can input text using their keyboard, or on Android, using the on-screen keyboard which is open using
			a button beneath the screen. Supported are all printable characters, as well as many control keys, such as arrows, Ctrl+letters
			and function keys. Sequences sent by function keys are based on VT102 and xterm.
		</p>

		<p>
			The codes sent by Home, End, F1-F4 and cursor keys are affected by various keyboard modes (Application Cursor Keys,
			Application Numpad Mode, SS3 Fn Keys Mode).
			Some can be set in the <a href="<?= url('cfg_term') ?>">Terminal Settings</a>, others via commands.
		</p>

		<p>
			Here are some examples of control key codes:
		</p>

		<table>
			<thead><tr><th>Key</th><th>Code</th><th>Key</th><th>Code</th></tr></thead>
			<tr>
				<td>🡑</td>
				<td><code>\e[A</code></td>
				<td>F1</td>
				<td><code>\eOP</code></td>
			</tr>
			<tr>
				<td>🡓</td>
				<td><code>\e[B</code></td>
				<td>F2</td>
				<td><code>\eOQ</code></td>
			</tr>
			<tr>
				<td>🡒</td>
				<td><code>\e[C</code></td>
				<td>F3</td>
				<td><code>\eOR</code></td>
			</tr>
			<tr>
				<td>🡐</td>
				<td><code>\e[D</code></td>
				<td>F4</td>
				<td><code>\eOS</code></td>
			</tr>
			<tr>
				<td>Home</td>
				<td><code>\eOH</code></td>
				<td>F5</td>
				<td><code>\e[15~</code></td>
			</tr>
			<tr>
				<td>End</td>
				<td><code>\eOF</code></td>
				<td>F6</td>
				<td><code>\e[17~</code></td>
			</tr>
			<tr>
				<td>Insert</td>
				<td><code>\e[2~</code></td>
				<td>F7</td>
				<td><code>\e[18~</code></td>
			</tr>
			<tr>
				<td>Delete</td>
				<td><code>\e[3~</code></td>
				<td>F8</td>
				<td><code>\e[19~</code></td>
			</tr>
			<tr>
				<td>Page Up</td>
				<td><code>\e[5~</code></td>
				<td>F9</td>
				<td><code>\e[20~</code></td>
			</tr>
			<tr>
				<td>Page Down</td>
				<td><code>\e[6~</code></td>
				<td>F10</td>
				<td><code>\e[21~</code></td>
			</tr>
			<tr>
				<td>Enter</td>
				<td><code>CR (13)</code></td>
				<td>F11</td>
				<td><code>\e[23~</code></td>
			</tr>
			<tr>
				<td>Ctrl+Enter</td>
				<td><code>LF (10)</code></td>
				<td>F12</td>
				<td><code>\e[24~</code></td>
			</tr>
			<tr>
				<td>Tab</td>
				<td><code>TAB (9)</code></td>
				<td>ESC</td>
				<td><code>ESC (27)</code></td>
			</tr>
			<tr>
				<td>Backspace</td>
				<td><code>BS (8)</code></td>
				<td>Ctrl+A..Z</td>
				<td><code>ASCII 1-26</code></td>
			</tr>
		</table>

		<h3>Action buttons</h3>

		<p>
			The blue buttons under the screen send ASCII codes 1, 2, 3, 4, 5, which incidentally correspond to Ctrl+A,B,C,D,E.
			This choice was made to make button press parsing as simple as possible.
		</p>

		<h3>Mouse</h3>

		<p>
			ESPTerm in the current version does not implement standard mouse input. Instead, clicks/taps produce CSI sequences
			<code>\e[<i>r</i>;<i>c</i>M</code> (row, column). You can use this for virtual on-screen buttons.
		</p>
	</div>
</div>

<div class="Box fold">
	<h2>Cursor Commands</h2>

	<div class="Row v">
		<p>
			The coordinates are 1-based, origin is top left. The cursor can move within the entire screen,
			or in the active Scrolling Region if Origin Mode is enabled.
		</p>

		<p>After writing a character, the cursor advances to the right. If it has reached the end of the row,
			it stays on the same line, but writing the next character makes it jump to the start of the next
			line first, scrolling up if needed. If Auto-wrap mode is disabled, the cursor never wraps or scrolls
			the screen.
		</p>

		<p>
			<b>Legend:</b>
			Italic letters such as <i>n</i> are ASCII numbers that serve as arguments, separated with a semicolon.
			If an argument is left out, it's treated as 0 or 1, depending on what makes sense for the command.
		</p>

		<h3>Movement</h3>

		<table class="ansiref w100">
			<thead><tr><th>Code</th><th>Meaning</th></tr></thead>
			<tbody>
			<tr>
				<td>
					\e[<i>n</i>A<br>
					\e[<i>n</i>B<br>
					\e[<i>n</i>C<br>
					\e[<i>n</i>D
				</td>
				<td>Move cursor up (A), down (B), right (C), left (D)</td>
			</tr>
			<tr>
				<td>
					\e[<i>n</i>F<br>
					\e[<i>n</i>E
				</td>
				<td>Go <i>n</i> lines up (F) or down (E), start of line</td>
			</tr>
			<tr>
				<td>
					\e[<i>r</i>d<br>
					\e[<i>c</i>G<br>
					\e[<i>r</i>;<i>c</i>H
				</td>
				<td>
					Go to absolute position - row (d), column (G), or both (H). Use <code>\e[H</code> to go to 1,1.
				</td>
			</tr>
			<tr>
				<td>\e[6n</td>
				<td>Query cursor position. Sent back as <code>\e[<i>r</i>;<i>c</i>R</code>.</td>
			</tr>
			</tbody>
		</table>

		<h3>Save / restore</h3>

		<table class="ansiref w100">
			<thead><tr><th>Code</th><th>Meaning</th></tr></thead>
			<tbody>
			<tr>
				<td>\e[s<br>\e[u</td>
				<td>Save (s) or restore (u) cursor position</td>
			</tr>
			<tr>
				<td>\e7<br>\e8</td>
				<td>Save (7) or restore (8) cursor position and attributes</td>
			</tr>
			</tbody>
		</table>

		<h3>Scrolling Region</h3>

		<table class="ansiref w100">
			<thead><tr><th>Code</th><th>Meaning</th></tr></thead>
			<tbody>
			<tr>
				<td>\e[<i>a</i>;<i>b</i>r</td>
				<td>Set scrolling region to rows <i>a</i> through <i>b</i> and go to 1,1. By default, the
					scrolling region spans the entire screen height. The cursor can leave the region using
					absolute position commands, unless Origin Mode (see below) is active.</td>
			</tr>
			<tr>
				<td>\e[?6h<br>\e[?6l</td>
				<td>Enable (h) or disable (l) Origin Mode and go to 1,1. In Origin Mode, all coordinates
					are relative to the Scrolling Region and the cursor can't leave the region.</td>
			</tr>
			<tr>
				<td>
					\e[<i>n</i>S<br>
					\e[<i>n</i>T
				</td>
				<td>Move contents of the Scrolling Region up (S) or down (T), pad with empty lines of the current background color.</td>
			</tr>
			</tbody>
		</table>

		<h3>Tab stops</h3>

		<table class="ansiref w100">
			<thead><tr><th>Code</th><th>Meaning</th></tr></thead>
			<tbody>
			<tr>
				<td>\eH</td>
				<td>Set tab stop at the current column. There are, by default, tabs every 8 columns.</td>
			</tr>
			<tr>
				<td>
					\e[<i>n</i>I<br>
					\e[<i>n</i>Z
				</td>
				<td>Advance (I) or go back (Z) <i>n</i> tab stops or end/start of line. ASCII TAB (9) is equivalent to <code>\e[1I</code></td>
			</tr>
			<tr>
				<td>
					\e[0g<br>
					\e[3g<br>
				</td>
				<td>Clear tab stop at the current column (0), or all columns (3).</td>
			</tr>
			</tbody>
		</table>

		<h3>Other options</h3>

		<table class="ansiref w100">
			<thead><tr><th>Code</th><th>Meaning</th></tr></thead>
			<tbody>
			<tr>
				<td>\e[?7h<br>\e[?7l</td>
				<td>Enable (h) or disable (l) cursor auto-wrap and screen auto-scroll</td>
			</tr>
			<tr>
				<td>\e[?25h<br>\e[?25l</td>
				<td>Show (h) or hide (l) the cursor</td>
			</tr>
			</tbody>
		</table>
	</div>
</div>

<div class="Box fold">
	<h2>Screen Content Manipulation</h2>

	<div class="Row v">
		<p>
			<b>Legend:</b>
			Italic letters such as <i>n</i> are ASCII numbers that serve as arguments, separated with a semicolon.
			If an argument is left out, it's treated as 0 or 1, depending on what makes sense for the command.
		</p>

		<table class="ansiref w100">
			<thead><tr><th>Code</th><th>Meaning</th></tr></thead>
			<tbody>
			<tr>
				<td>\e[<i>m</i>J</td>
				<td>Clear part of screen. <i>m</i>: 0 - from cursor, 1 - to cursor, 2 - all</td>
			</tr>
			<tr>
				<td>\e[<i>m</i>K</td>
				<td>Erase part of line. <i>m</i>: 0 - from cursor, 1 - to cursor, 2 - all</td>
			</tr>
			<tr>
				<td>
					\e[<i>n</i>X
				</td>
				<td>Erase <i>n</i> characters in line.</td>
			</tr>
			<tr>
				<td>
					\e[<i>n</i>L<br>
					\e[<i>n</i>M
				</td>
				<td>Insert (L) or delete (M) <i>n</i> lines. Following lines are pulled up or pushed down.</td>
			</tr>
			<tr>
				<td>
					\e[<i>n</i>@<br>
					\e[<i>nP
				</td>
				<td>Insert (@) or delete (P) <i>n</i> characters. The rest of the line is pulled left or pushed right.
					Characters going past the end of line are lost.</td>
			</tr>
			</tbody>
		</table>
	</div>
</div>

<div class="Box fold">
	<h2>Alternate Character Sets</h2>

	<div class="Row v">
		<p>
			ESPTerm implements Alternate Character Sets as a way to print box drawing characters
			and special symbols. A character set can change what each received ASCII character
			is printed as on the screen (eg. "{" is "π" in codepage 0). The implementation is based
			on the original VT devices.
			Since ESPTerm also fully supports UTF-8, you can probably ignore this feature and use
			Unicode directly. It's added for compatibility with some programs that use this.
		</p>

		<p>The following codepages are implemented:</p>

		<ul>
			<li>B - US ASCII (default)</li>
			<li>A - UK ASCII: # replaced with £</li>
			<li>0 - Symbols and basic line drawing (standard DEC alternate character set)</li>
			<li>1 - Symbols and advanced line drawing (based on DOS codepage 437)</li>
		</ul>

		<p>To see what character maps to which symbol, look in the source code or try it. All codepages use codes 32-127, 32 being space.</p>

		<p>
			There are two character set slots, G0 and G1.
			Those slots are selected as active using ASCII codes Shift In and Shift Out (those originally served for shifting
			a red-black typewriter tape). Each slot (G0 and G1) can have a different codepage assigned. G0 and G1 and the active slot number are
			saved and restored with the cursor and cleared with a screen reset (<code>\ec</code>).
		</p>

		<p>The following commands are used:</p>

		<table class="ansiref w100">
			<thead><tr><th>Code</th><th>Meaning</th></tr></thead>
			<tbody>
			<tr>
				<td>\e(<i>x</i></td>
				<td>Set G0 = codepage <i>x</i></td>
			</tr>
			<tr>
				<td>\e)<i>x</i></td>
				<td>Set G1 = codepage <i>x</i></td>
			</tr>
			<tr>
				<td>SO (14)</td>
				<td>Activate G0</td>
			</tr>
			<tr>
				<td>SI (15)</td>
				<td>Activate G1</td>
			</tr>
		</table>
	</div>
</div>

<div class="Box fold">
	<h2>System Commands</h2>

	<div class="Row v">
		<p>
			It's possible to dynamically change the screen title text and action button labels.
			Setting an empty label to a button makes it look disabled. The buttons send ASCII 1-5 when clicked.
			Those changes are not retained after restart.
		</p>

		<table class="ansiref w100">
			<thead><tr><th>Code</th><th>Meaning</th></tr></thead>
			<tbody>
			<tr>
				<td>\ec</td>
				<td>
					Clear screen, reset attributes and cursor.
					The screen size, title and button labels remain unchanged.
				</td>
			</tr>
			<tr>
				<td>\e[5n</td>
				<td>
					Query device status, ESPTerm replies with <code>\e[0n</code> "device is OK".
					Can be used to check if the terminal has booted up and is ready to receive commands.</td>
			</tr>
			<tr>
				<td>CAN (24)</td>
				<td>
					This ASCII code is not a command, but is sent by ESPTerm when it becomes ready to receive commands.
					When this code is received on the UART, it means ESPTerm has restarted and is ready. Use this to detect
					spontaneous restarts which require a full screen repaint.
				</td>
			</tr>
			<tr>
				<td>\e]0;<i>title</i>\a</td>
				<td>Set screen title (this is a standard OSC command)</td>
			</tr>
			<tr>
				<td>
					\e]<i>81</i>;<i>btn1</i>\a<br>
					\e]<i>82</i>;<i>btn2</i>\a<br>
					\e]<i>83</i>;<i>btn3</i>\a<br>
					\e]<i>84</i>;<i>btn4</i>\a<br>
					\e]<i>85</i>;<i>btn5</i>\a<br>
				</td>
				<td>Set button 1-5 label - eg. <code>\e]81;Yes\a</code>
					sets the first button text to "Yes".</td>
			</tr>
			<tr>
				<td>\e[8;<i>r</i>;<i>c</i>t</td>
				<td>Set screen size (this is a command borrowed from xterm)</td>
			</tr>
			</tbody>
		</table>
	</div>
</div>
