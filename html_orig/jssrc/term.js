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
		forceOn: false,
		hidden: false,    // do not show
		hanging: false,   // xenl
	};

	var screen = [];
	var blinkIval;
	var cursorFlashStartIval;

	var frakturExceptions = {
		'C': '\u212d',
		'H': '\u210c',
		'I': '\u2111',
		'R': '\u211c',
		'Z': '\u2128',
	};

	// for BEL
	var audioCtx = null;
	try {
		audioCtx = new (window.AudioContext || window.audioContext || window.webkitAudioContext)();
	} catch (er) {
		console.error("Browser does not support AudioContext, can't beep.", er);
	}

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
		if (!cell) return;
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
				e.addEventListener('mouseenter', function (evt) {
					Input.onMouseMove(x, y);
				});
				e.addEventListener('mousedown', function (evt) {
					Input.onMouseDown(x, y, evt.button+1);
				});
				e.addEventListener('mouseup', function (evt) {
					Input.onMouseUp(x, y, evt.button+1);
				});
				e.addEventListener('contextmenu', function (evt) {
					evt.preventDefault();
				});
				e.addEventListener('mousewheel', function (evt) {
					Input.onMouseWheel(x, y, evt.deltaY>0?1:-1);
					return false;
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
				fg: 7,
				bg: 0, // the colors will be replaced immediately as we receive data (user won't see this)
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
			// TODO try to invent a new way to indicate "hanging" - this is copied from gtkterm
			if (cursor.hidden || cursor.hanging) {
				cursor.a = false;
			}

			if (!cursor.suppress) {
				_draw(_curCell(), cursor.forceOn || cursor.a);
			}
		}, 500);

		// blink attribute
		setInterval(function () {
			$('#screen').removeClass('blink-hide');
			setTimeout(function () {
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

		var cursorMoved;

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
		cursorMoved = (cursor.x != num2 || cursor.y != num);
		cursorSet(num, num2);
		// console.log("Cursor at ",num, num2);

		// Attributes
		num = parse2B(str, i); i += 2; // fg bg bold hidden
		cursor.hidden = !(num & 0x0001);
		cursor.hanging = !!(num & 0x0002);
		// console.log("Attributes word ",num.toString(16)+'h');

		Input.setAlts(
			!!(num & 0x0004), // cu
			!!(num & 0x0008), // np
			!!(num & 0x0010) // fn
		);

		fg = 7;
		bg = 0;
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

		// if (!cursor.hidden || cursor.hanging || !cursor.suppress) {
		// 	// hide cursor asap
		// 	_draw(_curCell(), false);
		// }

		if (cursorMoved) {
			cursor.forceOn = true;
			cursorFlashStartIval = setTimeout(function() {
				cursor.forceOn = false;
			}, 1200);
			_draw(_curCell(), true);
		}
	}

	function _load_labels(str) {
		var pieces = str.split('\x01');
		qs('h1').textContent = pieces[0];
		qsa('#buttons button').forEach(function(x, i) {
			var s = pieces[i+1].trim();
			// if empty string, use the "dim" effect and put nbsp instead to stretch the btn vertically
			x.innerHTML = s.length > 0 ? e(s) : "&nbsp;";
			x.style.opacity = s.length > 0 ? 1 : 0.2;
		});
	}

	function _beep()
	{
		var osc, gain;
		if (!audioCtx) return;

		// Main beep
		osc = audioCtx.createOscillator();
		gain = audioCtx.createGain();
		osc.connect(gain);
		gain.connect(audioCtx.destination);
		gain.gain.value = 0.5;
		osc.frequency.value = 750;
		osc.type = 'sine';
		osc.start();
		osc.stop(audioCtx.currentTime+0.05);

		// Surrogate beep (making it sound like 'oops')
		osc = audioCtx.createOscillator();
		gain = audioCtx.createGain();
		osc.connect(gain);
		gain.connect(audioCtx.destination);
		gain.gain.value = 0.2;
		osc.frequency.value = 400;
		osc.type = 'sine';
		osc.start(audioCtx.currentTime+0.05);
		osc.stop(audioCtx.currentTime+0.08);
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
			case 'B':
				_beep();
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
	var heartbeatTout;

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
			// . = heartbeat
			if (evt.data != '.') {
				//console.log("RX: ", evt.data);
				// Assume all our messages are screen updates
				Screen.load(evt.data);
			}
			heartbeat();
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
		heartbeat();

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
			heartbeat();

			showPage();
		});
	}

	function heartbeat() {
		clearTimeout(heartbeatTout);
		heartbeatTout = setTimeout(heartbeatFail, 3000);
	}

	function heartbeatFail() {
		console.error("Heartbeat lost, reloading...");
		location.reload();
	}

	return {
		ws: null,
		init: init,
		send: doSend
	};
})();

