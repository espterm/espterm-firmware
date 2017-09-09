// Some non-bold Fraktur symbols are outside the contiguous block
const frakturExceptions = {
  'C': '\u212d',
  'H': '\u210c',
  'I': '\u2111',
  'R': '\u211c',
  'Z': '\u2128'
};

// constants for decoding the update blob
const SEQ_SET_COLOR_ATTR = 1;
const SEQ_REPEAT = 2;
const SEQ_SET_COLOR = 3;
const SEQ_SET_ATTR = 4;

const SELECTION_BG = '#b2d7fe';
const SELECTION_FG = '#333';

const themes = [
  [ // Tango
    '#111213', '#CC0000', '#4E9A06', '#C4A000', '#3465A4', '#75507B', '#06989A', '#D3D7CF',
    '#555753', '#EF2929', '#8AE234', '#FCE94F', '#729FCF', '#AD7FA8', '#34E2E2', '#EEEEEC',
  ],
  [ // Linux
    '#000000', '#aa0000', '#00aa00', '#aa5500', '#0000aa', '#aa00aa', '#00aaaa', '#aaaaaa',
    '#555555', '#ff5555', '#55ff55', '#ffff55', '#5555ff', '#ff55ff', '#55ffff', '#ffffff',
  ],
  [ // xterm
    '#000000', '#cd0000', '#00cd00', '#cdcd00', '#0000ee', '#cd00cd', '#00cdcd', '#e5e5e5',
    '#7f7f7f', '#ff0000', '#00ff00', '#ffff00', '#5c5cff', '#ff00ff', '#00ffff', '#ffffff',
  ],
  [ // rxvt
    '#000000', '#cd0000', '#00cd00', '#cdcd00', '#0000cd', '#cd00cd', '#00cdcd', '#faebd7',
    '#404040', '#ff0000', '#00ff00', '#ffff00', '#0000ff', '#ff00ff', '#00ffff', '#ffffff',
  ],
  [ // Ambience
    '#2e3436', '#cc0000', '#4e9a06', '#c4a000', '#3465a4', '#75507b', '#06989a', '#d3d7cf',
    '#555753', '#ef2929', '#8ae234', '#fce94f', '#729fcf', '#ad7fa8', '#34e2e2', '#eeeeec',
  ],
  [ // Solarized
    '#073642', '#dc322f', '#859900', '#b58900', '#268bd2', '#d33682', '#2aa198', '#eee8d5',
    '#002b36', '#cb4b16', '#586e75', '#657b83', '#839496', '#6c71c4', '#93a1a1', '#fdf6e3',
  ]
];

