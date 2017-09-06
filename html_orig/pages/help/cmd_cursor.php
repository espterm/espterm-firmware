<div class="Box fold">
	<h2>Commands: Cursor Functions</h2>

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
					lines of the current background color. This is similar to what happens when AutoWrap
					is enabled and some text is printed at the very end of the screen.
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
