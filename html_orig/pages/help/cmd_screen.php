<div class="Box fold">
	<h2>Commands: Screen Functions</h2>

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