class TermScreen {
  constructor () {
    this.canvas = document.createElement('canvas');
    this.ctx = this.canvas.getContext('2d');

    if ('AudioContext' in window || 'webkitAudioContext' in window) {
      this.audioCtx = new (window.AudioContext || window.webkitAudioContext)()
    } else {
      console.warn('No AudioContext!')
    }

    this.cursor = {
      x: 0,
      y: 0,
      fg: 7,
      bg: 0,
      attrs: 0,
      blinkOn: false,
      visible: true,
      hanging: false,
      style: 'block',
      blinkInterval: 0,
    };

    this._colors = themes[0];

    this._window = {
      width: 0,
      height: 0,
      devicePixelRatio: 1,
      fontFamily: '"DejaVu Sans Mono", "Liberation Mono", "Inconsolata", "Menlo", monospace',
      fontSize: 20,
      gridScaleX: 1.0,
      gridScaleY: 1.2,
      blinkStyleOn: true,
      blinkInterval: null,
      fitIntoWidth: 0,
      fitIntoHeight: 0,
    };

    // properties of this.window that require updating size and redrawing
    this.windowState = {
      width: 0,
      height: 0,
      devicePixelRatio: 0,
      gridScaleX: 0,
      gridScaleY: 0,
      fontFamily: '',
      fontSize: 0,
      fitIntoWidth: 0,
      fitIntoHeight: 0,
    };

    // current selection
    this.selection = {
      // when false, this will prevent selection in favor of mouse events,
      // though alt can be held to override it
      selectable: true,

      start: [0, 0],
      end: [0, 0],
    };

    this.mouseMode = { clicks: false, movement: false };

    // event listeners
    this._listeners = {};

    const self = this;
    this.window = new Proxy(this._window, {
      set (target, key, value, receiver) {
        target[key] = value;
        self.scheduleSizeUpdate();
        self.scheduleDraw();
        return true
      }
    });

    this.screen = [];
    this.screenFG = [];
    this.screenBG = [];
    this.screenAttrs = [];

    // used to determine if a cell should be redrawn
    this.drawnScreen = [];
    this.drawnScreenFG = [];
    this.drawnScreenBG = [];
    this.drawnScreenAttrs = [];

    this.resetBlink();
    this.resetCursorBlink();

    let selecting = false;
    let selectStart = (x, y) => {
      if (selecting) return;
      selecting = true;
      this.selection.start = this.selection.end = this.screenToGrid(x, y);
      this.scheduleDraw();
    };
    let selectMove = (x, y) => {
      if (!selecting) return;
      this.selection.end = this.screenToGrid(x, y);
      this.scheduleDraw();
    };
    let selectEnd = (x, y) => {
      if (!selecting) return;
      selecting = false;
      this.selection.end = this.screenToGrid(x, y);
      this.scheduleDraw();
      Object.assign(this.selection, this.getNormalizedSelection());
    };

    this.canvas.addEventListener('mousedown', e => {
      if (this.selection.selectable || e.altKey) {
        selectStart(e.offsetX, e.offsetY)
      } else {
        Input.onMouseDown(...this.screenToGrid(e.offsetX, e.offsetY),
          e.button + 1)
      }
    });
    window.addEventListener('mousemove', e => {
      selectMove(e.offsetX, e.offsetY)
    });
    window.addEventListener('mouseup', e => {
      selectEnd(e.offsetX, e.offsetY)
    });

    let touchPosition = null;
    let touchDownTime = 0;
    let touchSelectMinTime = 500;
    let touchDidMove = false;
    let getTouchPositionOffset = touch => {
      let rect = this.canvas.getBoundingClientRect();
      return [touch.clientX - rect.left, touch.clientY - rect.top];
    };
    this.canvas.addEventListener('touchstart', e => {
      touchPosition = getTouchPositionOffset(e.touches[0]);
      touchDidMove = false;
      touchDownTime = Date.now();
    });
    this.canvas.addEventListener('touchmove', e => {
      touchPosition = getTouchPositionOffset(e.touches[0]);

      if (!selecting && touchDidMove === false) {
        if (touchDownTime < Date.now() - touchSelectMinTime) {
          selectStart(...touchPosition);
        }
      } else if (selecting) {
        e.preventDefault();
        selectMove(...touchPosition);
      }

      touchDidMove = true;
    });
    this.canvas.addEventListener('touchend', e => {
      if (e.touches[0]) {
        touchPosition = getTouchPositionOffset(e.touches[0]);
      }
      if (selecting) {
        e.preventDefault();
        selectEnd(...touchPosition);

        let touchSelectMenu = qs('#touch-select-menu')
        touchSelectMenu.classList.add('open');
        let rect = touchSelectMenu.getBoundingClientRect()

        // use middle position for x and one line above for y
        let selectionPos = this.gridToScreen(
          (this.selection.start[0] + this.selection.end[0]) / 2,
          this.selection.start[1] - 1
        );
        selectionPos[0] -= rect.width / 2
        selectionPos[1] -= rect.height / 2
        touchSelectMenu.style.transform = `translate(${selectionPos[0]}px, ${
          selectionPos[1]}px)`
      }

      if (!touchDidMove) {
        this.emit('tap', Object.assign(e, {
          x: touchPosition[0],
          y: touchPosition[1],
        }))
      }

      touchPosition = null;
    });

    this.on('tap', e => {
      if (this.selection.start[0] !== this.selection.end[0] ||
        this.selection.start[1] !== this.selection.end[1]) {
        // selection is not empty
        // reset selection
        this.selection.start = this.selection.end = [0, 0];
        qs('#touch-select-menu').classList.remove('open');
        this.scheduleDraw();
      } else {
        e.preventDefault();
        this.emit('open-soft-keyboard');
      }
    })

    $.ready(() => {
      let copyButton = qs('#touch-select-copy-btn')
      copyButton.addEventListener('click', () => {
        this.copySelectionToClipboard();
      });
    });

    this.canvas.addEventListener('mousemove', e => {
      if (!selecting) {
        Input.onMouseMove(...this.screenToGrid(e.offsetX, e.offsetY))
      }
    });
    this.canvas.addEventListener('mouseup', e => {
      if (!selecting) {
        Input.onMouseUp(...this.screenToGrid(e.offsetX, e.offsetY),
          e.button + 1)
      }
    });
    this.canvas.addEventListener('wheel', e => {
      if (this.mouseMode.clicks) {
        Input.onMouseWheel(...this.screenToGrid(e.offsetX, e.offsetY),
          e.deltaY > 0 ? 1 : -1);

        // prevent page scrolling
        e.preventDefault();
      }
    });
    this.canvas.addEventListener('contextmenu', e => {
      // prevent mouse keys getting stuck
      e.preventDefault();
    })

    // bind ctrl+shift+c to copy
    key('⌃+⇧+c', e => {
      e.preventDefault();
      this.copySelectionToClipboard()
    });
  }

