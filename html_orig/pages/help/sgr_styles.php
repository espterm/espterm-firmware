<div class="Box fold">
	<h2>Commands: Style SGR</h2>

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
