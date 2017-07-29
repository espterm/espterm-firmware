/*!chibi 3.0.7, Copyright 2012-2016 Kyle Barrow, released under MIT license */

// MODIFIED VERSION.
(function () {
	'use strict';

	var readyfn = [],
		loadedfn = [],
		domready = false,
		pageloaded = false,
		d = document,
		w = window;

	// Fire any function calls on ready event
	function fireReady() {
		var i;
		domready = true;
		for (i = 0; i < readyfn.length; i += 1) {
			readyfn[i]();
		}
		readyfn = [];
	}

	// Fire any function calls on loaded event
	function fireLoaded() {
		var i;
		pageloaded = true;
		// For browsers with no DOM loaded support
		if (!domready) {
			fireReady();
		}
		for (i = 0; i < loadedfn.length; i += 1) {
			loadedfn[i]();
		}
		loadedfn = [];
	}

	// Check DOM ready, page loaded
	if (d.addEventListener) {
		// Standards
		d.addEventListener('DOMContentLoaded', fireReady, false);
		w.addEventListener('load', fireLoaded, false);
	} else if (d.attachEvent) {
		// IE
		d.attachEvent('onreadystatechange', fireReady);
		// IE < 9
		w.attachEvent('onload', fireLoaded);
	} else {
		// Anything else
		w.onload = fireLoaded;
	}

	// Utility functions

	// Loop through node array
	function nodeLoop(fn, nodes) {
		var i;
		// Good idea to walk up the DOM
		for (i = nodes.length - 1; i >= 0; i -= 1) {
			fn(nodes[i]);
		}
	}

	// Convert to camel case
	function cssCamel(property) {
		return property.replace(/-\w/g, function (result) {
			return result.charAt(1).toUpperCase();
		});
	}

	// Get computed style
	function computeStyle(elm, property) {
		// IE, everything else or null
		return (elm.currentStyle) ? elm.currentStyle[cssCamel(property)] : (w.getComputedStyle) ? w.getComputedStyle(elm, null).getPropertyValue(property) : null;

	}

	// Returns URI encoded query string pair
	function queryPair(name, value) {
		return encodeURIComponent(name).replace(/%20/g, '+') + '=' + encodeURIComponent(value).replace(/%20/g, '+');
	}

	// Set CSS, important to wrap in try to prevent error thown on unsupported property
	function setCss(elm, property, value) {
		try {
			elm.style[cssCamel(property)] = value;
		} catch (e) {
		}
	}

	// Show CSS
	function showCss(elm) {
		elm.style.display = '';
		// For elements still hidden by style block
		if (computeStyle(elm, 'display') === 'none') {
			elm.style.display = 'block';
		}
	}

	// Serialize form & JSON values
	function serializeData(nodes) {
		var querystring = '', subelm, i, j;
		if (nodes.constructor === Object) { // Serialize JSON data
			for (subelm in nodes) {
				if (nodes.hasOwnProperty(subelm)) {
					if (nodes[subelm].constructor === Array) {
						for (i = 0; i < nodes[subelm].length; i += 1) {
							querystring += '&' + queryPair(subelm, nodes[subelm][i]);
						}
					} else {
						querystring += '&' + queryPair(subelm, nodes[subelm]);
					}
				}
			}
		} else { // Serialize node data
			nodeLoop(function (elm) {
				if (elm.nodeName === 'FORM') {
					for (i = 0; i < elm.elements.length; i += 1) {
						subelm = elm.elements[i];

						if (!subelm.disabled) {
							switch (subelm.type) {
								// Ignore buttons, unsupported XHR 1 form fields
								case 'button':
								case 'image':
								case 'file':
								case 'submit':
								case 'reset':
									break;

								case 'select-one':
									if (subelm.length > 0) {
										querystring += '&' + queryPair(subelm.name, subelm.value);
									}
									break;

								case 'select-multiple':
									for (j = 0; j < subelm.length; j += 1) {
										if (subelm[j].selected) {
											querystring += '&' + queryPair(subelm.name, subelm[j].value);
										}
									}
									break;

								case 'checkbox':
								case 'radio':
									if (subelm.checked) {
										querystring += '&' + queryPair(subelm.name, subelm.value);
									}
									break;

								// Everything else including shinny new HTML5 input types
								default:
									querystring += '&' + queryPair(subelm.name, subelm.value);
							}
						}
					}
				}
			}, nodes);
		}
		// Tidy up first &
		return (querystring.length > 0) ? querystring.substring(1) : '';
	}

	// Class helper
	function classHelper(classes, action, nodes) {
		var classarray, search, i, replace, has = false;
		if (classes) {
			// Trim any whitespace
			classarray = classes.split(/\s+/);
			nodeLoop(function (elm) {
				for (i = 0; i < classarray.length; i += 1) {
					var clz = classarray[i];
					if (action === 'remove') {
						elm.classList.remove(clz);
					}
					else if (action === 'add') {
						elm.classList.add(clz);
					}
					else if (action === 'toggle') {
						elm.classList.toggle(clz);
					}
					else if (action === 'has') {
						if (elm.classList.contains(clz)) {
							has = true;
							break;
						}
					}

					// search = new RegExp('\\b' + classarray[i] + '\\b', 'g');
					// replace = new RegExp(' *' + classarray[i] + '\\b', 'g');
					// if (action === 'remove') {
					// 	elm.className = elm.className.replace(search, '');
					// } else if (action === 'toggle') {
					// 	elm.className = (elm.className.match(search)) ? elm.className.replace(replace, '') : elm.className + ' ' + classarray[i];
					// } else if (action === 'has') {
					// 	if (elm.className.match(search)) {
					// 		has = true;
					// 		break;
					// 	}
					// }
				}
			}, nodes);
		}
		return has;
	}

	// HTML insertion helper
	function insertHtml(value, position, nodes) {
		var tmpnodes, tmpnode;
		if (value) {
			nodeLoop(function (elm) {
				// No insertAdjacentHTML support for FF < 8 and IE doesn't allow insertAdjacentHTML table manipulation, so use this instead
				// Convert string to node. We can't innerHTML on a document fragment
				tmpnodes = d.createElement('div');
				tmpnodes.innerHTML = value;
				while ((tmpnode = tmpnodes.lastChild) !== null) {
					// Catch error in unlikely case elm has been removed
					try {
						if (position === 'before') {
							elm.parentNode.insertBefore(tmpnode, elm);
						} else if (position === 'after') {
							elm.parentNode.insertBefore(tmpnode, elm.nextSibling);
						} else if (position === 'append') {
							elm.appendChild(tmpnode);
						} else if (position === 'prepend') {
							elm.insertBefore(tmpnode, elm.firstChild);
						}
					} catch (e) {
						break;
					}
				}
			}, nodes);
		}
	}

	// Get nodes and return chibi
	function chibi(selector) {
		var cb, nodes = [], json = false, nodelist, i;

		if (selector) {

			// Element node, would prefer to use (selector instanceof HTMLElement) but no IE support
			if (selector.nodeType && selector.nodeType === 1) {
				nodes = [selector]; // return element as node list
			} else if (typeof selector === 'object') {
				// JSON, document object or node list, would prefer to use (selector instanceof NodeList) but no IE support
				json = (typeof selector.length !== 'number');
				nodes = selector;
			} else if (typeof selector === 'string') {
				nodelist = d.querySelectorAll(selector);

				// Convert node list to array so results have full access to array methods
				// Array.prototype.slice.call not supported in IE < 9 and often slower than loop anyway
				for (i = 0; i < nodelist.length; i += 1) {
					nodes[i] = nodelist[i];
				}
			}
		}

		// Only attach nodes if not JSON
		cb = json ? {} : nodes;

		// Public functions

		// Executes a function on nodes
		cb.each = function (fn) {
			if (typeof fn === 'function') {
				nodeLoop(function (elm) {
					// <= IE 8 loses scope so need to apply
					return fn.apply(elm, arguments);
				}, nodes);
			}
			return cb;
		};
		// Find first
		cb.first = function () {
			return chibi(nodes.shift());
		};
		// Find last
		cb.last = function () {
			return chibi(nodes.pop());
		};
		// Find odd
		cb.odd = function () {
			var odds = [], i;
			for (i = 0; i < nodes.length; i += 2) {
				odds.push(nodes[i]);
			}
			return chibi(odds);
		};
		// Find even
		cb.even = function () {
			var evens = [], i;
			for (i = 1; i < nodes.length; i += 2) {
				evens.push(nodes[i]);
			}
			return chibi(evens);
		};
		// Hide node
		cb.hide = function () {
			nodeLoop(function (elm) {
				elm.style.display = 'none';
			}, nodes);
			return cb;
		};
		// Show node
		cb.show = function () {
			nodeLoop(function (elm) {
				showCss(elm);
			}, nodes);
			return cb;
		};
		// Toggle node display
		cb.toggle = function (state) {
			if (typeof state != 'undefined') { // ADDED
				if (state)
					cb.show();
				else
					cb.hide();
			} else {
				nodeLoop(function (elm) {
					// computeStyle instead of style.display == 'none' catches elements that are hidden via style block
					if (computeStyle(elm, 'display') === 'none') {
						showCss(elm);
					} else {
						elm.style.display = 'none';
					}

				}, nodes);
			}
			return cb;
		};
		// Remove node
		cb.remove = function () {
			nodeLoop(function (elm) {
				// Catch error in unlikely case elm has been removed
				try {
					elm.parentNode.removeChild(elm);
				} catch (e) {
				}
			}, nodes);
			return chibi();
		};
		// Get/Set CSS
		cb.css = function (property, value) {
			if (property) {
				if (value || value === '') {
					nodeLoop(function (elm) {
						setCss(elm, property, value);
					}, nodes);
					return cb;
				}
				if (nodes[0]) {
					if (nodes[0].style[cssCamel(property)]) {
						return nodes[0].style[cssCamel(property)];
					}
					if (computeStyle(nodes[0], property)) {
						return computeStyle(nodes[0], property);
					}
				}
			}
		};
		// Get class(es)
		cb.getClass = function () {
			if (nodes[0] && nodes[0].className.length > 0) {
				// Weak IE trim support
				return nodes[0].className.replace(/^[\s\uFEFF\xA0]+|[\s\uFEFF\xA0]+$/g, '').replace(/\s+/, ' ');
			}
		};
		// Set (replaces) classes
		cb.setClass = function (classes) {
			if (classes || classes === '') {
				nodeLoop(function (elm) {
					elm.className = classes;
				}, nodes);
			}
			return cb;
		};
		// Add class
		cb.addClass = function (classes) {
			classHelper(classes, 'add', nodes);
			// if (classes) {
			// 	nodeLoop(function (elm) {
			// 		elm.className += ' ' + classes;
			// 	}, nodes);
			// }
			return cb;
		};
		// Remove class
		cb.removeClass = function (classes) {
			classHelper(classes, 'remove', nodes);
			return cb;
		};
		// Toggle class
		cb.toggleClass = function (classes, set) {
			var method = ((typeof set === 'undefined') ? 'toggle' : (+set ? 'add' : 'remove'));
			classHelper(classes, method, nodes);
			return cb;
		};
		// Has class
		cb.hasClass = function (classes) {
			return classHelper(classes, 'has', nodes);
		};
		// Get/set HTML
		cb.html = function (value) {
			if (value || value === '') {
				nodeLoop(function (elm) {
					elm.innerHTML = value;
				}, nodes);
				return cb;
			}
			if (nodes[0]) {
				return nodes[0].innerHTML;
			}
		};
		// Insert HTML before selector
		cb.htmlBefore = function (value) {
			insertHtml(value, 'before', nodes);
			return cb;
		};
		// Insert HTML after selector
		cb.htmlAfter = function (value) {
			insertHtml(value, 'after', nodes);
			return cb;
		};
		// Insert HTML after selector innerHTML
		cb.htmlAppend = function (value) {
			insertHtml(value, 'append', nodes);
			return cb;
		};
		// Insert HTML before selector innerHTML
		cb.htmlPrepend = function (value) {
			insertHtml(value, 'prepend', nodes);
			return cb;
		};
		// Get/Set HTML attributes
		cb.attr = function (property, value) {
			if (property) {
				property = property.toLowerCase();
				// IE < 9 doesn't allow style or class via get/setAttribute so switch. cssText returns prettier CSS anyway
				if (typeof value !== 'undefined') {//FIXED BUG HERE
					nodeLoop(function (elm) {
						if (property === 'style') {
							elm.style.cssText = value;
						} else if (property === 'class') {
							elm.className = value;
						} else {
							elm.setAttribute(property, value);
						}
					}, nodes);
					return cb;
				}
				if (nodes[0]) {
					if (property === 'style') {
						if (nodes[0].style.cssText) {
							return nodes[0].style.cssText;
						}
					} else if (property === 'class') {
						if (nodes[0].className) {
							return nodes[0].className;
						}
					} else {
						if (nodes[0].getAttribute(property)) {
							return nodes[0].getAttribute(property);
						}
					}
				}
			}
		};
		// Get/Set HTML data property
		cb.data = function (key, value) {
			if (key) {
				return cb.attr('data-' + key, value);
			}
		};
		// Get/Set form element values
		cb.val = function (value) {
			var values, i, j;
			if (typeof value != 'undefined') { // FIXED A BUG HERE
				nodeLoop(function (elm) {
					switch (elm.nodeName) {
						case 'SELECT':
							if (typeof value === 'string' || typeof value === 'number') {
								value = [value];
							}
							for (i = 0; i < elm.length; i += 1) {
								// Multiple select
								for (j = 0; j < value.length; j += 1) {
									elm[i].selected = '';
									if (elm[i].value === ""+value[j]) {
										elm[i].selected = 'selected';
										break;
									}
								}
							}
							break;
						case 'INPUT':
						case 'TEXTAREA':
						case 'BUTTON':
							elm.value = value;
							break;
					}
				}, nodes);

				return cb;
			}
			if (nodes[0]) {
				switch (nodes[0].nodeName) {
					case 'SELECT':
						values = [];
						for (i = 0; i < nodes[0].length; i += 1) {
							if (nodes[0][i].selected) {
								values.push(nodes[0][i].value);
							}
						}
						return (values.length > 1) ? values : values[0];
					case 'INPUT':
					case 'TEXTAREA':
					case 'BUTTON':
						return nodes[0].value;
				}
			}
		};
		// Return matching checked checkbox or radios
		cb.checked = function (check) {
			if (typeof check === 'boolean') {
				nodeLoop(function (elm) {
					if (elm.nodeName === 'INPUT' && (elm.type === 'checkbox' || elm.type === 'radio')) {
						elm.checked = check;
					}
				}, nodes);
				return cb;
			}
			if (nodes[0] && nodes[0].nodeName === 'INPUT' && (nodes[0].type === 'checkbox' || nodes[0].type === 'radio')) {
				return (!!nodes[0].checked);
			}
		};
		// Add event handler
		cb.on = function (event, fn) {
			if (selector === w || selector === d) {
				nodes = [selector];
			}
			nodeLoop(function (elm) {
				if (d.addEventListener) {
					elm.addEventListener(event, fn, false);
				} else if (d.attachEvent) {
					// <= IE 8 loses scope so need to apply, we add this to object so we can detach later (can't detach anonymous functions)
					elm[event + fn] = function () {
						return fn.apply(elm, arguments);
					};
					elm.attachEvent('on' + event, elm[event + fn]);
				}
			}, nodes);
			return cb;
		};
		// Remove event handler
		cb.off = function (event, fn) {
			if (selector === w || selector === d) {
				nodes = [selector];
			}
			nodeLoop(function (elm) {
				if (d.addEventListener) {
					elm.removeEventListener(event, fn, false);
				} else if (d.attachEvent) {
					elm.detachEvent('on' + event, elm[event + fn]);
					// Tidy up
					elm[event + fn] = null;
				}
			}, nodes);
			return cb;
		};
		return cb;
	}

	// Basic XHR
	chibi.ajax = function (options) { // if options is a number, it's timeout in ms
		var opts = extend({
			method: 'GET',
			nocache: true,
			timeout: 5000,
			loader: true,
			callback: null
		}, options);
		opts.method = opts.method.toUpperCase();

		var loaderFn = opts.loader ? chibi._loader : function(){};
		var url = opts.url;
		var method = opts.method;
		var query = null;

		if (opts.data) {
			query = serializeData(opts.data);
		}

		if (query && (method === 'GET')) {
			url += (url.indexOf('?') === -1) ? '?' + query : '&' + query;
			query = null;
		}

		var xhr = new XMLHttpRequest();

		// prevent caching
		if (opts.nocache) {
			var ts = (+(new Date())).toString(36);
			url += ((url.indexOf('?') === -1) ? '?' : '&') + '_=' + ts;
		}

		loaderFn(true);

		xhr.open(method, url, true);
		xhr.timeout = opts.timeout;

		// Abort after given timeout
		var abortTmeo = setTimeout(function () {
			console.error("XHR timed out.");
			xhr.abort();
			loaderFn(false);
		}, opts.timeout + 10);

		xhr.onreadystatechange = function () {
			if (xhr.readyState === 4) {
				loaderFn(false);

				opts.callback && opts.callback(xhr.responseText, xhr.status);

				clearTimeout(abortTmeo);
			}
		};

		xhr.setRequestHeader('X-Requested-With', 'XMLHttpRequest');
		if (method === 'POST') {
			xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
		}

		xhr.send(query);
		return xhr;
	};

	chibi._loader = function(){};

	// Alias to cb.ajax(url, 'get', callback)
	chibi.get = function (url, callback, opts) {
		opts = opts || {};
		opts.url = url;
		opts.callback = callback;
		opts.method = 'GET';
		return chibi.ajax(opts);
	};

	// Alias to cb.ajax(url, 'post', callback)
	chibi.post = function (url, callback, opts) {
		opts = opts || {};
		opts.url = url;
		opts.callback = callback;
		opts.method = 'POST';
		return chibi.ajax(opts);
	};

	// Fire on DOM ready
	chibi.ready = function (fn) {
		if (fn) {
			if (domready) {
				fn();
				return chibi;
			} else {
				readyfn.push(fn);
			}
		}
	};

	// Fire on page loaded
	chibi.loaded = function (fn) {
		if (fn) {
			if (pageloaded) {
				fn();
				return chibi;
			} else {
				loadedfn.push(fn);
			}
		}
	};

	var entityMap = {
		'&': '&amp;',
		'<': '&lt;',
		'>': '&gt;',
		'"': '&quot;',
		"'": '&#39;',
		'/': '&#x2F;',
		'`': '&#x60;',
		'=': '&#x3D;'
	};

	chibi.htmlEscape = function(string) {
		return String(string).replace(/[&<>"'`=\/]/g, function (s) {
			return entityMap[s];
		});
	};

	// Set Chibi's global namespace here ($)
	w.$ = chibi;
}());
/** Make a node */
function mk(e) {return document.createElement(e)}