  on (event, listener) {
    if (!this._listeners[event]) this._listeners[event] = [];
    this._listeners[event].push({ listener });
  }

  once (event, listener) {
    if (!this._listeners[event]) this._listeners[event] = [];
    this._listeners[event].push({ listener, once: true });
  }

  off (event, listener) {
    let listeners = this._listeners[event];
    if (listeners) {
      for (let i in listeners) {
        if (listeners[i].listener === listener) {
          listeners.splice(i, 1);
          break;
        }
      }
    }
  }

  emit (event, ...args) {
    let listeners = this._listeners[event];
    if (listeners) {
      let remove = [];
      for (let listener of listeners) {
        try {
          listener.listener(...args);
          if (listener.once) remove.push(listener);
        } catch (err) {
          console.error(err);
        }
      }

      // this needs to be done in this roundabout way because for loops
      // do not like arrays with changing lengths
      for (let listener of remove) {
        listeners.splice(listeners.indexOf(listener), 1);
      }
    }
  }

  get colors () { return this._colors }
  set colors (theme) {
    this._colors = theme;
    this.scheduleDraw();
  }

  getColor (i) {
    if (i === -1) return SELECTION_FG
    if (i === -2) return SELECTION_BG
    return this.colors[i]
  }

  // schedule a size update in the next tick
  scheduleSizeUpdate () {
    clearTimeout(this._scheduledSizeUpdate);
    this._scheduledSizeUpdate = setTimeout(() => this.updateSize(), 1)
  }

  // schedule a draw in the next tick
  scheduleDraw (aggregateTime = 1) {
    clearTimeout(this._scheduledDraw);
    this._scheduledDraw = setTimeout(() => this.draw(), aggregateTime)
  }

  getFont (modifiers = {}) {
    let fontStyle = modifiers.style || 'normal';
    let fontWeight = modifiers.weight || 'normal';
    return `${fontStyle} normal ${fontWeight} ${this.window.fontSize}px ${this.window.fontFamily}`
  }

  getCharSize () {
    this.ctx.font = this.getFont();

    return {
      width: Math.floor(this.ctx.measureText(' ').width),
      height: this.window.fontSize
    }
  }

  getCellSize () {
    let charSize = this.getCharSize();

    return {
      width: Math.ceil(charSize.width * this.window.gridScaleX),
      height: Math.ceil(charSize.height * this.window.gridScaleY)
    }
  }

