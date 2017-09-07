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
		forceOn: false,  // force on unless hanging: used to keep cursor visible during move
		hidden: false,    // do not show (DEC opt)
		hanging: false,   // cursor at column "W+1" - not visible
	};

	var screen = [];
	var blinkIval;
	var cursorFlashStartIval;

	// Some non-bold Fraktur symbols are outside the contiguous block
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
		console.error("No AudioContext!", er);
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

	function alpha2fraktur(t) {
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
		return t;
	}

	/** Update cell on display. inv = invert (for cursor) */
	function _draw(cell, inv) {
		if (!cell) return;
		if (typeof inv == 'undefined') {
			inv = cursor.a && cursor.x == cell.x && cursor.y == cell.y;
		}

		var fg, bg, cn, t;

		fg = inv ? cell.bg : cell.fg;
		bg = inv ? cell.fg : cell.bg;

		t = cell.t;
		if (!t.length) t = ' ';

		cn = 'fg' + fg + ' bg' + bg;
		if (cell.attrs & (1<<0)) cn += ' bold';
		if (cell.attrs & (1<<1)) cn += ' faint';
		if (cell.attrs & (1<<2)) cn += ' italic';
		if (cell.attrs & (1<<3)) cn += ' under';
		if (cell.attrs & (1<<4)) cn += ' blink';
		if (cell.attrs & (1<<5)) {
			cn += ' fraktur';
			t = alpha2fraktur(t);
		}
		if (cell.attrs & (1<<6)) cn += ' strike';

		cell.slot.textContent = t;
		cell.elem.className = cn;
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
		var cOuter, cInner, cell, screenDiv = qs('#screen');

		// Empty the screen node
		while (screenDiv.firstChild) screenDiv.removeChild(screenDiv.firstChild);

		screen = [];

		for(var i = 0; i < W*H; i++) {
			cOuter = mk('span');
			cInner = mk('span');

			/* Mouse tracking */
			(function() {
				var x = i % W;
				var y = Math.floor(i / W);
				cOuter.addEventListener('mouseenter', function (evt) {
					Input.onMouseMove(x, y);
				});
				cOuter.addEventListener('mousedown', function (evt) {
					Input.onMouseDown(x, y, evt.button+1);
				});
				cOuter.addEventListener('mouseup', function (evt) {
					Input.onMouseUp(x, y, evt.button+1);
				});
				cOuter.addEventListener('contextmenu', function (evt) {
					if (Input.mouseTracksClicks()) {
						evt.preventDefault();
					}
				});
				cOuter.addEventListener('mousewheel', function (evt) {
					Input.onMouseWheel(x, y, evt.deltaY>0?1:-1);
					return false;
				});
			})();

			/* End of line */
			if ((i > 0) && (i % W == 0)) {
				screenDiv.appendChild(mk('br'));
			}
			/* The cell */
			cOuter.appendChild(cInner);
			screenDiv.appendChild(cOuter);

			cell = {
				t: ' ',
				fg: 7,
				bg: 0, // the colors will be replaced immediately as we receive data (user won't see this)
				attrs: 0,
				elem: cOuter,
				slot: cInner,
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
			if (cursor.hidden || cursor.hanging) {
				cursor.a = false;
			}

			if (!cursor.suppress) {
				_draw(_curCell(), cursor.forceOn || cursor.a);
			}
		}, 500);

		/* blink attribute animation */
		setInterval(function () {
			$('#screen').removeClass('blink-hide');
			setTimeout(function () {
				$('#screen').addClass('blink-hide');
			}, 800); // 200 ms ON
		}, 1000);

		inited = true;
	}

	// constants for decoding the update blob
	var SEQ_SET_COLOR_ATTR = 1;
	var SEQ_REPEAT = 2;
	var SEQ_SET_COLOR = 3;
	var SEQ_SET_ATTR = 4;

	/** Parse received screen update object (leading S removed already) */
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
		num = parse2B(str, i); i += 2; // fg bg attribs
		cursor.hidden = !(num & (1<<0)); // DEC opt "visible"
		cursor.hanging = !!(num & (1<<1));
		// console.log("Attributes word ",num.toString(16)+'h');

		Input.setAlts(
			!!(num & (1<<2)), // cursors alt
			!!(num & (1<<3)), // numpad alt
			!!(num & (1<<4)) // fn keys alt
		);

		var mt_click = !!(num & (1<<5));
		var mt_move = !!(num & (1<<6));
		Input.setMouseMode(
			mt_click,
			mt_move
		);
		$('#screen').toggleClass('noselect', mt_move);

		var show_buttons = !!(num & (1<<7));
		var show_config_links = !!(num & (1<<8));
		$('.x-term-conf-btn').toggleClass('hidden', !show_config_links);
		$('#action-buttons').toggleClass('hidden', !show_buttons);

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

	/** Apply labels to buttons and screen title (leading T removed already) */
	function _load_labels(str) {
		var pieces = str.split('\x01');
		qs('h1').textContent = pieces[0];
		$('#action-buttons button').forEach(function(x, i) {
			var s = pieces[i+1].trim();
			// if empty string, use the "dim" effect and put nbsp instead to stretch the btn vertically
			x.innerHTML = s.length > 0 ? e(s) : "&nbsp;";
			x.style.opacity = s.length > 0 ? 1 : 0.2;
		})
	}

	/** Audible beep for ASCII 7 */
	function _beep() {
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
		//console.log(JSON.stringify(str));
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
				console.log(str);
		}
	}

	return  {
		load: load, // full load (string)
	};
})();
