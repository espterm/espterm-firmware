//region Libs / utils

/*
 * DOM selector
 *
 * Usage:
 *   $('div');
 *   $('#name');
 *   $('.name');
 *
 *
 * Copyright (C) 2011 Jed Schmidt <http://jed.is> - WTFPL
 * More: https://gist.github.com/991057
 *
 */

var $ = function(
  a,                         // take a simple selector like "name", "#name", or ".name", and
  b                          // an optional context, and
){
  a = a.match(/^(\W)?(.*)/); // split the selector into name and symbol.
  return(                    // return an element or list, from within the scope of
    b                        // the passed context
    || document              // or document,
  )[
    "getElement" + (         // obtained by the appropriate method calculated by
      a[1]
        ? a[1] == "#"
          ? "ById"           // the node by ID,
          : "sByClassName"   // the nodes by class name, or
        : "sByTagName"       // the nodes by tag name,
    )
  ](
    a[2]                     // called with the name.
  )
};

/*
 * Create DOM element
 *
 * Usage:
 *   var el = m('<h1>Hello</h1>');
 *   document.body.appendChild(el);
 *
 *
 *            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 *                    Version 2, December 2004
 *
 * Copyright (C) 2011 Jed Schmidt <http://jed.is> - WTFPL
 * More: https://gist.github.com/966233
 *
 */

var m = function(
  a, // an HTML string
  b, // placeholder
  c  // placeholder
){
  b = document;                   // get the document,
  c = b.createElement("p");       // create a container element,
  c.innerHTML = a;                // write the HTML to it, and
  a = b.createDocumentFragment(); // create a fragment.

  while (                         // while
    b = c.firstChild              // the container element has a first child
  ) a.appendChild(b);             // append the child to the fragment,

  return a                        // and then return the fragment.
};

//endregion


//
// Terminal class
//
(function () {
	function make(e) {return document.createElement(e)}

	var W = 26, H = 10; //26, 10
	var cursor = {a: false, x: 0, y: 0, suppress: false, hidden: false};
	var screen = [];

	/** Colors table */
	var CLR = [// dark gray #2E3436
		// 0 black, 1 red, 2 green, 3 yellow
		// 4 blue, 5 mag, 6 cyan, 7 white
		'#111213','#CC0000','#4E9A06','#C4A000',
		'#3465A4','#75507B','#06989A','#D3D7CF',
		// BRIGHT
		// 8 black, 9 red, 10 green, 11 yellow
		// 12 blue, 13 mag, 14 cyan, 15 white
		'#555753','#EF2929','#8AE234','#FCE94F',
		'#729FCF','#AD7FA8','#34E2E2','#EEEEEC'
	];

	/** Clear screen */
	function cls() {
		screen.forEach(function(cell, i) {
			cell.t = ' ';
			cell.fg = 7;
			cell.bg = 0;
			blit(cell);
		});
	}

	/** Set text and color at XY */
	function cellAt(y, x) {
		return screen[y*W+x];
	}

	/** Get cell under cursor */
	function cursorCell() {
		return cellAt(cursor.y, cursor.x);
	}

	/** Enable or disable cursor visibility */
	function cursorEnable(enable) {
		cursor.hidden = !enable;
		cursor.a &= enable;
		blit(cursorCell(), cursor.a);
	}

	/** Safely move cursor */
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

	/** Update cell on display. inv = invert (for cursor) */
	function blit(cell, inv) {
		var e = cell.e, fg, bg;
		// Colors
		fg = inv ? cell.bg : cell.fg;
		bg = inv ? cell.fg : cell.bg;
		// Update
		e.innerText = (cell.t+' ')[0];
		e.style.color = colorHex(fg);
		e.style.backgroundColor = colorHex(bg);
		e.style.fontWeight = fg > 7 ? 'bold' : 'normal';
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
	function load(seq) {
		if (seq.length != W*H*3) throw "Bad data format.";

		// primitive format with 3 chars per cell: letter, fg [hex], bg [hex]
		for (var i = 0; i < W * H; i++) {
			var cell = screen[i];
			cell.t = seq[i*3];
			cell.fg = parseInt(seq[i*3+1], 16);
			cell.bg = parseInt(seq[i*3+2], 16);
		}

		blitAll();
	}

	/** Parse color */
	function colorHex(c) {
		var c = parseInt(c);
		if (c < 0 || c > 15) c = 0;
		return CLR[c];
	}

	/** Init the terminal */
	function init() {
		/* Build screen & show */
		var e, scr = $('#screen');
		for(var i = 0; i < W*H; i++) {
			e = make('span');

			/* End of line */
			if ((i > 0) && (i % W == 0)) {
				scr.appendChild(make('br'));
			}
			/* The cell */
			scr.appendChild(e);

			var cell = {t: ' ', fg: 7, bg: 0, e: e};
			screen.push(cell);
			blit(cell);
		}

		/* Cursor blinking */
		setInterval(function() {
			cursor.a = !cursor.a;
			if (cursor.hidden) {
				cursor.a = false;
			}

			if (!cursor.suppress) {
				blit(cursorCell(), cursor.a);
			}
		}, 500);
	}

	// publish
	window.Term = {
		init: init,
		load: load,
		setCursor: cursorSet,
		enableCursor: cursorEnable,
		clear: cls,
	};
})();

//
// Connection class
//
(function() {
	var wsUri = "ws://"+window.location.host+"/ws/update.cgi";
	var ws;

	function onOpen(evt) {
		console.log("CONNECTED");
	}

	function onClose(evt) {
		console.error("SOCKET CLOSED");
	}

	function onMessage(evt) {
		console.log("Message received!", evt.data);
		// TODO process
	}

	function onError(evt) {
		console.error(evt.data);
	}

	function doSend(message) {
		ws.send(message);
	}
		
	function init() {
		ws = new WebSocket(wsUri);
		ws.onopen = onOpen;
		ws.onclose = onClose
		ws.onmessage = onMessage;
		ws.onerror = onError;
		
		console.log("Opening socket.");
	}
	
	window.Conn = {
		ws: null,
		init: init,
	};
})();

function init() {
	Term.init();
	Conn.init();
}