  updateSize () {
    this._window.devicePixelRatio = window.devicePixelRatio || 1;

    let didChange = false;
    for (let key in this.windowState) {
      if (this.windowState.hasOwnProperty(key) && this.windowState[key] !== this.window[key]) {
        didChange = true;
        this.windowState[key] = this.window[key];
      }
    }

    if (didChange) {
      const {
        width,
        height,
        devicePixelRatio,
        gridScaleX,
        gridScaleY,
        fitIntoWidth,
        fitIntoHeight
      } = this.window;
      const cellSize = this.getCellSize();

      // real height of the canvas element in pixels
      let realWidth = width * cellSize.width
      let realHeight = height * cellSize.height

      if (fitIntoWidth && fitIntoHeight) {
        if (realWidth > fitIntoWidth || realHeight > fitIntoHeight) {
          let terminalAspect = realWidth / realHeight
          let fitAspect = fitIntoWidth / fitIntoHeight

          if (terminalAspect < fitAspect) {
            // align heights
            realHeight = fitIntoHeight
            realWidth = realHeight * terminalAspect
          } else {
            // align widths
            realWidth = fitIntoWidth
            realHeight = realWidth / terminalAspect
          }
        }
      } else if (fitIntoWidth && realWidth > fitIntoWidth) {
        realHeight = fitIntoWidth / (realWidth / realHeight)
        realWidth = fitIntoWidth
      } else if (fitIntoHeight && realHeight > fitIntoHeight) {
        realWidth = fitIntoHeight * (realWidth / realHeight)
        realHeight = fitIntoHeight
      }

      this.canvas.width = width * devicePixelRatio * cellSize.width;
      this.canvas.style.width = `${realWidth}px`;
      this.canvas.height = height * devicePixelRatio * cellSize.height;
      this.canvas.style.height = `${realHeight}px`;

      // the screen has been cleared (by changing canvas width)
      this.drawnScreen = [];
      this.drawnScreenFG = [];
      this.drawnScreenBG = [];
      this.drawnScreenAttrs = [];

      // draw immediately; the canvas shouldn't flash
      this.draw();
    }
  }

  resetCursorBlink () {
    this.cursor.blinkOn = true;
    clearInterval(this.cursor.blinkInterval);
    this.cursor.blinkInterval = setInterval(() => {
      this.cursor.blinkOn = !this.cursor.blinkOn;
      this.scheduleDraw();
    }, 500);
  }

  resetBlink () {
    this.window.blinkStyleOn = true;
    clearInterval(this.window.blinkInterval);
    let intervals = 0;
    this.window.blinkInterval = setInterval(() => {
      intervals++;
      if (intervals >= 4 && this.window.blinkStyleOn) {
        this.window.blinkStyleOn = false;
        intervals = 0;
      } else if (intervals >= 1 && !this.window.blinkStyleOn) {
        this.window.blinkStyleOn = true;
        intervals = 0;
      }
    }, 200);
  }

  getNormalizedSelection () {
    let { start, end } = this.selection;
    // if the start line is after the end line, or if they're both on the same
    // line but the start column comes after the end column, swap
    if (start[1] > end[1] || (start[1] === end[1] && start[0] > end[0])) {
      [start, end] = [end, start];
    }
    return { start, end };
  }

  isInSelection (col, line) {
    let { start, end } = this.getNormalizedSelection();
    let colAfterStart = start[0] <= col;
    let colBeforeEnd = col < end[0];
    let onStartLine = line === start[1];
    let onEndLine = line === end[1];

    if (onStartLine && onEndLine) return colAfterStart && colBeforeEnd;
    else if (onStartLine) return colAfterStart;
    else if (onEndLine) return colBeforeEnd;
    else return start[1] < line && line < end[1];
  }

  getSelectedText () {
    const screenLength = this.window.width * this.window.height;
    let lines = [];
    let previousLineIndex = -1;

    for (let cell = 0; cell < screenLength; cell++) {
      let x = cell % this.window.width;
      let y = Math.floor(cell / this.window.width);

      if (this.isInSelection(x, y)) {
        if (previousLineIndex !== y) {
          previousLineIndex = y;
          lines.push('');
        }
        lines[lines.length - 1] += this.screen[cell];
      }
    }

    return lines.join('\n');
  }

  copySelectionToClipboard () {
    let selectedText = this.getSelectedText();
    // don't copy anything if nothing is selected
    if (!selectedText) return;
    let textarea = document.createElement('textarea');
    document.body.appendChild(textarea);
    textarea.value = selectedText;
    textarea.select();
    if (document.execCommand('copy')) {
      Notify.show('Copied to clipboard');
    } else {
      Notify.show('Failed to copy');
      // unsuccessful copy
    }
    document.body.removeChild(textarea);
  }

  screenToGrid (x, y) {
    let cellSize = this.getCellSize();

    return [
      Math.floor((x + cellSize.width / 2) / cellSize.width),
      Math.floor(y / cellSize.height),
    ];
  }

  gridToScreen (x, y) {
    let cellSize = this.getCellSize();

    return [ x * cellSize.width, y * cellSize.height ];
  }

