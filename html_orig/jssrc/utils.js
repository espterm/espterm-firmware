/** Make a node */
function mk(e) {return document.createElement(e)}

/** Find one by query */
function qq(s) {return document.querySelector(s)}

/** Find all by query */
function qa(s) {return document.querySelectorAll(s)}

/** Convert any to bool safely */
function bool(x) {
	return (x === 1 || x === '1' || x === true || x === 'true');
}

function intval(x) {
	return parseInt(x);
}

/** Extend an objects with options */
function extend(defaults, options) {
	var target = {};

	Object.keys(defaults).forEach(function(k){
		target[k] = defaults[k];
	});

	Object.keys(options).forEach(function(k){
		target[k] = options[k];
	});

	return target;
}

/** Escape string for use as literal in RegExp */
function rgxe(str) {
	return str.replace(/[\-\[\]\/\{\}\(\)\*\+\?\.\\\^\$\|]/g, "\\$&");
}

function numfmt(x, places) {
	var pow = Math.pow(10, places);
	return Math.round(x*pow) / pow;
}

function estimateLoadTime(fs, n) {
	return (1000/fs)*n+1500;
}

function msNow() {
	return +(new Date);
}

function msElapsed(start) {
	return msNow() - start;
}

Math.log10 = Math.log10 || function(x) {
	return Math.log(x) / Math.LN10;
};

/**
 * Perform a substitution in the given string.
 *
 * Arguments - array or list of replacements.
 * Arguments numeric keys will replace {0}, {1} etc.
 * Named keys also work, ie. {foo: "bar"} -> replaces {foo} with bar.
 *
 * Braces are added to keys if missing.
 *
 * @returns {String} result
 */
String.prototype.format = function () {
	var out = this;
	var repl = arguments;

	if (arguments.length == 1 && (Array.isArray(arguments[0]) || typeof arguments[0] == 'object')) {
		repl = arguments[0];
	}

	for (var ph in repl) {
		if (repl.hasOwnProperty(ph)) {
			var ph_orig = ph;

			if (!ph.match(/^\{.*\}$/)) {
				ph = '{' + ph + '}';
			}

			// replace all occurrences
			var pattern = new RegExp(rgxe(ph), "g");
			out = out.replace(pattern, repl[ph_orig]);
		}
	}

	return out;
};

function e(str) {
	return String(str)
		.replace(/&/g, '&amp;')
		.replace(/"/g, '&quot;')
		.replace(/'/g, '&#39;')
		.replace(/</g, '&lt;')
		.replace(/>/g, '&gt;');
}

function undef(x) {
	return typeof x == 'undefined';
}
