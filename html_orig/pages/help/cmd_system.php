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
				<td>`\e]0;<i>t</i>\a`</td>
				<td>Set screen title to _t_ (this is a standard OSC command)</td>
			</tr>
			<tr>
				<td>
					<code>
						\e]<i>80+n</i>;<i>t</i>\a
					</code>
				</td>
				<td>
					Set label for button _n_ = 1-5 (code 81-85) to _t_ - e.g.`\e]81;Yes\a`
					sets the first button text to "Yes".
				</td>
			</tr>
			<tr>
				<td>
					<code>
						\e]<i>90+n</i>;<i>m</i>\a
					</code>
				</td>
				<td>
					Set message for button _n_ = 1-5 (code 81-85) to _m_ - e.g.`\e]94;iv\a`
					sets the 3rd button to send string "iv" when pressed.
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
				<td>
					<code>
						\e[12h \\
						\e[12l
					</code>
				</td>
				<td>
					Enable (`h`) or disable (`l`) Send-Receive Mode (SRM).
					SRM is the opposite of Local Echo, meaning `\e[12h` disables and `\e[12l` enables Local Echo.
				</td>
			</tr>
			<tr>
				<td>`\e[8;<i>r</i>;<i>c</i>t`</td>
				<td>Set screen size to _r_ rows and _c_ columns (this is a command borrowed from Xterm)</td>
			</tr>
			</tbody>
		</table>
	</div>
</div>
