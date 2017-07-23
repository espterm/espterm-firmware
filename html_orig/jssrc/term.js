(function() {
	/**
	 * Terminal module
	 */
	var Term = (function () {
		var W, H;
		var cursor = {a: false, x: 0, y: 0, suppress: false, hidden: false};
		var screen = [];
		var blinkIval;

/*
		/!** Clear screen *!/
		function cls() {
			screen.forEach(function(cell, i) {
				cell.t = ' ';
				cell.fg = 7;
				cell.bg = 0;
				blit(cell);
			});
		}
*/

		/** Set text and color at XY */
		function cellAt(y, x) {
			return screen[y*W+x];
		}

		/** Get cell under cursor */
		function cursorCell() {
			return cellAt(cursor.y, cursor.x);
		}

/*
		/!** Enable or disable cursor visibility *!/
		function cursorEnable(enable) {
			cursor.hidden = !enable;
			cursor.a &= enable;
			blit(cursorCell(), cursor.a);
		}

		/!** Safely move cursor *!/
		function cursorSet(y, x) {
			// Hide and prevent from showing up during the move
			cursor.suppress = true;
			blit(cursorCell(), false);

			cursor.x = x;
			cursor.y = y;

			// Show again
			cursor.suppress = false;
			blit(cursorCell(), cursor.a);
		}
*/

		/** Update cell on display. inv = invert (for cursor) */
		function blit(cell, inv) {
			var e = cell.e, fg, bg;
			// Colors
			fg = inv ? cell.bg : cell.fg;
			bg = inv ? cell.fg : cell.bg;
			// Update
			e.innerText = (cell.t+' ')[0];
			e.className = 'fg'+fg+' bg'+bg;
		}

		/** Show entire screen */
		function blitAll() {
			screen.forEach(function(cell, i) {
				/* Invert if under cursor & cursor active */
				var inv = cursor.a && (i == cursor.y*W+cursor.x);
				blit(cell, inv);
			});
		}

		/** Load screen content from a 'binary' sequence */
		function load(obj) {
			cursor.x = obj.x;
			cursor.y = obj.y;
			cursor.hidden = !obj.cv;

			// full re-init if size changed
			if (obj.w != W || obj.h != H) {
				Term.init(obj);
				return;
			}

			// Simple compression - hexFG hexBG 'ASCII' (r/s/t/u NUM{1,2,3,4})?
			// comma instead of both colors = same as before

			var i = 0, ci = 0, str = obj.screen;
			var fg = 7, bg = 0;
			while(i < str.length && ci<W*H) {
				var cell = screen[ci++];

				var j = str[i];
				if (j != ',') { // comma = repeat last colors
					fg = cell.fg = parseInt(str[i++], 16);
					bg = cell.bg = parseInt(str[i++], 16);
				} else {
					i++;
					cell.fg = fg;
					cell.bg = bg;
				}

				var t = cell.t = str[i++];

				var repchars = 0;
				switch(str[i]) {
					case 'r': repchars = 1; break;
					case 's': repchars = 2; break;
					case 't': repchars = 3; break;
					case 'u': repchars = 4; break;
					default: repchars = 0;
				}

				if (repchars > 0) {
					var rep = parseInt(str.substr(i+1,repchars));
					i = i + repchars + 1;
					for (; rep>0 && ci<W*H; rep--) {
						cell = screen[ci++];
						cell.fg = fg;
						cell.bg = bg;
						cell.t = t;
					}
				}
			}

			blitAll();
		}

		/** Init the terminal */
		function init(obj) {
			W = obj.w;
			H = obj.h;

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

				cell = {t: ' ', fg: 7, bg: 0, e: e};
				screen.push(cell);
				blit(cell);
			}

			/* Cursor blinking */
			clearInterval(blinkIval);
			blinkIval = setInterval(function() {
				cursor.a = !cursor.a;
				if (cursor.hidden) {
					cursor.a = false;
				}

				if (!cursor.suppress) {
					blit(cursorCell(), cursor.a);
				}
			}, 500);

			load(obj);
		}

		// publish
		return {
			init: init,
			load: load
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
			}, 1000);
		}

		function onMessage(evt) {
			try {
				console.log("RX: ", evt.data);
				// Assume all our messages are screen updates
				Term.load(JSON.parse(evt.data));
			} catch(e) {
				console.error(e);
			}
		}

		function doSend(message) {
			console.log("TX: ", message);
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
			ws = new WebSocket("ws://"+_root+"/ws/update.cgi");
			ws.onopen = onOpen;
			ws.onclose = onClose;
			ws.onmessage = onMessage;

			console.log("Opening socket.");
		}

		return {
			ws: null,
			init: init,
			send: doSend
		};
	})();

	//
	// Keyboard (& mouse) input
	//
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
			onTap: sendPosMsg
		};
	})();


	window.termInit = function (obj) {
		Term.init(obj);
		Conn.init();
		Input.init();
	};
})();
