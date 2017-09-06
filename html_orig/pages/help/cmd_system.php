<div class="Box fold">
	<h2>Commands: System Functions</h2>

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
				<td>
					<code>
						\e[?800h \\
						\e[?800l
					</code>
				</td>
				<td>
					Show (`h`) or hide (`l`) action buttons (the blue buttons under the screen).
				</td>
			</tr>
			<tr>
				<td>
					<code>
						\e[?801h \\
						\e[?801l
					</code>
				</td>
				<td>
					Show (`h`) or hide (`l`) menu/help links under the screen.
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
