/** Global generic init */
$.ready(function () {
	// Checkbox UI (checkbox CSS and hidden input with int value)
	$('.Row.checkbox').forEach(function(x) {
		var inp = x.querySelector('input');
		var box = x.querySelector('.box');

		$(box).toggleClass('checked', inp.value);

		var hdl = function() {
			inp.value = 1 - inp.value;
			$(box).toggleClass('checked', inp.value)
		};

		$(x).on('click', hdl).on('keypress', cr(hdl));
	});

	// Expanding boxes on mobile
	$('.Box.mobcol,.Box.fold').forEach(function(x) {
		var h = x.querySelector('h2');

		var hdl = function() {
			$(x).toggleClass('expanded');
		};
		$(h).on('click', hdl).on('keypress', cr(hdl));
	});

	$('form').forEach(function(x) {
		$(x).on('keypress', function(e) {
			if ((e.keyCode == 10 || e.keyCode == 13) && e.ctrlKey) {
				x.submit();
			}
		})
	});

	// loader dots...
	setInterval(function () {
		$('.anim-dots').each(function (x) {
			var $x = $(x);
			var dots = $x.html() + '.';
			if (dots.length == 5) dots = '.';
			$x.html(dots);
		});
	}, 1000);

	// flipping number boxes with the mouse wheel
	$('input[type=number]').on('mousewheel', function(e) {
		var $this = $(this);
		var val = +$this.val();
		if (isNaN(val)) val = 1;

		var step = +($this.attr('step') || 1);
		var min = +$this.attr('min');
		var max = +$this.attr('max');
		if(e.wheelDelta > 0) {
			val += step;
		} else {
			val -= step;
		}

		if (typeof min != 'undefined') val = Math.max(val, +min);
		if (typeof max != 'undefined') val = Math.min(val, +max);
		$this.val(val);

		if ("createEvent" in document) {
			var evt = document.createEvent("HTMLEvents");
			evt.initEvent("change", false, true);
			$this[0].dispatchEvent(evt);
		} else {
			$this[0].fireEvent("onchange");
		}

		e.preventDefault();
	});

	var errAt = location.search.indexOf('err=');
	if (errAt !== -1 && qs('.Box.errors')) {
		var errs = location.search.substr(errAt+4).split(',');
		var hres = [];
		errs.forEach(function(er) {
			var lbl = qs('label[for="'+er+'"]');
			if (lbl) {
				lbl.classList.add('error');
				hres.push(lbl.childNodes[0].textContent.trim().replace(/: ?$/, ''));
			} else {
				hres.push(er);
			}
		});

		qs('.Box.errors .list').innerHTML = hres.join(', ');
		qs('.Box.errors').classList.remove('hidden');
	}

	Modal.init();
	Notify.init();

	// remove tabindixes from h2 if wide
	if (window.innerWidth > 550) {
		$('.Box h2').forEach(function (x) {
			x.removeAttribute('tabindex');
		});

		// brand works as a link back to term in widescreen mode
		var br = qs('#brand');
		br && br.addEventListener('click', function() {
			location.href='/'; // go to terminal
		});
	}
});

$._loader = function(vis) {
	$('#loader').toggleClass('show', vis);
};

function showPage() {
	$('#content').addClass('load');
}

$.ready(function() {
	if (window.noAutoShow !== true) {
		setTimeout(function () {
			showPage();
		}, 1);
	}
});


/*! http://mths.be/fromcodepoint v0.1.0 by @mathias */
if (!String.fromCodePoint) {
	(function() {
		var defineProperty = (function() {
			// IE 8 only supports `Object.defineProperty` on DOM elements
			try {
				var object = {};
				var $defineProperty = Object.defineProperty;
				var result = $defineProperty(object, object, object) && $defineProperty;
			} catch(error) {}
			return result;
		}());
		var stringFromCharCode = String.fromCharCode;
		var floor = Math.floor;
		var fromCodePoint = function() {
			var MAX_SIZE = 0x4000;
			var codeUnits = [];
			var highSurrogate;
			var lowSurrogate;
			var index = -1;
			var length = arguments.length;
			if (!length) {
				return '';
			}
			var result = '';
			while (++index < length) {
				var codePoint = Number(arguments[index]);
				if (
					!isFinite(codePoint) ||       // `NaN`, `+Infinity`, or `-Infinity`
					codePoint < 0 ||              // not a valid Unicode code point
					codePoint > 0x10FFFF ||       // not a valid Unicode code point
					floor(codePoint) != codePoint // not an integer
				) {
					throw RangeError('Invalid code point: ' + codePoint);
				}
				if (codePoint <= 0xFFFF) { // BMP code point
					codeUnits.push(codePoint);
				} else { // Astral code point; split in surrogate halves
					// http://mathiasbynens.be/notes/javascript-encoding#surrogate-formulae
					codePoint -= 0x10000;
					highSurrogate = (codePoint >> 10) + 0xD800;
					lowSurrogate = (codePoint % 0x400) + 0xDC00;
					codeUnits.push(highSurrogate, lowSurrogate);
				}
				if (index + 1 == length || codeUnits.length > MAX_SIZE) {
					result += stringFromCharCode.apply(null, codeUnits);
					codeUnits.length = 0;
				}
			}
			return result;
		};
		if (defineProperty) {
			defineProperty(String, 'fromCodePoint', {
				'value': fromCodePoint,
				'configurable': true,
				'writable': true
			});
		} else {
			String.fromCodePoint = fromCodePoint;
		}
	}());
}
