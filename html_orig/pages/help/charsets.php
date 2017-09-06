<div class="Box fold">
	<h2>Alternate Character Sets</h2>

	<div class="Row v">
		<p>
			ESPTerm implements Alternate Character Sets as a way to print box drawing characters
			and special symbols. A character set can change what each received ASCII character
			is printed as on the screen (eg. "{" is "π" in codepage `0`). The implementation is based
			on the original VT devices.
		</p>

		<p>
			Since ESPTerm also supports UTF-8, this feature is the most useful for applications
			which can't print UTF-8 or already use alternate character sets for historical reasons.
		</p>

		<h3>Supported codepages</h3>

		<ul>
			<li>`B` - US ASCII (default)</li>
			<li>`A` - UK ASCII: # replaced with £</li>
			<li>`0` - Symbols and basic line drawing (standard DEC alternate character set)</li>
			<li>`1` - Symbols and advanced line drawing (based on DOS codepage 437, ESPTerm specific)</li>
		</ul>

		<p>
			All codepages use codes 32-127, 32 being space. A character with no entry in the active codepage
			stays unchanged.
		</p>

		<?php
			$codepages = load_esp_charsets();
			foreach($codepages as $name => $cp) {
				echo "<h4>Codepage `$name`</h4>\n";
				echo '<div class="charset">';
				foreach($cp as $point) {
					$dis = $point[1]==$point[2]?' class="none"' : '';
					echo "<div$dis><span>$point[0]</span><span>$point[1]</span><span>$point[2]</span></div>";
				}
				echo '</div>';
			}
		?>

		<h3>Switching commands</h3>

		<p>
			There are two character set slots, G0 and G1.
			Those slots are selected as active using ASCII codes Shift In and Shift Out (those originally served for shifting
			a red-black typewriter tape). Often only G0 is used for simplicity.
		</p>

		<p>
			Each slot (G0 and G1) can have a different codepage assigned. G0 and G1 and the active slot number are
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