/** Find one by query */
function qs(s) {return document.querySelector(s)}

/** Find all by query */
function qsa(s) {return document.querySelectorAll(s)}

/** Convert any to bool safely */
function bool(x) {
	return (x === 1 || x === '1' || x === true || x === 'true');
}

/**
 * Filter 'spacebar' and 'return' from keypress handler,
 * and when they're pressed, fire the callback.
 * use $(...).on('keypress', cr(handler))
 */
function cr(hdl) {
	return function(e) {
		if (e.which == 10 || e.which == 13 || e.which == 32) {
			hdl();
		}
	};
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

/** Format number to N decimal places, output as string */
function numfmt(x, places) {
	var pow = Math.pow(10, places);
	return Math.round(x*pow) / pow;
}

/** Get millisecond timestamp */
function msNow() {
	return +(new Date);
}

/** Get ms elapsed since msNow() */
function msElapsed(start) {
	return msNow() - start;
}

/** Shim for log base 10 */
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

/** HTML escape */
function e(str) {
	return $.htmlEscape(str);
}

/** Check for undefined */
function undef(x) {
	return typeof x == 'undefined';
}

/** Safe json parse */
function jsp() {
	try {
		return JSON.parse(e);
	} catch(e) {
		console.error(e);
		return null;
	}
}
/** Module for toggling a modal overlay */
(function () {
	var modal = {};

	modal.show = function (sel) {
		var $m = $(sel);
		$m.removeClass('hidden visible');
		setTimeout(function () {
			$m.addClass('visible');
		}, 1);
	};

	modal.hide = function (sel) {
		var $m = $(sel);
		$m.removeClass('visible');
		setTimeout(function () {
			$m.addClass('hidden');
		}, 500); // transition time
	};

	modal.init = function () {
		// close modal by click outside the dialog
		$('.Modal').on('click', function () {
			if ($(this).hasClass('no-close')) return; // this is a no-close modal
			modal.hide(this);
		});

		$('.Dialog').on('click', function (e) {
			e.stopImmediatePropagation();
		});

		// Hide all modals on esc
		$(window).on('keydown', function (e) {
			if (e.which == 27) {
				modal.hide('.Modal');
			}
		});
	};

	window.Modal = modal;
})();
(function (nt) {
	var sel = '#notif';

	var hideTmeo1; // timeout to start hiding (transition)
	var hideTmeo2; // timeout to add the hidden class

	nt.show = function (message, timeout) {
		$(sel).html(message);
		Modal.show(sel);

		clearTimeout(hideTmeo1);
		clearTimeout(hideTmeo2);

		if (undef(timeout)) timeout = 2500;

		hideTmeo1 = setTimeout(nt.hide, timeout);
	};

	nt.hide = function () {
		var $m = $(sel);
		$m.removeClass('visible');
		hideTmeo2 = setTimeout(function () {
			$m.addClass('hidden');
		}, 250); // transition time
	};

	nt.init = function() {
		$(sel).on('click', function() {
			nt.hide(this);
		});
	};
})(window.Notify = {});
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
	$('.Box.mobcol').forEach(function(x) {
		var h = x.querySelector('h2');

		var hdl = function() {
			$(x).toggleClass('expanded');
		};
		$(h).on('click', hdl).on('keypress', cr(hdl));
	});

	qsa('form').forEach(function(x) {
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
		qsa('.Box h2').forEach(function (x) {
			x.removeAttribute('tabindex');
		});

		var br = qs('#brand');
		br && br.removeAttribute('tabindex');
	}
});

