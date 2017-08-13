var Screen = (function () {
	var W = 0, H = 0; // dimensions
	var inited = false;

	var cursor = {
		a: false,        // active (blink state)
		x: 0,            // 0-based coordinates
		y: 0,
		fg: 7,           // colors 0-15
		bg: 0,
		attrs: 0,
		suppress: false, // do not turn on in blink interval (for safe moving)
		hidden: false    // do not show
	};

	var screen = [];
	var blinkIval;

	var frakturExceptions = {
		'C': '\u212d',
		'H': '\u210c',
		'I': '\u2111',
		'R': '\u211c',
		'Z': '\u2128',
	};

	/** Get cell under cursor */
	function _curCell() {
		return screen[cursor.y*W + cursor.x];
	}

	/** Safely move cursor */
	function cursorSet(y, x) {
		// Hide and prevent from showing up during the move
		cursor.suppress = true;
		_draw(_curCell(), false);
		cursor.x = x;
		cursor.y = y;
		// Show again
		cursor.suppress = false;
		_draw(_curCell());
	}

	/** Update cell on display. inv = invert (for cursor) */
	function _draw(cell, inv) {
		if (typeof inv == 'undefined') {
			inv = cursor.a && cursor.x == cell.x && cursor.y == cell.y;
		}

		var elem = cell.e, fg, bg, cn, t;
		// Colors
		fg = inv ? cell.bg : cell.fg;
		bg = inv ? cell.fg : cell.bg;
		// Update
		elem.textContent = t = (cell.t + ' ')[0];

		cn = 'fg' + fg + ' bg' + bg;
		if (cell.attrs & (1<<0)) cn += ' bold';
		if (cell.attrs & (1<<2)) cn += ' italic';
		if (cell.attrs & (1<<3)) cn += ' under';
		if (cell.attrs & (1<<4)) cn += ' blink';
		if (cell.attrs & (1<<5)) {
			cn += ' fraktur';
			// perform substitution
			if (t >= 'a' && t <= 'z') {
				t = String.fromCodePoint(0x1d51e - 97 + t.charCodeAt(0));
			}
			else if (t >= 'A' && t <= 'Z') {
				// this set is incomplete, some exceptions are needed
				if (frakturExceptions.hasOwnProperty(t)) {
					t = frakturExceptions[t];
				} else {
					t = String.fromCodePoint(0x1d504 - 65 + t.charCodeAt(0));
				}
			}
			elem.textContent = t;
		}
		if (cell.attrs & (1<<6)) cn += ' strike';

		if (cell.attrs & (1<<1)) {
			cn += ' faint';
			// faint requires special html - otherwise it would also dim the background.
			// we use opacity on the text...
			elem.innerHTML = '<span>' + e(elem.textContent) + '</span>';
		}

		elem.className = cn;
	}

	/** Show entire screen */
	function _drawAll() {
		for (var i = W*H-1; i>=0; i--) {
			_draw(screen[i]);
		}
	}

	function _rebuild(rows, cols) {
		W = cols;
		H = rows;

		/* Build screen & show */
		var e, cell, scr = qs('#screen');

		// Empty the screen node
		while (scr.firstChild) scr.removeChild(scr.firstChild);

		screen = [];

		for(var i = 0; i < W*H; i++) {
			e = mk('span');

			(function() {
				var x = i % W;
				var y = Math.floor(i / W);
				e.addEventListener('click', function () {
					Input.onTap(y, x);
				});
			})();

			/* End of line */
			if ((i > 0) && (i % W == 0)) {
				scr.appendChild(mk('br'));
			}
			/* The cell */
			scr.appendChild(e);

			cell = {
				t: ' ',
				fg: cursor.fg,
				bg: cursor.bg,
				attrs: 0,
				e: e,
				x: i % W,
				y: Math.floor(i / W),
			};
			screen.push(cell);
			_draw(cell);
		}
	}

	/** Init the terminal */
	function _init() {
		/* Cursor blinking */
		clearInterval(blinkIval);
		blinkIval = setInterval(function () {
			cursor.a = !cursor.a;
			if (cursor.hidden) {
				cursor.a = false;
			}

			if (!cursor.suppress) {
				_draw(_curCell(), cursor.a);
			}
		}, 500);

		// blink attribute
		setInterval(function () {
			$('#screen').removeClass('blink-hide');
			setTimeout(function() {
				$('#screen').addClass('blink-hide');
			}, 800); // 200 ms ON
		}, 1000);

		inited = true;
	}

	/** Decode two-byte number */
	function parse2B(s, i) {
		return (s.charCodeAt(i++) - 1) + (s.charCodeAt(i) - 1) * 127;
	}

	/** Decode three-byte number */
	function parse3B(s, i) {
		return (s.charCodeAt(i) - 1) + (s.charCodeAt(i+1) - 1) * 127 + (s.charCodeAt(i+2) - 1) * 127 * 127;
	}

	var SEQ_SET_COLOR_ATTR = 1;
	var SEQ_REPEAT = 2;
	var SEQ_SET_COLOR = 3;
	var SEQ_SET_ATTR = 4;

	function _load_content(str) {
		var i = 0, ci = 0, j, jc, num, num2, t = ' ', fg, bg, attrs, cell;

		if (!inited) _init();

		// Set size
		num = parse2B(str, i); i += 2;  // height
		num2 = parse2B(str, i); i += 2; // width
		if (num != H || num2 != W) {
			_rebuild(num, num2);
		}
		// console.log("Size ",num, num2);

		// Cursor position
		num = parse2B(str, i); i += 2; // row
		num2 = parse2B(str, i); i += 2; // col
		cursorSet(num, num2);
		// console.log("Cursor at ",num, num2);

		// Attributes
		num = parse2B(str, i); i += 2; // fg bg bold hidden
		cursor.fg = num & 0x0F;
		cursor.bg = (num & 0xF0) >> 4;
		cursor.hidden = !(num & 0x100);

		fg = cursor.fg;
		bg = cursor.bg;
		attrs = 0;

		// Here come the content
		while(i < str.length && ci<W*H) {

			j = str[i++];
			jc = j.charCodeAt(0);
			if (jc == SEQ_SET_COLOR_ATTR) {
				num = parse3B(str, i); i += 3;
				fg = num & 0x0F;
				bg = (num & 0xF0) >> 4;
				attrs = (num & 0xFF00)>>8;
			}
			else if (jc == SEQ_SET_COLOR) {
				num = parse2B(str, i); i += 2;
				fg = num & 0x0F;
				bg = (num & 0xF0) >> 4;
			}
			else if (jc == SEQ_SET_ATTR) {
				num = parse2B(str, i); i += 2;
				attrs = num & 0xFF;
			}
			else if (jc == SEQ_REPEAT) {
				num = parse2B(str, i); i += 2;
				// console.log("Repeat x ",num);
				for (; num>0 && ci<W*H; num--) {
					cell = screen[ci++];
					cell.fg = fg;
					cell.bg = bg;
					cell.t = t;
					cell.attrs = attrs;
				}
			}
			else {
				cell = screen[ci++];
				// Unique cell character
				t = cell.t = j;
				cell.fg = fg;
				cell.bg = bg;
				cell.attrs = attrs;
				// console.log("Symbol ", j);
			}
		}

		_drawAll();
	}

	function _load_labels(str) {
		var pieces = str.split('\x01');
		qs('h1').textContent = pieces[0];
		qsa('#buttons button').forEach(function(x, i) {
			var s = pieces[i+1].trim();
			// if empty string, use the "dim" effect and put nbsp instead to stretch the btn vertically
			x.innerHTML = s.length >0 ? e(s) : "&nbsp;";
			x.style.opacity = s.length > 0 ? 1 : 0.2;
		});
	}

	/** Load screen content from a binary sequence (new) */
	function load(str) {
		var content = str.substr(1);
		switch(str.charAt(0)) {
			case 'S':
				_load_content(content);
				break;
			case 'T':
				_load_labels(content);
				break;
			default:
				console.warn("Bad data message type, ignoring.");
		}
	}

	return  {
		load: load, // full load (string)
	};
})();

