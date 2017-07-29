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
