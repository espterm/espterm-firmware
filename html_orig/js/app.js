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
//     keymaster.js
//     (c) 2011-2013 Thomas Fuchs
//     keymaster.js may be freely distributed under the MIT license.

;(function(global){
  var k,
    _handlers = {},
    _mods = { 16: false, 18: false, 17: false, 91: false },
    _scope = 'all',
    // modifier keys
    _MODIFIERS = {
      '⇧': 16, shift: 16,
      '⌥': 18, alt: 18, option: 18,
      '⌃': 17, ctrl: 17, control: 17,
      '⌘': 91, command: 91
    },
    // special keys
    _MAP = {
      backspace: 8, tab: 9, clear: 12,
      enter: 13, 'return': 13,
      esc: 27, escape: 27, space: 32,
      left: 37, up: 38,
      right: 39, down: 40,
      del: 46, 'delete': 46,
      home: 36, end: 35,
      pageup: 33, pagedown: 34,
      ',': 188, '.': 190, '/': 191,
      '`': 192, '-': 189, '=': 187,
      ';': 186, '\'': 222,
      '[': 219, ']': 221, '\\': 220,
      // added:
      insert: 45,
      np_0: 96, np_1: 97, np_2: 98, np_3: 99, np_4: 100, np_5: 101,
	  np_6: 102, np_7: 103, np_8: 104, np_9: 105, np_mul: 106,
	  np_add: 107, np_sub: 109, np_point: 110, np_div: 111, numlock: 144,
    },
    code = function(x){
      return _MAP[x] || x.toUpperCase().charCodeAt(0);
    },
    _downKeys = [];

  for(k=1;k<20;k++) _MAP['f'+k] = 111+k;

  // IE doesn't support Array#indexOf, so have a simple replacement
  function index(array, item){
    var i = array.length;
    while(i--) if(array[i]===item) return i;
    return -1;
  }

  // for comparing mods before unassignment
  function compareArray(a1, a2) {
    if (a1.length != a2.length) return false;
    for (var i = 0; i < a1.length; i++) {
        if (a1[i] !== a2[i]) return false;
    }
    return true;
  }

  var modifierMap = {
      16:'shiftKey',
      18:'altKey',
      17:'ctrlKey',
      91:'metaKey'
  };
  function updateModifierKey(event) {
      for(k in _mods) _mods[k] = event[modifierMap[k]];
  };

  function isModifierPressed(mod) {
    if (mod=='control'||mod=='ctrl') return _mods[17];
	if (mod=='shift') return _mods[16];
	if (mod=='meta') return _mods[91];
	if (mod=='alt') return _mods[18];
	return false;
  }

  // handle keydown event
  function dispatch(event) {
    var key, handler, k, i, modifiersMatch, scope;
    key = event.keyCode;

    if (index(_downKeys, key) == -1) {
        _downKeys.push(key);
    }

    // if a modifier key, set the key.<modifierkeyname> property to true and return
    if(key == 93 || key == 224) key = 91; // right command on webkit, command on Gecko
    if(key in _mods) {
      _mods[key] = true;
      // 'assignKey' from inside this closure is exported to window.key
      for(k in _MODIFIERS) if(_MODIFIERS[k] == key) assignKey[k] = true;
      return;
    }
    updateModifierKey(event);

    // see if we need to ignore the keypress (filter() can can be overridden)
    // by default ignore key presses if a select, textarea, or input is focused
    if(!assignKey.filter.call(this, event)) return;

    // abort if no potentially matching shortcuts found
    if (!(key in _handlers)) return;

    scope = getScope();

    // for each potential shortcut
    for (i = 0; i < _handlers[key].length; i++) {
      handler = _handlers[key][i];

      // see if it's in the current scope
      if(handler.scope == scope || handler.scope == 'all'){
        // check if modifiers match if any
        modifiersMatch = handler.mods.length > 0;
        for(k in _mods)
          if((!_mods[k] && index(handler.mods, +k) > -1) ||
            (_mods[k] && index(handler.mods, +k) == -1)) modifiersMatch = false;
        // call the handler and stop the event if neccessary
        if((handler.mods.length == 0 && !_mods[16] && !_mods[18] && !_mods[17] && !_mods[91]) || modifiersMatch){
          if(handler.method(event, handler)===false){
            if(event.preventDefault) event.preventDefault();
              else event.returnValue = false;
            if(event.stopPropagation) event.stopPropagation();
            if(event.cancelBubble) event.cancelBubble = true;
          }
        }
      }
    }
  };

  // unset modifier keys on keyup
  function clearModifier(event){
    var key = event.keyCode, k,
        i = index(_downKeys, key);

    // remove key from _downKeys
    if (i >= 0) {
        _downKeys.splice(i, 1);
    }

    if(key == 93 || key == 224) key = 91;
    if(key in _mods) {
      _mods[key] = false;
      for(k in _MODIFIERS) if(_MODIFIERS[k] == key) assignKey[k] = false;
    }
  };

  function resetModifiers() {
    for(k in _mods) _mods[k] = false;
    for(k in _MODIFIERS) assignKey[k] = false;
  };

  // parse and assign shortcut
  function assignKey(key, scope, method){
    var keys, mods;
    keys = getKeys(key);
    if (method === undefined) {
      method = scope;
      scope = 'all';
    }

    // for each shortcut
    for (var i = 0; i < keys.length; i++) {
      // set modifier keys if any
      mods = [];
      key = keys[i].split('+');
      if (key.length > 1){
        mods = getMods(key);
        key = [key[key.length-1]];
      }
      // convert to keycode and...
      key = key[0]
      key = code(key);
      // ...store handler
      if (!(key in _handlers)) _handlers[key] = [];
      _handlers[key].push({ shortcut: keys[i], scope: scope, method: method, key: keys[i], mods: mods });
    }
  };

  // unbind all handlers for given key in current scope
  function unbindKey(key, scope) {
    var multipleKeys, keys,
      mods = [],
      i, j, obj;

    multipleKeys = getKeys(key);

    for (j = 0; j < multipleKeys.length; j++) {
      keys = multipleKeys[j].split('+');

      if (keys.length > 1) {
        mods = getMods(keys);
      }

      key = keys[keys.length - 1];
      key = code(key);

      if (scope === undefined) {
        scope = getScope();
      }
      if (!_handlers[key]) {
        return;
      }
      for (i = 0; i < _handlers[key].length; i++) {
        obj = _handlers[key][i];
        // only clear handlers if correct scope and mods match
        if (obj.scope === scope && compareArray(obj.mods, mods)) {
          _handlers[key][i] = {};
        }
      }
    }
  };

  // Returns true if the key with code 'keyCode' is currently down
  // Converts strings into key codes.
  function isPressed(keyCode) {
      if (typeof(keyCode)=='string') {
        keyCode = code(keyCode);
      }
      return index(_downKeys, keyCode) != -1;
  }

  function getPressedKeyCodes() {
      return _downKeys.slice(0);
  }

  function filter(event){
    var tagName = (event.target || event.srcElement).tagName;
    // ignore keypressed in any elements that support keyboard data input
    return !(tagName == 'INPUT' || tagName == 'SELECT' || tagName == 'TEXTAREA');
  }

  // initialize key.<modifier> to false
  for(k in _MODIFIERS) assignKey[k] = false;

  // set current scope (default 'all')
  function setScope(scope){ _scope = scope || 'all' };
  function getScope(){ return _scope || 'all' };

  // delete all handlers for a given scope
  function deleteScope(scope){
    var key, handlers, i;

    for (key in _handlers) {
      handlers = _handlers[key];
      for (i = 0; i < handlers.length; ) {
        if (handlers[i].scope === scope) handlers.splice(i, 1);
        else i++;
      }
    }
  };

  // abstract key logic for assign and unassign
  function getKeys(key) {
    var keys;
    key = key.replace(/\s/g, '');
    keys = key.split(',');
    if ((keys[keys.length - 1]) == '') {
      keys[keys.length - 2] += ',';
    }
    return keys;
  }

  // abstract mods logic for assign and unassign
  function getMods(key) {
    var mods = key.slice(0, key.length - 1);
    for (var mi = 0; mi < mods.length; mi++)
    mods[mi] = _MODIFIERS[mods[mi]];
    return mods;
  }

  // cross-browser events
  function addEvent(object, event, method) {
    if (object.addEventListener)
      object.addEventListener(event, method, false);
    else if(object.attachEvent)
      object.attachEvent('on'+event, function(){ method(window.event) });
  };

  // set the handlers globally on document
  addEvent(document, 'keydown', function(event) { dispatch(event) }); // Passing _scope to a callback to ensure it remains the same by execution. Fixes #48
  addEvent(document, 'keyup', clearModifier);

  // reset modifiers to false whenever the window is (re)focused.
  addEvent(window, 'focus', resetModifiers);

  // store previously defined key
  var previousKey = global.key;

  // restore previously defined key and return reference to our key object
  function noConflict() {
    var k = global.key;
    global.key = previousKey;
    return k;
  }

  // set window.key and window.key.set/get/deleteScope, and the default filter
  global.key = assignKey;
  global.key.setScope = setScope;
  global.key.getScope = getScope;
  global.key.deleteScope = deleteScope;
  global.key.filter = filter;
  global.key.isPressed = isPressed;
  global.key.isModifier = isModifierPressed;
  global.key.getPressedKeyCodes = getPressedKeyCodes;
  global.key.noConflict = noConflict;
  global.key.unbind = unbindKey;

  if(typeof module !== 'undefined') module.exports = assignKey;

})(this);
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
function jsp(str) {
	try {
		return JSON.parse(str);
	} catch(e) {
		console.error(e);
		return null;
	}
}