/** User input */
var Input = (function() {
	var opts = {
		np_alt: false,
		cu_alt: false,
		fn_alt: false,
	};

	function sendStrMsg(str) {
		Conn.send("STR:"+str);
	}

	function sendBtnMsg(n) {
		Conn.send("BTN:"+n);
	}

	function fa(alt, normal) {
		return opts.fn_alt ? alt : normal;
	}

	function ca(alt, normal) {
		return opts.cu_alt ? alt : normal;
	}

	function na(alt, normal) {
		return opts.np_alt ? alt : normal;
	}

	function _bindFnKeys() {
		var keymap = {
			'tab': '\x09',
			'backspace': '\x08',
			'enter': '\x0d',
			'ctrl+enter': '\x0a',
			'esc': '\x1b',
			'up': ca('\x1bOA', '\x1b[A'),
			'down': ca('\x1bOB', '\x1b[B'),
			'right': ca('\x1bOC', '\x1b[C'),
			'left': ca('\x1bOD', '\x1b[D'),
			'home': ca('\x1bOH', fa('\x1b[H', '\x1b[1~')),
			'insert': '\x1b[2~',
			'delete': '\x1b[3~',
			'end': ca('\x1bOF', fa('\x1b[F', '\x1b[4~')),
			'pageup': '\x1b[5~',
			'pagedown': '\x1b[6~',
			'f1': fa('\x1bOP', '\x1b[11~'),
			'f2': fa('\x1bOQ', '\x1b[12~'),
			'f3': fa('\x1bOR', '\x1b[13~'),
			'f4': fa('\x1bOS', '\x1b[14~'),
			'f5': '\x1b[15~', // note the disconnect
			'f6': '\x1b[17~',
			'f7': '\x1b[18~',
			'f8': '\x1b[19~',
			'f9': '\x1b[20~',
			'f10': '\x1b[21~', // note the disconnect
			'f11': '\x1b[23~',
			'f12': '\x1b[24~',
			'shift+f1': fa('\x1bO1;2P', '\x1b[25~'),
			'shift+f2': fa('\x1bO1;2Q', '\x1b[26~'), // note the disconnect
			'shift+f3': fa('\x1bO1;2R', '\x1b[28~'),
			'shift+f4': fa('\x1bO1;2S', '\x1b[29~'), // note the disconnect
			'shift+f5': fa('\x1b[15;2~', '\x1b[31~'),
			'shift+f6': fa('\x1b[17;2~', '\x1b[32~'),
			'shift+f7': fa('\x1b[18;2~', '\x1b[33~'),
			'shift+f8': fa('\x1b[19;2~', '\x1b[34~'),
			'shift+f9': fa('\x1b[20;2~', '\x1b[35~'), // 35-38 are not standard - but what is?
			'shift+f10': fa('\x1b[21;2~', '\x1b[36~'),
			'shift+f11': fa('\x1b[22;2~', '\x1b[37~'),
			'shift+f12': fa('\x1b[23;2~', '\x1b[38~'),
			'np_0': na('\x1bOp', '0'),
			'np_1': na('\x1bOq', '1'),
			'np_2': na('\x1bOr', '2'),
			'np_3': na('\x1bOs', '3'),
			'np_4': na('\x1bOt', '4'),
			'np_5': na('\x1bOu', '5'),
			'np_6': na('\x1bOv', '6'),
			'np_7': na('\x1bOw', '7'),
			'np_8': na('\x1bOx', '8'),
			'np_9': na('\x1bOy', '9'),
			'np_mul': na('\x1bOR', '*'),
			'np_add': na('\x1bOl', '+'),
			'np_sub': na('\x1bOS', '-'),
			'np_point': na('\x1bOn', '.'),
			'np_div': na('\x1bOQ', '/'),
			// we don't implement numlock key (should change in numpad_alt mode, but it's even more useless than the rest)
		};

		for (var k in keymap) {
			if (keymap.hasOwnProperty(k)) {
				bind(k, keymap[k]);
			}
		}
	}

	function bind(combo, str) {
		// mac fix - allow also cmd
		if (combo.indexOf('ctrl+') !== -1) {
			combo += ',' + combo.replace('ctrl', 'command');
		}

		// unbind possible old binding
		key.unbind(combo);

		key(combo, function (e) {
			e.preventDefault();
			sendStrMsg(str)
		});
	}

	function _initKeys() {
		// This takes care of text characters typed
		window.addEventListener('keypress', function(evt) {
			var str = '';
			if (evt.key) str = evt.key;
			else if (evt.which) str = String.fromCodePoint(evt.which);
			if (str.length>0 && str.charCodeAt(0) >= 32) {
//				console.log("Typed ", str);
				sendStrMsg(str);
			}
		});

		// ctrl-letter codes are sent as simple low ASCII codes
		for (var i = 1; i<=26;i++) {
			bind('ctrl+' + String.fromCharCode(96+i), String.fromCharCode(i));
		}
		bind('ctrl+]', '\x1b'); // alternate way to enter ESC
		bind('ctrl+\\', '\x1c');
		bind('ctrl+[', '\x1d');
		bind('ctrl+^', '\x1e');
		bind('ctrl+_', '\x1f');

		_bindFnKeys();
	}

	var mb1 = 0;
	var mb2 = 0;
	var mb3 = 0;

	function init() {
		_initKeys();

		// Button presses
		qsa('#buttons button').forEach(function(s) {
			s.addEventListener('click', function() {
				sendBtnMsg(+this.dataset['n']);
			});
		});

		window.addEventListener('mousedown', function(evt) {
			if (evt.button == 0) mb1 = 1;
			if (evt.button == 1) mb2 = 1;
			if (evt.button == 2) mb3 = 1;
		});

		window.addEventListener('mouseup', function(evt) {
			if (evt.button == 0) mb1 = 0;
			if (evt.button == 1) mb2 = 0;
			if (evt.button == 2) mb3 = 0;
		});
	}

	function packModifiersForMouse() {
		return (key.isModifier('ctrl')?1:0) |
			(key.isModifier('shift')?2:0) |
			(key.isModifier('alt')?4:0) |
			(key.isModifier('meta')?8:0);
	}

	return {
		init: init,
		// onTap: sendPosMsg,
		sendString: sendStrMsg,
		setAlts: function(cu, np, fn) {
			if (opts.cu_alt != cu || opts.np_alt != np || opts.fn_alt != fn) {
				opts.cu_alt = cu;
				opts.np_alt = np;
				opts.fn_alt = fn;

				// rebind keys - codes have changed
				_bindFnKeys();
			}
		},
		onMouseMove: function(x, y) {
			var b = (mb1?1:0) | (mb2?2:0) | (mb3?4:0);
			var m = packModifiersForMouse();
			Conn.send("MM:"+y+','+x+','+b+','+m);
		},
		onMouseDown: function(x, y, b) {
			if(b>3) return;
			var m = packModifiersForMouse();
			Conn.send("MP:"+y+','+x+','+b+','+m);
		},
		onMouseUp: function(x, y, b) {
			if(b>3) return;
			var m = packModifiersForMouse();
			Conn.send("MR:"+y+','+x+','+b+','+m);
		},
		onMouseWheel: function(x, y, dir) {
			// -1 ... btn 4 (away from user)
			// +1 ... btn 5 (towards user)
			var m = packModifiersForMouse();
			Conn.send("MP:"+y+','+x+','+(dir<0?4:5)+','+m);
		},
	};
})();

window.termInit = function () {
	Conn.init();
	Input.init();
};