  drawCell ({ x, y, charSize, cellWidth, cellHeight, text, fg, bg, attrs }) {
    const ctx = this.ctx;
    ctx.fillStyle = this.getColor(bg);
    ctx.fillRect(x * cellWidth, y * cellHeight,
      Math.ceil(cellWidth), Math.ceil(cellHeight));

    if (!text) return;

    let underline = false;
    let blink = false;
    let strike = false;
    if (attrs & 1 << 1) ctx.globalAlpha = 0.5;
    if (attrs & 1 << 3) underline = true;
    if (attrs & 1 << 4) blink = true;
    if (attrs & 1 << 5) text = TermScreen.alphaToFraktur(text);
    if (attrs & 1 << 6) strike = true;

    if (!blink || this.window.blinkStyleOn) {
      ctx.fillStyle = this.getColor(fg);
      ctx.fillText(text, (x + 0.5) * cellWidth, (y + 0.5) * cellHeight);

      if (underline || strike) {
        ctx.strokeStyle = this.getColor(fg);
        ctx.lineWidth = 1;
        ctx.lineCap = 'round';
        ctx.beginPath();

        if (underline) {
          let lineY = Math.round(y * cellHeight + charSize.height) + 0.5;
          ctx.moveTo(x * cellWidth, lineY);
          ctx.lineTo((x + 1) * cellWidth, lineY);
        }

        if (strike) {
          let lineY = Math.round((y + 0.5) * cellHeight) + 0.5;
          ctx.moveTo(x * cellWidth, lineY);
          ctx.lineTo((x + 1) * cellWidth, lineY);
        }

        ctx.stroke();
      }
    }

    ctx.globalAlpha = 1;
  }

  draw () {
    const ctx = this.ctx;
    const {
      width,
      height,
      devicePixelRatio,
      gridScaleX,
      gridScaleY
    } = this.window;

    const charSize = this.getCharSize();
    const { width: cellWidth, height: cellHeight } = this.getCellSize();
    const screenWidth = width * cellWidth;
    const screenHeight = height * cellHeight;
    const screenLength = width * height;

    ctx.setTransform(devicePixelRatio, 0, 0, devicePixelRatio, 0, 0);

    ctx.font = this.getFont();
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';

    // bits in the attr value that affect the font
    const FONT_MASK = 0b101;

    // Map of (attrs & FONT_MASK) -> Array of cell indices
    const fontGroups = new Map();

    // Map of (cell index) -> boolean, whether or not a cell needs to be redrawn
    const updateMap = new Map();

    for (let cell = 0; cell < screenLength; cell++) {
      let x = cell % width;
      let y = Math.floor(cell / width);
      let isCursor = this.cursor.x === x && this.cursor.y === y &&
        !this.cursor.hanging;
      let invertForCursor = isCursor && this.cursor.blinkOn &&
        this.cursor.style === 'block';
      let inSelection = this.isInSelection(x, y)

      let text = this.screen[cell];
      let fg = invertForCursor ? this.screenBG[cell] : this.screenFG[cell];
      let bg = invertForCursor ? this.screenFG[cell] : this.screenBG[cell];
      let attrs = this.screenAttrs[cell];

      // HACK: ensure cursor is visible
      if (invertForCursor && fg === bg) bg = fg === 0 ? 7 : 0;

      if (inSelection) {
        fg = -1
        bg = -2
      }

      let cellDidChange = text !== this.drawnScreen[cell] ||
        fg !== this.drawnScreenFG[cell] ||
        bg !== this.drawnScreenBG[cell] ||
        attrs !== this.drawnScreenAttrs[cell];

      let font = attrs & FONT_MASK;
      if (!fontGroups.has(font)) fontGroups.set(font, []);

      fontGroups.get(font).push([cell, x, y, text, fg, bg, attrs, isCursor]);
      updateMap.set(cell, cellDidChange);
    }

    for (let font of fontGroups.keys()) {
      // set font once because in Firefox, this is a really slow action for some
      // reason
      let modifiers = {};
      if (font & 1) modifiers.weight = 'bold';
      if (font & 1 << 2) modifiers.style = 'italic';
      ctx.font = this.getFont(modifiers);

      for (let data of fontGroups.get(font)) {
        let [cell, x, y, text, fg, bg, attrs, isCursor] = data;

        // check if this cell or any adjacent cells updated
        let needsUpdate = false;
        let updateCells = [
          cell,
          cell - 1,
          cell + 1,
          cell - width,
          cell + width,
          // diagonal box drawing characters exist, too
          cell - width - 1,
          cell - width + 1,
          cell + width - 1,
          cell + width + 1
        ];
        for (let index of updateCells) {
          if (updateMap.has(index) && updateMap.get(index)) {
            needsUpdate = true;
            break;
          }
        }

        if (needsUpdate) {
          this.drawCell({
            x, y, charSize, cellWidth, cellHeight, text, fg, bg, attrs
          });

          this.drawnScreen[cell] = text;
          this.drawnScreenFG[cell] = fg;
          this.drawnScreenBG[cell] = bg;
          this.drawnScreenAttrs[cell] = attrs;
        }

        if (isCursor && this.cursor.blinkOn && this.cursor.style !== 'block') {
          ctx.save();
          ctx.beginPath();
          if (this.cursor.style === 'bar') {
            // vertical bar
            let barWidth = 2;
            ctx.rect(x * cellWidth, y * cellHeight, barWidth, cellHeight)
          } else if (this.cursor.style === 'line') {
            // underline
            let lineHeight = 2;
            ctx.rect(x * cellWidth, y * cellHeight + charSize.height,
              cellWidth, lineHeight)
          }
          ctx.clip();

          // swap foreground/background
          fg = this.screenBG[cell];
          bg = this.screenFG[cell];
          // HACK: ensure cursor is visible
          if (fg === bg) bg = fg === 0 ? 7 : 0;

          this.drawCell({
            x, y, charSize, cellWidth, cellHeight, text, fg, bg, attrs
          });
          ctx.restore();
        }
      }
    }
  }