/** Handle connections */
var Conn = (function() {
	var ws;

	function onOpen(evt) {
		console.log("CONNECTED");
	}

	function onClose(evt) {
		console.warn("SOCKET CLOSED, code "+evt.code+". Reconnecting...");
		setTimeout(function() {
			init();
		}, 200);
		// this happens when the buffer gets fucked up via invalid unicode.
		// we basically use polling instead of socket then
	}

	function onMessage(evt) {
		try {
			//console.log("RX: ", evt.data);
			// Assume all our messages are screen updates
			Screen.load(evt.data);
		} catch(e) {
			console.error(e);
		}
	}

	function doSend(message) {
		console.log("TX: ", message);
		if (!ws) return; // for dry testing
		if (ws.readyState != 1) {
			console.error("Socket not ready");
			return;
		}
		if (typeof message != "string") {
			message = JSON.stringify(message);
		}
		ws.send(message);
	}

	function init() {
		ws = new WebSocket("ws://"+_root+"/term/update.ws");
		ws.onopen = onOpen;
		ws.onclose = onClose;
		ws.onmessage = onMessage;

		console.log("Opening socket.");

		// Ask for initial data
		$.get('http://'+_root+'/term/init', function(resp, status) {
			if (status !== 200) location.reload(true);
			console.log("Data received!");
			Screen.load(resp);

			showPage();
		});
	}

	return {
		ws: null,
		init: init,
		send: doSend
	};
})();

/** User input */
var Input = (function() {
	function sendStrMsg(str) {
		Conn.send("STR:"+str);
	}

	function sendPosMsg(y, x) {
		Conn.send("TAP:"+y+','+x);
	}

	function sendBtnMsg(n) {
		Conn.send("BTN:"+n);
	}

	function init() {
		window.addEventListener('keypress', function(e) {
			var code = +e.which;
			if (code >= 32 && code < 127) {
				var ch = String.fromCharCode(code);
				//console.log("Typed ", ch, "code", code, e);
				sendStrMsg(ch);
			}
		});

		window.addEventListener('keydown', function(e) {
			var code = e.keyCode;
			//console.log("Down ", code, e);
			switch(code) {
				case 8: sendStrMsg('\x08'); break;
				case 9: sendStrMsg('\x09'); break;
				case 10:
				case 13: sendStrMsg('\x0d\x0a'); break;
				case 27: sendStrMsg('\x1b'); break; // this allows to directly enter control sequences
				case 37: sendStrMsg('\x1b[D'); break;
				case 38: sendStrMsg('\x1b[A'); break;
				case 39: sendStrMsg('\x1b[C'); break;
				case 40: sendStrMsg('\x1b[B'); break;
			}
		});

		qsa('#buttons button').forEach(function(s) {
			s.addEventListener('click', function() {
				sendBtnMsg(+this.dataset['n']);
			});
		});
	}

	return {
		init: init,
		onTap: sendPosMsg,
		sendString: sendStrMsg,
	};
})();

window.termInit = function () {
	Conn.init();
	Input.init();
};
