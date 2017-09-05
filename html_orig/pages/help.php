<div class="Box fold">
	<h2>Tips & Troubleshooting</h2>

	<div class="Row v">
		<img src="/img/adapter.jpg" class="aside" alt="ESPTerm v2">
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

<div class="Box fold">
	<h2>Basic Intro & Nomenclature</h2>

	<div class="Row v">
		<img src="/img/vt100.jpg" class="aside" alt="VT102">

		<p>
			ESPTerm emulates VT102 (pictured) with some additions from later VT models and Xterm.
			All commonly used attributes and commands are supported.
			ESPTerm is capable of displaying ncurses applications such as _Midnight Commander_ using _agetty_.
		</p>

		<p>
			ESPTerm accepts UTF-8 characters received on the communication UART and displays them on the screen,
			interpreting some codes as Control Characters. Those are e.g. _Carriage Return_ (13), _Line Feed_ (10),
			_Tab_ (9), _Backspace_ (8) and _Bell_ (7).
		</p>

		<p>
			Escape sequences start with the control character _ESC_ (27),
			followed by any number of ASCII characters forming the body of the command.
		</p>

		<h3>Nomenclature & Command Types</h3>

		<p>
			Examples on this help page use the following symbols for special characters and command types:\\
			(spaces are for clarity only, _DO NOT_ include them in the commands!)
		</p>

		<div class="tscroll">
		<table class="nomen">
			<thead><tr><th>Name</th><th>Symbol</th><th>ASCII</th><th>C string</th><th>Function</th></tr></thead>
			<tbody>
			<tr>
				<td>*ESC*</td>
				<td>`\e`</td>
				<td>`ESC` (27)</td>
				<td>`"\e"`, `"\x1b"`, `"\033"`</td>
				<td>Introduces an escape sequence. _(Note: `\e` is a GCC extension)_</td>
			</tr>
			<tr>
				<td>*Bell*</td>
				<td>`\a`</td>
				<td>`BEL`~(7)</td>
				<td>`"\a"`, `"\x7"`, `"\07"`</td>
				<td>Audible beep</td>
			</tr>
			<tr>
				<td>*String Terminator*</td>
				<td>`ST`</td>
				<td>`ESC \`~(27~92)<br>_or_~`\a`~(7)</td>
				<td>`"\x1b\\"`, `"\a"`</td>
				<td>Terminates a string command (`\a` can be used as an alternative)</td>
			</tr>
			<tr>
				<td>*Control Sequence Introducer*</td>
				<td>`CSI`</td>
				<td>`ESC [`</td>
				<td>`"\x1b["`</td>
				<td>Starts a CSI command. Examples: `\e[?7;10h`, `\e[2J`</td>
			</tr>
			<tr>
				<td>*Operating System Command*</td>
				<td>`OSC`</td>
				<td>`ESC ]`</td>
				<td>`"\x1b]"`</td>
				<td>Starts an OSC command. Is followed by a command string terminated by `ST`. Example: `\e]0;My Screen Title\a`</td>
			</tr>
			<tr>
				<td>*Select Graphic Rendition*</td>
				<td>`SGR`</td>
				<td>`CSI <i>n</i>;<i>n</i>;<i>n</i>m`</td>
				<td>`"\x1b[1;2;3m"`</td>
				<td>Set text attributes, like color or style. 0 to 10 numbers can be used, `\e[m` is treated as `\e[0m`</td>
			</tr>
			</tbody>
		</table>
		</div>

		<p>There are also some other commands that don't follow the CSI, SGR or OSC pattern, such as `\e7` or
			`\e#8`. A list of the most important escape sequences is presented in the following sections.</p>
	</div>
</div>

<div class="Box fold">
	<h2>Screen Behavior & Refreshing</h2>

	<div class="Row v">
		<p>
			The initial screen size, title text and button labels can be configured in <a href="<?= url('cfg_term') ?>">Terminal Settings</a>.
		</p>

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
			All text attributes are set using SGR commands like `\e[10;20;30m`, with up to 10 numbers separated by semicolons.
			To restore all attributes to their default states, use SGR 0: `\e[0m` or `\e[m`.
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
		<p>
			Colors are set using SGR commands (like `\e[10;20;30m`). The following tables list the SGR codes to use.
			Selected colors are used for any new text entered, as well as for empty space when using line and screen clearing commands.
			The configured default colors can be restored using SGR 39 for foreground and SGR 49 for background.
		</p>

		<p>
			The actual color representation depends on a color theme which
			can be selected in <a href="<?= url('cfg_term') ?>">Terminal Settings</a>.
		</p>

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
	<h2>User Input: Keyboard, Mouse</h2>

	<div class="Row v">
		<h3>Keyboard</h3>

		<p>
			The user can input text using their keyboard, or on Android, using the on-screen keyboard which is open using
			a button beneath the screen. Supported are all printable characters, as well as many control keys, such as arrows, _Ctrl+letters_
			and function keys. Sequences sent by function keys are based on VT102 and Xterm.
		</p>

		<p>
			The codes sent by _Home_, _End_, _F1-F4_ and cursor keys are affected by various keyboard modes (_Application Cursor Keys_,
			_Application Numpad Mode_, _SS3 Fn Keys Mode_).
			Some can be set in the <a href="<?= url('cfg_term') ?>">Terminal Settings</a>, others via commands.
		</p>

		<p>
			Here are some examples of control key codes:
		</p>

		<table>
			<thead><tr><th>Key</th><th>Code</th><th>Key</th><th>Code</th></tr></thead>
			<tr>
				<td>Up</td>
				<td>`\e[A`</td>
				<td>F1</td>
				<td>`\eOP`</td>
			</tr>
			<tr>
				<td>Down</td>
				<td>`\e[B`</td>
				<td>F2</td>
				<td>`\eOQ`</td>
			</tr>
			<tr>
				<td>Right</td>
				<td>`\e[C`</td>
				<td>F3</td>
				<td>`\eOR`</td>
			</tr>
			<tr>
				<td>Left</td>
				<td>`\e[D`</td>
				<td>F4</td>
				<td>`\eOS`</td>
			</tr>
			<tr>
				<td>Home</td>
				<td>`\eOH`</td>
				<td>F5</td>
				<td>`\e[15~`</td>
			</tr>
			<tr>
				<td>End</td>
				<td>`\eOF`</td>
				<td>F6</td>
				<td>`\e[17~`</td>
			</tr>
			<tr>
				<td>Insert</td>
				<td>`\e[2~`</td>
				<td>F7</td>
				<td>`\e[18~`</td>
			</tr>
			<tr>
				<td>Delete</td>
				<td>`\e[3~`</td>
				<td>F8</td>
				<td>`\e[19~`</td>
			</tr>
			<tr>
				<td>Page Up</td>
				<td>`\e[5~`</td>
				<td>F9</td>
				<td>`\e[20~`</td>
			</tr>
			<tr>
				<td>Page Down</td>
				<td>`\e[6~`</td>
				<td>F10</td>
				<td>`\e[21~`</td>
			</tr>
			<tr>
				<td>Enter</td>
				<td>`\r` (13)</td>
				<td>F11</td>
				<td>`\e[23~`</td>
			</tr>
			<tr>
				<td>Ctrl+Enter</td>
				<td>`\n` (10)</td>
				<td>F12</td>
				<td>`\e[24~`</td>
			</tr>
			<tr>
				<td>Tab</td>
				<td>`\t` (9)</td>
				<td>ESC</td>
				<td>`\e` (27)</td>
			</tr>
			<tr>
				<td>Backspace</td>
				<td>`\b` (8)</td>
				<td>Ctrl+A..Z</td>
				<td>ASCII 1-26</td>
			</tr>
		</table>

		<h3>Action buttons</h3>

		<p>
			The blue buttons under the screen send ASCII codes 1, 2, 3, 4, 5, which incidentally
			correspond to _Ctrl+A,B,C,D,E_. This choice was made to make button press parsing as simple as possible.
		</p>

		<h3>Mouse</h3>

		<p>
			ESPTerm implements standard mouse tracking modes based on Xterm. Mouse tracking can be used to implement
			powerful user interactions such as on-screen buttons, draggable sliders or dials, menus etc. ESPTerm's
			mouse tracking was tested using VTTest and should be compatible with all terminal applications
			that request mouse tracking.
		</p>

		<p>
			Mouse can be tracked in different ways; some are easier to parse, others more powerful. The coordinates
			can also be encoded in different ways. All mouse tracking options are set using option commands:
			`CSI ? _n_ h` to enable, `CSI ? _n_ l` to disable option _n_.
		</p>

		<h4>Mouse Tracking Modes</h4>

		<p>
			All tracking modes produce three numbers which are then encoded and send to the application.
			First is the _event number_ N, then the _X and Y coordinates_, 1-based.
		</p>

		<p>
			Mouse buttons are numbered: 1=left, 2=middle, 3=right.
			Wheel works as two buttons (4 and 5) which generate only press events (no release).
		</p>

		<div class="tscroll">
		<table class="nomen">
			<thead><tr><th>Option</th><th>Name</th><th>Description</th></tr></thead>
			<tr>
				<td>`9`</td>
				<td>*X10~mode*</td>
				<td>
					This is the most basic tracking mode, in which <b>only button presses</b> are reported.
					N = button - 1: (0 left, 1 middle, 2 right, 3, 4 wheel).
				</td>
			</tr>
			<tr>
				<td>`1000`</td>
				<td>*Normal~mode*</td>
				<td>
					In Normal mode, both button presses and releases are reported.
					The lower two bits of N indicate the button pressed:
					`00b` (0) left, `01b` (1) middle, `10b` (2) right, `11b` (3) button release.
					Wheel buttons are reported as 0 and 1 with added 64 (e.g. 64 and 65).
					Normal mode also supports tracking of modifier keys, which are added to N as bit masks:
					4=_Shift_, 8=_Meta/Alt_, 16=_Control/Cmd_. Example: middle button with _Shift_ = 1 + 4 = `101b` (5).
				</td>
			</tr>
			<tr>
				<td>`1002`</td>
				<td>*Button-Event tracking*</td>
				<td>
					This is similar to Normal mode (`1000`), but mouse motion with a button held is also reported.
					A motion event is generated when the mouse cursor moves between screen character cells.
					A motion event has the same N as a press event, but 32 is added.
					For example, drag-drop event with the middle button will produce N = 1 (press), 33 (dragging) and 3 (release).
				</td>
			</tr>
			<tr>
				<td>`1003`</td>
				<td>*Any-Event tracking*</td>
				<td>
					This mode is almost identical to Button Event tracking (1002), but motion events
					are sent even when no mouse buttons are held. This could be used to draw on-screen mouse cursor, for example.
					Motion events with no buttons will use N = 32 + _11b_ (35).
				</td>
			</tr>
			<tr>
				<td>`1004`</td>
				<td>*Focus~tracking*</td>
				<td>
					Focus tracking is a separate function from the other mouse tracking modes, therefore they can be enabled together.
					Focus tracking reports when the terminal window (in Xterm) gets or loses focus, or in ESPTerm's case, when any
					user is connected. This can be used to pause/resume a game or on-screen animations.
					Focus tracking mode sends `CSI I` when the terminal receives, and `CSI O` when it loses focus.
				</td>
			</tr>
		</table>
		</div>

		<h4>Mouse Report Encoding</h4>

		<p>
			The following encoding schemes can be used with any of the tracking modes (except Focus tracking, which is not affected).
		</p>

		<div class="tscroll">
		<table class="nomen">
			<thead><tr><th>Option</th><th>Name</th><th>Description</th></tr></thead>
			<tr>
				<td>--</td>
				<td>*Normal~encoding*</td>
				<td>
					This is the default encoding scheme used when no other option is selected.
					In this mode, a mouse report has the format `CSI M _n_ _x_ _y_`,
					where _n_, _x_ and _y_ are characters with ASCII value = 32 (space) + the respective number, e.g.
					0 becomes 32 (space), 1 becomes 33 (!). The reason for adding 32 is to avoid producing control characters.
					Example: `\e[M !!` - left button press at coordinates 1,1 when using X10 mode.
				</td>
			</tr>
			<tr>
				<td>`1005`</td>
				<td>*UTF-8~encoding*</td>
				<td>
					This scheme should encode each of the numbers as a UTF-8 code point, expanding the maximum possible value.
					Since ESPTerm's screen size is limited and this has no practical benefit, this serves simply as an alias
					to the normal scheme.
				</td>
			</tr>
			<tr>
				<td>`1006`</td>
				<td>*SGR~encoding*</td>
				<td>
					In SGR encoding, the response looks like a SGR sequence with the three numbers as semicolon-separated
					ASCII values. In this case 32 is not added like in the Normal and UTF-8 schemes, because
					it would serve nor purpose here. Also, button release is not reported as 11b,
					but using the normal button code while changing the final SGR character: `M` for button press
					and `m` for button release. Example: `\e[2;80;24m` - the right button was released
					at row 80, column 24.
				</td>
			</tr>
			<tr>
				<td>`1015`</td>
				<td>*URXVT~encoding*</td>
				<td>
					This is similar to SGR encoding, but the final character is always `M` and the numbers are
					like in the Normal scheme, with 32 added. This scheme has no real advantage over the previous schemes and
					was added solely for completeness.
				</td>
			</tr>
		</table>
		</div>
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
			*Legend:*
			Italic letters such as _n_ are ASCII numbers that serve as arguments, separated with a semicolon.
			If an argument is left out, it's treated as 0 or 1, depending on what makes sense for the command.
		</p>

		<h3>Movement</h3>

		<table class="ansiref w100">
			<thead><tr><th>Code</th><th>Meaning</th></tr></thead>
			<tbody>
			<tr>
				<td>
					<code>
						\e[<i>n</i>A \\
						\e[<i>n</i>B \\
						\e[<i>n</i>C \\
						\e[<i>n</i>D
					</code>
				</td>
				<td>Move cursor up (`A`), down (`B`), right (`C`), left (`D`)</td>
			</tr>
			<tr>
				<td>
					<code>
						\e[<i>n</i>F \\
						\e[<i>n</i>E
					</code>
				</td>
				<td>Go _n_ lines up (`F`) or down (`E`), start of line</td>
			</tr>
			<tr>
				<td>
					<code>
						\e[<i>r</i>d \\
						\e[<i>c</i>G \\
						\e[<i>r</i>;<i>c</i>H
					</code>
				</td>
				<td>
					Go to absolute position - row (`d`), column (`G`), or both (`H`). Use `\e[H` to go to 1,1.
				</td>
			</tr>
			<tr>
				<td>
					`\e[6n`
				</td>
				<td>
					Query cursor position. Sent back as `\e[<i>r</i>;<i>c</i>R`.
				</td>
			</tr>
			</tbody>
		</table>

		<h3>Save / restore</h3>

		<table class="ansiref w100">
			<thead><tr><th>Code</th><th>Meaning</th></tr></thead>
			<tbody>
			<tr>
				<td>
					<code>
						\e[s \\
						\e[u
					</code>
				</td>
				<td>Save (`s`) or restore (`u`) cursor position</td>
			</tr>
			<tr>
				<td>
					<code>
						\e7 \\
						\e8
					</code>
				</td>
				<td>Save (`7`) or restore (`8`) cursor position and attributes</td>
			</tr>
			</tbody>
		</table>

		<h3>Scrolling Region</h3>

		<table class="ansiref w100">
			<thead><tr><th>Code</th><th>Meaning</th></tr></thead>
			<tbody>
			<tr>
				<td>
					`\e[<i>a</i>;<i>b</i>r`
				</td>
				<td>
					Set scrolling region to rows _a_ through _b_ and go to 1,1. By default, the
					scrolling region spans the entire screen height. The cursor can leave the region using
					absolute position commands, unless Origin Mode (see below) is active.
				</td>
			</tr>
			<tr>
				<td>
					<code>
						\e[?6h \\
						\e[?6l
					</code>
				</td>
				<td>
					Enable (`h`) or disable (`l`) Origin Mode and go to 1,1. In Origin Mode, all coordinates
					are relative to the Scrolling Region and the cursor can't leave the region.
				</td>
			</tr>
			<tr>
				<td>
					<code>
						\e[<i>n</i>S \\
						\e[<i>n</i>T
					</code>
				</td>
				<td>
					Move contents of the Scrolling Region up (`S`) or down (`T`), pad with empty
					lines of the current background color.
				</td>
			</tr>
			</tbody>
		</table>

		<h3>Tab stops</h3>

		<table class="ansiref w100">
			<thead><tr><th>Code</th><th>Meaning</th></tr></thead>
			<tbody>
			<tr>
				<td>
					`\eH`
				</td>
				<td>
					Set tab stop at the current column. There are, by default, tabs every 8 columns.
				</td>
			</tr>
			<tr>
				<td>
					<code>
						\e[<i>n</i>I \\
						\e[<i>n</i>Z
					</code>
				</td>
				<td>Advance (`I`) or go back (`Z`) _n_ tab stops or end/start of line. ASCII _TAB_ (9) is equivalent to <code>\e[1I</code></td>
			</tr>
			<tr>
				<td>
					<code>
						\e[0g \\
						\e[3g \\
					</code>
				</td>
				<td>Clear tab stop at the current column (`0`), or all columns (`3`).</td>
			</tr>
			</tbody>
		</table>

		<h3>Other options</h3>

		<table class="ansiref w100">
			<thead><tr><th>Code</th><th>Meaning</th></tr></thead>
			<tbody>
			<tr>
				<td>
					<code>
						\e[?7h \\
						\e[?7l
					</code>
				</td>
				<td>Enable (`h`) or disable (`l`) cursor auto-wrap and screen auto-scroll</td>
			</tr>
			<tr>
				<td>
					<code>
						\e[?25h \\
						\e[?25l
					</code>
				</td>
				<td>Show (`h`) or hide (`l`) the cursor</td>
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
			Italic letters such as _n_ are ASCII numbers that serve as arguments, separated with a semicolon.
			If an argument is left out, it's treated as 0 or 1, depending on what makes sense for the command.
		</p>

		<table class="ansiref w100">
			<thead><tr><th>Code</th><th>Meaning</th></tr></thead>
			<tbody>
			<tr>
				<td>
					`\e[<i>m</i>J`
				</td>
				<td>
					Clear part of screen. _m_: 0 - from cursor, 1 - to cursor, 2 - all
				</td>
			</tr>
			<tr>
				<td>
					`\e[<i>m</i>K`
				</td>
				<td>
					Erase part of line. _m_: 0 - from cursor, 1 - to cursor, 2 - all
				</td>
			</tr>
			<tr>
				<td>
					`\e[<i>n</i>X`</td>
				<td>
					Erase _n_ characters in line.
				</td>
			</tr>
			<tr>
				<td>
					<code>
						\e[<i>n</i>L \\
						\e[<i>n</i>M
					</code>
				</td>
				<td>
					Insert (`L`) or delete (`M`) _n_ lines. Following lines are pulled up or pushed down.
				</td>
			</tr>
			<tr>
				<td>
					<code>
						\e[<i>n</i>@ \\
						\e[<i>n</i>P
					</code>
				</td>
				<td>
					Insert (`@`) or delete (`P`) _n_ characters. The rest of the line is pulled left or pushed right.
					Characters going past the end of line are lost.
				</td>
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
			is printed as on the screen (eg. "{" is "π" in codepage `0`). The implementation is based
			on the original VT devices.
			Since ESPTerm also fully supports UTF-8, you can probably ignore this feature and use
			Unicode directly. It's added for compatibility with some programs that use this.
		</p>

		<p>The following codepages are implemented:</p>

		<ul>
			<li>`B` - US ASCII (default)</li>
			<li>`A` - UK ASCII: # replaced with £</li>
			<li>`0` - Symbols and basic line drawing (standard DEC alternate character set)</li>
			<li>`1` - Symbols and advanced line drawing (based on DOS codepage 437)</li>
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
				<td>`\e(<i>x</i>`</td>
				<td>Set G0 = codepage <i>x</i></td>
			</tr>
			<tr>
				<td>`\e)<i>x</i>`</td>
				<td>Set G1 = codepage <i>x</i></td>
			</tr>
			<tr>
				<td>_SO_ (14)</td>
				<td>Activate G0</td>
			</tr>
			<tr>
				<td>_SI_ (15)</td>
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
				<td>`\ec`</td>
				<td>
					Clear screen, reset attributes and cursor.
					The screen size, title and button labels remain unchanged.
				</td>
			</tr>
			<tr>
				<td>`\e[5n`</td>
				<td>
					Query device status, ESPTerm replies with `\e[0n` "device is OK".
					Can be used to check if the terminal has booted up and is ready to receive commands.
				</td>
			</tr>
			<tr>
				<td>_CAN_ (24)</td>
				<td>
					This ASCII code is not a command, but is sent by ESPTerm when it becomes ready to receive commands.
					When this code is received on the UART, it means ESPTerm has restarted and is ready. Use this to detect
					spontaneous restarts which require a full screen repaint.
				</td>
			</tr>
			<tr>
				<td>`\e]0;<i>title</i>\a`</td>
				<td>Set screen title (this is a standard OSC command)</td>
			</tr>
			<tr>
				<td>
					<code>
						\e]<i>81</i>;<i>btn1</i>\a \\
						\e]<i>82</i>;<i>btn2</i>\a \\
						\e]<i>83</i>;<i>btn3</i>\a \\
						\e]<i>84</i>;<i>btn4</i>\a \\
						\e]<i>85</i>;<i>btn5</i>\a \\
					</code>
				</td>
				<td>
					Set button 1-5 label - eg.`\e]81;Yes\a`
					sets the first button text to "Yes".
				</td>
			</tr>
			<tr>
				<td>`\e[8;<i>r</i>;<i>c</i>t`</td>
				<td>Set screen size (this is a command borrowed from xterm)</td>
			</tr>
			</tbody>
		</table>
	</div>
</div>