  loadContent (str) {
    // current index
    let i = 0;

    // window size
    this.window.height = parse2B(str, i);
    this.window.width = parse2B(str, i + 2);
    this.updateSize();
    i += 4;

    // cursor position
    let [cursorY, cursorX] = [parse2B(str, i), parse2B(str, i + 2)];
    i += 4;
    let cursorMoved = (cursorX !== this.cursor.x || cursorY !== this.cursor.y);
    this.cursor.x = cursorX;
    this.cursor.y = cursorY;

    if (cursorMoved) {
      this.resetCursorBlink();
      this.emit('cursor-moved');
    }

    // attributes
    let attributes = parse2B(str, i);
    i += 2;

    this.cursor.visible = !!(attributes & 1);
    this.cursor.hanging = !!(attributes & 1 << 1);

    Input.setAlts(
      !!(attributes & 1 << 2), // cursors alt
      !!(attributes & 1 << 3), // numpad alt
      !!(attributes & 1 << 4)  // fn keys alt
    );

    let trackMouseClicks = !!(attributes & 1 << 5);
    let trackMouseMovement = !!(attributes & 1 << 6);

    Input.setMouseMode(trackMouseClicks, trackMouseMovement);
    this.selection.selectable = !trackMouseMovement;
    $(this.canvas).toggleClass('selectable', !trackMouseMovement);
    this.mouseMode = {
      clicks: trackMouseClicks,
      movement: trackMouseMovement
    };

    let showButtons = !!(attributes & 1 << 7);
    let showConfigLinks = !!(attributes & 1 << 8);

    $('.x-term-conf-btn').toggleClass('hidden', !showConfigLinks);
    $('#action-buttons').toggleClass('hidden', !showButtons);

    // content
    let fg = 7;
    let bg = 0;
    let attrs = 0;
    let cell = 0; // cell index
    let lastChar = ' ';
    let screenLength = this.window.width * this.window.height;

    this.screen = new Array(screenLength).fill(' ');
    this.screenFG = new Array(screenLength).fill(' ');
    this.screenBG = new Array(screenLength).fill(' ');
    this.screenAttrs = new Array(screenLength).fill(' ');

    let strArray = typeof Array.from !== 'undefined'
      ? Array.from(str)
      : str.split('');

    while (i < strArray.length && cell < screenLength) {
      let character = strArray[i++];
      let charCode = character.codePointAt(0);

      let data;
      switch (charCode) {
        case SEQ_SET_COLOR_ATTR:
          data = parse3B(strArray[i] + strArray[i + 1] + strArray[i + 2]);
          i += 3;
          fg = data & 0xF;
          bg = data >> 4 & 0xF;
          attrs = data >> 8 & 0xFF;
          break;

        case SEQ_SET_COLOR:
          data = parse2B(strArray[i] + strArray[i + 1]);
          i += 2;
          fg = data & 0xF;
          bg = data >> 4 & 0xF;
          break;

        case SEQ_SET_ATTR:
          data = parse2B(strArray[i] + strArray[i + 1]);
          i += 2;
          attrs = data & 0xFF;
          break;

        case SEQ_REPEAT:
          let count = parse2B(strArray[i] + strArray[i + 1]);
          i += 2;
          for (let j = 0; j < count; j++) {
            this.screen[cell] = lastChar;
            this.screenFG[cell] = fg;
            this.screenBG[cell] = bg;
            this.screenAttrs[cell] = attrs;

            if (++cell > screenLength) break;
          }
          break;

        default:
          // safety replacement
          if (charCode < 32) character = '\ufffd';
          // unique cell character
          this.screen[cell] = lastChar = character;
          this.screenFG[cell] = fg;
          this.screenBG[cell] = bg;
          this.screenAttrs[cell] = attrs;
          cell++;
      }
    }

    this.scheduleDraw(16);
    this.emit('load');
  }

