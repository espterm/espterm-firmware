/** File upload utility */
var TermUpl = (function() {
	var lines, // array of lines without newlines
		line_i, // current line index
		fuTout, // timeout handle for line sending
		send_delay_ms, // delay between lines (ms)
		nl_str, // newline string to use
		curLine, // current line (when using fuOil)
		inline_pos; // Offset in line (for long lines)

	// lines longer than this are split to chunks
	// sending a super-ling string through the socket is not a good idea
	var MAX_LINE_LEN = 128;

	function fuOpen() {
		fuStatus("Ready...");
		Modal.show('#fu_modal', onClose);
		$('#fu_form').toggleClass('busy', false);
		Input.blockKeys(true);
	}

	function onClose() {
		console.log("Upload modal closed.");
		clearTimeout(fuTout);
		line_i = 0;
		Input.blockKeys(false);
	}

	function fuStatus(msg) {
		qs('#fu_prog').textContent = msg;
	}

	function fuSend() {
		var v = qs('#fu_text').value;
		if (!v.length) {
			fuClose();
			return;
		}

		lines = v.split('\n');
		line_i = 0;
		inline_pos = 0; // offset in line
		send_delay_ms = qs('#fu_delay').value;

		// sanitize - 0 causes overflows
		if (send_delay_ms < 0) {
			send_delay_ms = 0;
			qs('#fu_delay').value = send_delay_ms;
		}

		nl_str = {
			'CR': '\r',
			'LF': '\n',
			'CRLF': '\r\n',
		}[qs('#fu_crlf').value];

		$('#fu_form').toggleClass('busy', true);
		fuStatus("Starting...");
		fuSendLine();
	}

	function fuSendLine() {
		if (!$('#fu_modal').hasClass('visible')) {
			// Modal is closed, cancel
			return;
		}

		if (!Conn.canSend()) {
			// postpone
			fuTout = setTimeout(fuSendLine, 1);
			return;
		}

		if (inline_pos == 0) {
			curLine = lines[line_i++] + nl_str;
		}

		var chunk;
		if ((curLine.length - inline_pos) <= MAX_LINE_LEN) {
			chunk = curLine.substr(inline_pos, MAX_LINE_LEN);
			inline_pos = 0;
		} else {
			chunk = curLine.substr(inline_pos, MAX_LINE_LEN);
			inline_pos += MAX_LINE_LEN;
		}

		if (!Input.sendString(chunk)) {
			fuStatus("FAILED!");
			return;
		}

		var all = lines.length;

		fuStatus(line_i+" / "+all+ " ("+(Math.round((line_i/all)*1000)/10)+"%)");

		if (lines.length > line_i || inline_pos > 0) {
			fuTout = setTimeout(fuSendLine, send_delay_ms);
		} else {
			closeWhenReady();
		}
	}

	function closeWhenReady() {
		if (!Conn.canSend()) {
			// stuck in XOFF still, wait to process...
			fuStatus("Waiting for Tx buffer...");
			setTimeout(closeWhenReady, 100);
		} else {
			fuStatus("Done.");
			// delay to show it
			setTimeout(function() {
				fuClose();
			}, 100);
		}
	}

	function fuClose() {
		Modal.hide('#fu_modal');
	}

	return {
		init: function() {
			qs('#fu_file').addEventListener('change', function (evt) {
				var reader = new FileReader();
				var file = evt.target.files[0];
				console.log("Selected file type: "+file.type);
				if (!file.type.match(/text\/.*|application\/(json|csv|.*xml.*|.*script.*)/)) {
					// Deny load of blobs like img - can crash browser and will get corrupted anyway
					if (!confirm("This does not look like a text file: "+file.type+"\nReally load?")) {
						qs('#fu_file').value = '';
						return;
					}
				}
				reader.onload = function(e) {
					var txt = e.target.result.replace(/[\r\n]+/,'\n');
					qs('#fu_text').value = txt;
				};
				console.log("Loading file...");
				reader.readAsText(file);
			}, false);
		},
		close: fuClose,
		start: fuSend,
		open: fuOpen,
	}
})();
