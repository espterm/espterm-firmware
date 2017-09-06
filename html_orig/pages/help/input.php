
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
			_Application Numpad Mode_, _SS3 Fn Keys Mode_). Some can be set in the <a href="<?= url('cfg_term') ?>">Terminal Settings</a>,
			others via commands.
		</p>

		<p>
			Here are some examples of control key codes:
		</p>

		<table>
			<thead><tr><th>Key</th><th>Code</th><th>Key</th><th>Code</th></tr></thead>
			<tr>
				<td>Up</td>
				<td>`\e[A`,~`\eOA`</td>
				<td>F1</td>
				<td>`\eOP`,~`\e[11\~`</td>
			</tr>
			<tr>
				<td>Down</td>
				<td>`\e[B`,~`\eOB`</td>
				<td>F2</td>
				<td>`\eOQ`,~`\e[12\~`</td>
			</tr>
			<tr>
				<td>Right</td>
				<td>`\e[C`,~`\eOC`</td>
				<td>F3</td>
				<td>`\eOR`,~`\e[13\~`</td>
			</tr>
			<tr>
				<td>Left</td>
				<td>`\e[D`,~`\eOD`</td>
				<td>F4</td>
				<td>`\eOS`,~`\e[14\~`</td>
			</tr>
			<tr>
				<td>Home</td>
				<td>`\eOH`,~`\e[H`,~`\e[1\~`</td>
				<td>F5</td>
				<td>`\e[15~`</td>
			</tr>
			<tr>
				<td>End</td>
				<td>`\eOF`,~`\e[F`,~`\e[4\~`</td>
				<td>F6</td>
				<td>`\e[17\~`</td>
			</tr>
			<tr>
				<td>Insert</td>
				<td>`\e[2\~`</td>
				<td>F7</td>
				<td>`\e[18\~`</td>
			</tr>
			<tr>
				<td>Delete</td>
				<td>`\e[3\~`</td>
				<td>F8</td>
				<td>`\e[19\~`</td>
			</tr>
			<tr>
				<td>Page Up</td>
				<td>`\e[5\~`</td>
				<td>F9</td>
				<td>`\e[20\~`</td>
			</tr>
			<tr>
				<td>Page Down</td>
				<td>`\e[6\~`</td>
				<td>F10</td>
				<td>`\e[21\~`</td>
			</tr>
			<tr>
				<td>Enter</td>
				<td>`\r` (13)</td>
				<td>F11</td>
				<td>`\e[23\~`</td>
			</tr>
			<tr>
				<td>Ctrl+Enter</td>
				<td>`\n` (10)</td>
				<td>F12</td>
				<td>`\e[24\~`</td>
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