$._loader = function(vis) {
	$('#loader').toggleClass('show', vis);
};

$.ready(function() {
	setTimeout(function() {
		$('#content').addClass('load');
	}, 1);
});
// Generated from PHP locale file
var _tr = {
    "wifi.connected_ip_is": "Connected, IP is ",
    "wifi.not_conn": "Not connected.",
    "wifi.enter_passwd": "Enter password for \":ssid:\""
};

function tr(key) { return _tr[key] || '?'+key+'?'; }
(function(w) {
	var authStr = ['Open', 'WEP', 'WPA', 'WPA2', 'WPA/WPA2'];
	var curSSID;

	// Get XX % for a slider input
	function rangePt(inp) {
		return Math.round(((inp.value / inp.max)*100)) + '%';
	}

	// Display selected STA SSID etc
	function selectSta(name, password, ip) {
		$('#sta_ssid').val(name);
		$('#sta_password').val(password);

		$('#sta-nw').toggleClass('hidden', name.length == 0);
		$('#sta-nw-nil').toggleClass('hidden', name.length > 0);

		$('#sta-nw .essid').html(e(name));
		var nopw = undef(password) || password.length == 0;
		$('#sta-nw .x-passwd').html(e(password));
		$('#sta-nw .passwd').toggleClass('hidden', nopw);
		$('#sta-nw .nopasswd').toggleClass('hidden', !nopw);
		$('#sta-nw .ip').html(ip.length>0 ? tr('wifi.connected_ip_is')+ip : tr('wifi.not_conn'));
	}

	/** Update display for received response */
	function onScan(resp, status) {
		//var ap_json = {
		//	"result": {
		//		"inProgress": "0",
		//		"APs": [
		//			{"essid": "Chlivek", "bssid": "88:f7:c7:52:b3:99", "rssi": "204", "enc": "4", "channel": "1"},
		//			{"essid": "TyNikdy", "bssid": "5c:f4:ab:0d:f1:1b", "rssi": "164", "enc": "3", "channel": "1"},
		//		]
		//	}
		//};

		if (status != 200) {
			// bad response
			rescan(5000); // wait 5sm then retry
			return;
		}

		try {
			resp = JSON.parse(resp);
		} catch (e) {
			console.log(e);
			rescan(5000);
			return;
		}

		var done = !bool(resp.result.inProgress) && (resp.result.APs.length > 0);
		rescan(done ? 15000 : 1000);
		if (!done) return; // no redraw yet

		// clear the AP list
		var $list = $('#ap-list');
		// remove old APs
		$('#ap-list .AP').remove();

		$list.toggleClass('hidden', !done);
		$('#ap-loader').toggleClass('hidden', done);

		// scan done
		resp.result.APs.sort(function (a, b) {
			return b.rssi - a.rssi;
		}).forEach(function (ap) {
			ap.enc = parseInt(ap.enc);

			if (ap.enc > 4) return; // hide unsupported auths
			WiFi.scan_url = '/cfg/wifi/scan';

			var item = mk('div');

			var $item = $(item)
				.data('ssid', ap.essid)
				.data('pwd', ap.enc)
				.attr('tabindex', 0)
				.addClass('AP');

			// mark current SSID
			if (ap.essid == curSSID) {
				$item.addClass('selected');
			}

			var inner = mk('div');
			$(inner).addClass('inner')
				.htmlAppend('<div class="rssi">{0}</div>'.format(ap.rssi_perc))
				.htmlAppend('<div class="essid" title="{0}">{0}</div>'.format($.htmlEscape(ap.essid)))
				.htmlAppend('<div class="auth">{0}</div>'.format(authStr[ap.enc]));

			$item.on('click', function () {
				var $th = $(this);

				var conn_ssid = $th.data('ssid');
				var conn_pass = '';

				if (+$th.data('pwd')) {
					// this AP needs a password
					conn_pass = prompt(tr("wifi.enter_passwd").replace(":ssid:", conn_ssid));
					if (!conn_pass) return;
				}

				$('#sta_password').val(conn_pass);
				$('#sta_ssid').val(conn_ssid);
				selectSta(conn_ssid, conn_pass, '');
			});


			item.appendChild(inner);
			$list[0].appendChild(item);
		});
	}

	function startScanning() {
		$('#ap-loader').removeClass('hidden');
		$('#ap-scan').addClass('hidden');
		$('#ap-loader .anim-dots').html('.');

		scanAPs();
	}

	/** Ask the CGI what APs are visible (async) */
	function scanAPs() {
		$.get('http://'+_root+w.scan_url, onScan);
	}

	function rescan(time) {
		setTimeout(scanAPs, time);
	}

	/** Set up the WiFi page */
	function wifiInit(cfg) {
		// Update slider value displays
		$('.Row.range').forEach(function(x) {
			var inp = x.querySelector('input');
			var disp1 = x.querySelector('.x-disp1');
			var disp2 = x.querySelector('.x-disp2');
			var t = rangePt(inp);
			$(disp1).html(t);
			$(disp2).html(t);
			$(inp).on('input', function() {
				t = rangePt(inp);
				$(disp1).html(t);
				$(disp2).html(t);
			});
		});

		// Forget STA credentials
		$('#forget-sta').on('click', function() {
			selectSta('', '', '');
			return false;
		});

		selectSta(cfg.sta_ssid, cfg.sta_password, cfg.sta_active_ip);
		curSSID = cfg.sta_active_ssid;
	}

	w.init = wifiInit;
	w.startScanning = startScanning;
})(window.WiFi = {});
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
