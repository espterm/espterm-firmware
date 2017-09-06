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