  /** Apply labels to buttons and screen title (leading T removed already) */
  loadLabels (str) {
    let pieces = str.split('\x01');
    qs('h1').textContent = pieces[0];
    $('#action-buttons button').forEach((button, i) => {
      let label = pieces[i + 1].trim();
      // if empty string, use the "dim" effect and put nbsp instead to
      // stretch the button vertically
      button.innerHTML = label ? e(label) : '&nbsp;';
      button.style.opacity = label ? 1 : 0.2;
    })
  }

  load (str) {
    const content = str.substr(1);

    switch (str[0]) {
      case 'S':
        this.loadContent(content);
        break;
      case 'T':
        this.loadLabels(content);
        break;
      case 'B':
        this.beep();
        break;
      default:
        console.warn(`Bad data message type; ignoring.\n${JSON.stringify(content)}`)
    }
  }

  beep () {
    const audioCtx = this.audioCtx;
    if (!audioCtx) return;

    let osc, gain;

    // main beep
    osc = audioCtx.createOscillator();
    gain = audioCtx.createGain();
    osc.connect(gain);
    gain.connect(audioCtx.destination);
    gain.gain.value = 0.5;
    osc.frequency.value = 750;
    osc.type = 'sine';
    osc.start();
    osc.stop(audioCtx.currentTime + 0.05);

    // surrogate beep (making it sound like 'oops')
    osc = audioCtx.createOscillator();
    gain = audioCtx.createGain();
    osc.connect(gain);
    gain.connect(audioCtx.destination);
    gain.gain.value = 0.2;
    osc.frequency.value = 400;
    osc.type = 'sine';
    osc.start(audioCtx.currentTime + 0.05);
    osc.stop(audioCtx.currentTime + 0.08);
  }

  static alphaToFraktur (character) {
    if ('a' <= character && character <= 'z') {
      character = String.fromCodePoint(0x1d51e - 0x61 + character.charCodeAt(0))
    } else if ('A' <= character && character <= 'Z') {
      character = frakturExceptions[character] || String.fromCodePoint(
        0x1d504 - 0x41 + character.charCodeAt(0))
    }
    return character
  }
}

const Screen = new TermScreen();

Screen.once('load', () => {
  qs('#screen').appendChild(Screen.canvas);
  for (let item of qs('#screen').classList) {
    if (item.startsWith('theme-')) {
      Screen.colors = themes[item.substr(6)]
    }
  }
});

let fitScreen = false
function fitScreenIfNeeded () {
  Screen.window.fitIntoWidth = fitScreen ? window.innerWidth : 0
  Screen.window.fitIntoHeight = fitScreen ? window.innerHeight : 0
}
fitScreenIfNeeded();
window.addEventListener('resize', fitScreenIfNeeded)

window.toggleFitScreen = function () {
  fitScreen = !fitScreen;
  const resizeButtonIcon = qs('#resize-button-icon')
  if (fitScreen) {
    resizeButtonIcon.classList.remove('icn-resize-small')
    resizeButtonIcon.classList.add('icn-resize-full')
  } else {
    resizeButtonIcon.classList.remove('icn-resize-full')
    resizeButtonIcon.classList.add('icn-resize-small')
  }
  fitScreenIfNeeded();
}