/** Create a character from ASCII code */
function Chr(n) {
	return String.fromCharCode(n);
}

/** Decode number from 2B encoding */
function parse2B(s, i) {
	return (s.charCodeAt(i++) - 1) + (s.charCodeAt(i) - 1) * 127;
}

/** Decode number from 3B encoding */
function parse3B(s, i) {
	return (s.charCodeAt(i) - 1) + (s.charCodeAt(i+1) - 1) * 127 + (s.charCodeAt(i+2) - 1) * 127 * 127;
}

/** Encode using 2B encoding, returns string. */
function encode2B(n) {
	var lsb, msb;
	lsb = (n % 127);
	n = ((n - lsb) / 127);
	lsb += 1;
	msb = (n + 1);
	return Chr(lsb) + Chr(msb);
}

/** Encode using 3B encoding, returns string. */
function encode3B(n) {
	var lsb, msb, xsb;
	lsb = (n % 127);
	n = (n - lsb) / 127;
	lsb += 1;
	msb = (n % 127);
	n = (n - msb) / 127;
	msb += 1;
	xsb = (n + 1);
	return Chr(lsb) + Chr(msb) + Chr(xsb);
}
/** Module for toggling a modal overlay */
(function () {
	var modal = {};
	var curCloseCb = null;

	modal.show = function (sel, closeCb) {
		var $m = $(sel);
		$m.removeClass('hidden visible');
		setTimeout(function () {
			$m.addClass('visible');
		}, 1);
		curCloseCb = closeCb;
	};

	modal.hide = function (sel) {
		var $m = $(sel);
		$m.removeClass('visible');
		setTimeout(function () {
			$m.addClass('hidden');
			if (curCloseCb) curCloseCb();
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
	$('.Box.mobcol,.Box.fold').forEach(function(x) {
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
		if (_demo) {
			onScan(_demo_aps, 200);
		} else {
			$.get('http://' + _root + '/cfg/wifi/scan', onScan);
		}
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
/** Handle connections */
var Conn = (function() {
	var ws;
	var heartbeatTout;
	var pingIv;
	var xoff = false;
	var autoXoffTout;

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
			switch (evt.data.charAt(0)) {
				case 'B':
				case 'T':
				case 'S':
					Screen.load(evt.data);
					break;

				case '-':
					//console.log('xoff');
					xoff = true;
					autoXoffTout = setTimeout(function(){xoff=false;}, 250);
					break;

				case '+':
					//console.log('xon');
					xoff = false;
					clearTimeout(autoXoffTout);
					break;
			}
			heartbeat();
		} catch(e) {
			console.error(e);
		}
	}

	function canSend() {
		return !xoff;
	}

	function doSend(message) {
		if (_demo) {
			console.log("TX: ", message);
			return true; // Simulate success
		}
		if (xoff) {
			// TODO queue
			console.log("Can't send, flood control.");
			return false;
		}

		if (!ws) return false; // for dry testing
		if (ws.readyState != 1) {
			console.error("Socket not ready");
			return false;
		}
		if (typeof message != "string") {
			message = JSON.stringify(message);
		}
		ws.send(message);
		return true;
	}

	function init() {
		if (_demo) {
			console.log("Demo mode!");
			Screen.load(_demo_screen);
			showPage();
			return;
		}
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
		heartbeatTout = setTimeout(heartbeatFail, 2000);
	}

	function heartbeatFail() {
		console.error("Heartbeat lost, probing server...");
		pingIv = setInterval(function() {
			console.log("> ping");
			$.get('http://'+_root+'/system/ping', function(resp, status) {
				if (status == 200) {
					clearInterval(pingIv);
					console.info("Server ready, reloading page...");
					location.reload();
				}
			}, {
				timeout: 100,
			});
		}, 500);
	}

	return {
		ws: null,
		init: init,
		send: doSend,
		canSend: canSend, // check flood control
	};
})();
/**
 * User input
 *
 * --- Rx messages: ---
 * S - screen content (binary encoding of the entire screen with simple compression)
 * T - text labels - Title and buttons, \0x01-separated
 * B - beep
 * . - heartbeat
 *
 * --- Tx messages ---
 * s - string
 * b - action button
 * p - mb press
 * r - mb release
 * m - mouse move
 */
var Input = (function() {
	var opts = {
		np_alt: false,
		cu_alt: false,
		fn_alt: false,
		mt_click: false,
		mt_move: false,
		no_keys: false,
	};

	/** Send a literal message */
	function sendStrMsg(str) {
		return Conn.send("s"+str);
	}

	/** Send a button event */
	function sendBtnMsg(n) {
		Conn.send("b"+Chr(n));
	}

	/** Fn alt choice for key message */
	function fa(alt, normal) {
		return opts.fn_alt ? alt : normal;
	}

	/** Cursor alt choice for key message */
	function ca(alt, normal) {
		return opts.cu_alt ? alt : normal;
	}

	/** Numpad alt choice for key message */
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

	/** Bind a keystroke to message */
	function bind(combo, str) {
		// mac fix - allow also cmd
		if (combo.indexOf('ctrl+') !== -1) {
			combo += ',' + combo.replace('ctrl', 'command');
		}

		// unbind possible old binding
		key.unbind(combo);

		key(combo, function (e) {
			if (opts.no_keys) return;
			e.preventDefault();
			sendStrMsg(str)
		});
	}

	/** Bind/rebind key messages */
	function _initKeys() {
		// This takes care of text characters typed
		window.addEventListener('keypress', function(evt) {
			if (opts.no_keys) return;
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

	// mouse button states
	var mb1 = 0;
	var mb2 = 0;
	var mb3 = 0;

	/** Init the Input module */
	function init() {
		_initKeys();

		// Button presses
		qsa('#action-buttons button').forEach(function(s) {
			s.addEventListener('click', function() {
				sendBtnMsg(+this.dataset['n']);
			});
		});

		// global mouse state tracking - for motion reporting
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

	/** Prepare modifiers byte for mouse message */
	function packModifiersForMouse() {
		return (key.isModifier('ctrl')?1:0) |
			(key.isModifier('shift')?2:0) |
			(key.isModifier('alt')?4:0) |
			(key.isModifier('meta')?8:0);
	}

	return {
		/** Init the Input module */
		init: init,

		/** Send a literal string message */
		sendString: sendStrMsg,

		/** Enable alternate key modes (cursors, numpad, fn) */
		setAlts: function(cu, np, fn) {
			if (opts.cu_alt != cu || opts.np_alt != np || opts.fn_alt != fn) {
				opts.cu_alt = cu;
				opts.np_alt = np;
				opts.fn_alt = fn;

				// rebind keys - codes have changed
				_bindFnKeys();
			}
		},

		setMouseMode: function(click, move) {
			opts.mt_click = click;
			opts.mt_move = move;
		},

		// Mouse events
		onMouseMove: function (x, y) {
			if (!opts.mt_move) return;
			var b = mb1 ? 1 : mb2 ? 2 : mb3 ? 3 : 0;
			var m = packModifiersForMouse();
			Conn.send("m" + encode2B(y) + encode2B(x) + encode2B(b) + encode2B(m));
		},
		onMouseDown: function (x, y, b) {
			if (!opts.mt_click) return;
			if (b > 3 || b < 1) return;
			var m = packModifiersForMouse();
			Conn.send("p" + encode2B(y) + encode2B(x) + encode2B(b) + encode2B(m));
			// console.log("B ",b," M ",m);
		},
		onMouseUp: function (x, y, b) {
			if (!opts.mt_click) return;
			if (b > 3 || b < 1) return;
			var m = packModifiersForMouse();
			Conn.send("r" + encode2B(y) + encode2B(x) + encode2B(b) + encode2B(m));
			// console.log("B ",b," M ",m);
		},
		onMouseWheel: function (x, y, dir) {
			if (!opts.mt_click) return;
			// -1 ... btn 4 (away from user)
			// +1 ... btn 5 (towards user)
			var m = packModifiersForMouse();
			var b = (dir < 0 ? 4 : 5);
			Conn.send("p" + encode2B(y) + encode2B(x) + encode2B(b) + encode2B(m));
			// console.log("B ",b," M ",m);
		},
		mouseTracksClicks: function() {
			return opts.mt_click;
		},
		blockKeys: function(yes) {
			opts.no_keys = yes;
		}
	};
})();

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
		qsa('#action-buttons button').forEach(function(x, i) {
			var s = pieces[i+1].trim();
			// if empty string, use the "dim" effect and put nbsp instead to stretch the btn vertically
			x.innerHTML = s.length > 0 ? e(s) : "&nbsp;";
			x.style.opacity = s.length > 0 ? 1 : 0.2;
		});
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
/** Init the terminal sub-module - called from HTML */
window.termInit = function () {
	Conn.init();
	Input.init();
	TermUpl.init();
};
