// Some non-bold Fraktur symbols are outside the contiguous block
const frakturExceptions = {
  'C': '\u212d',
  'H': '\u210c',
  'I': '\u2111',
  'R': '\u211c',
  'Z': '\u2128'
}

// constants for decoding the update blob
const SEQ_SET_COLOR_ATTR = 1
const SEQ_REPEAT = 2
const SEQ_SET_COLOR = 3
const SEQ_SET_ATTR = 4

const SELECTION_BG = '#b2d7fe'
const SELECTION_FG = '#333'

const themes = [
  [ // Tango
    '#111213', '#CC0000', '#4E9A06', '#C4A000', '#3465A4', '#75507B', '#06989A',
    '#D3D7CF',
    '#555753', '#EF2929', '#8AE234', '#FCE94F', '#729FCF', '#AD7FA8', '#34E2E2',
    '#EEEEEC'
  ],
  [ // Linux
    '#000000', '#aa0000', '#00aa00', '#aa5500', '#0000aa', '#aa00aa', '#00aaaa',
    '#aaaaaa',
    '#555555', '#ff5555', '#55ff55', '#ffff55', '#5555ff', '#ff55ff', '#55ffff',
    '#ffffff'
  ],
  [ // xterm
    '#000000', '#cd0000', '#00cd00', '#cdcd00', '#0000ee', '#cd00cd', '#00cdcd',
    '#e5e5e5',
    '#7f7f7f', '#ff0000', '#00ff00', '#ffff00', '#5c5cff', '#ff00ff', '#00ffff',
    '#ffffff'
  ],
  [ // rxvt
    '#000000', '#cd0000', '#00cd00', '#cdcd00', '#0000cd', '#cd00cd', '#00cdcd',
    '#faebd7',
    '#404040', '#ff0000', '#00ff00', '#ffff00', '#0000ff', '#ff00ff', '#00ffff',
    '#ffffff'
  ],
  [ // Ambience
    '#2e3436', '#cc0000', '#4e9a06', '#c4a000', '#3465a4', '#75507b', '#06989a',
    '#d3d7cf',
    '#555753', '#ef2929', '#8ae234', '#fce94f', '#729fcf', '#ad7fa8', '#34e2e2',
    '#eeeeec'
  ],
  [ // Solarized
    '#073642', '#dc322f', '#859900', '#b58900', '#268bd2', '#d33682', '#2aa198',
    '#eee8d5',
    '#002b36', '#cb4b16', '#586e75', '#657b83', '#839496', '#6c71c4', '#93a1a1',
    '#fdf6e3'
  ]
]

class TermScreen {
  constructor () {
    this.canvas = document.createElement('canvas')
    this.ctx = this.canvas.getContext('2d')

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
      blinkInterval: null
    }

    this._colors = themes[0]

    this._window = {
      width: 0,
      height: 0,
      devicePixelRatio: 1,
      fontFamily: '"DejaVu Sans Mono", "Liberation Mono", "Inconsolata", ' +
        'monospace',
      fontSize: 20,
      gridScaleX: 1.0,
      gridScaleY: 1.2,
      blinkStyleOn: true,
      blinkInterval: null
    }
    this.windowState = {
      width: 0,
      height: 0,
      devicePixelRatio: 0,
      gridScaleX: 0,
      gridScaleY: 0,
      fontFamily: '',
      fontSize: 0
    }
    this.selection = {
      selectable: true,
      start: [0, 0],
      end: [0, 0]
    }

    this.mouseMode = { clicks: false, movement: false }

    const self = this
    this.window = new Proxy(this._window, {
      set (target, key, value, receiver) {
        target[key] = value
        self.updateSize()
        self.scheduleDraw()
        return true
      }
    })

    this.screen = []
    this.screenFG = []
    this.screenBG = []
    this.screenAttrs = []

    this.resetBlink()
    this.resetCursorBlink()

    let selecting = false
    this.canvas.addEventListener('mousedown', e => {
      if (this.selection.selectable || e.altKey) {
        let x = e.offsetX
        let y = e.offsetY
        selecting = true
        this.selection.start = this.selection.end = this.screenToGrid(x, y)
        this.scheduleDraw()
      } else {
        Input.onMouseDown(...this.screenToGrid(e.offsetX, e.offsetY),
          e.button + 1)
      }
    })
    window.addEventListener('mousemove', e => {
      if (selecting) {
        this.selection.end = this.screenToGrid(e.offsetX, e.offsetY)
        this.scheduleDraw()
      }
    })
    window.addEventListener('mouseup', e => {
      if (selecting) {
        selecting = false
        this.selection.end = this.screenToGrid(e.offsetX, e.offsetY)
        this.scheduleDraw()
        Object.assign(this.selection, this.getNormalizedSelection())
      }
    })
    this.canvas.addEventListener('mousemove', e => {
      if (!selecting) {
        Input.onMouseMove(...this.screenToGrid(e.offsetX, e.offsetY))
      }
    })
    this.canvas.addEventListener('mouseup', e => {
      if (!selecting) {
        Input.onMouseUp(...this.screenToGrid(e.offsetX, e.offsetY),
          e.button + 1)
      }
    })
    this.canvas.addEventListener('wheel', e => {
      if (this.mouseMode.clicks) {
        Input.onMouseWheel(...this.screenToGrid(e.offsetX, e.offsetY),
          e.deltaY > 0 ? 1 : -1)

        // prevent page scrolling
        e.preventDefault()
      }
    })

    // bind ctrl+shift+c to copy
    key('⌃+⇧+c', e => {
      e.preventDefault()
      this.copySelectionToClipboard()
    })
  }

  get colors () { return this._colors }
  set colors (theme) {
    this._colors = theme
    this.scheduleDraw()
  }

  // schedule a draw in the next tick
  scheduleDraw () {
    clearTimeout(this._scheduledDraw)
    this._scheduledDraw = setTimeout(() => this.draw(), 1)
  }

  getFont (modifiers = {}) {
    let fontStyle = modifiers.style || 'normal'
    let fontWeight = modifiers.weight || 'normal'
    return `${fontStyle} normal ${fontWeight} ${
      this.window.fontSize}px ${this.window.fontFamily}`
  }

  getCharSize () {
    this.ctx.font = this.getFont()

    return {
      width: this.ctx.measureText(' ').width,
      height: this.window.fontSize
    }
  }

  updateSize () {
    this._window.devicePixelRatio = window.devicePixelRatio || 1

    let didChange = false
    for (let key in this.windowState) {
      if (this.windowState[key] !== this.window[key]) {
        didChange = true
        this.windowState[key] = this.window[key]
      }
    }

    if (didChange) {
      const {
        width, height, devicePixelRatio, gridScaleX, gridScaleY, fontSize
      } = this.window
      const charSize = this.getCharSize()

      this.canvas.width = width * devicePixelRatio * charSize.width * gridScaleX
      this.canvas.style.width = `${Math.ceil(width * charSize.width *
        gridScaleX)}px`
      this.canvas.height = height * devicePixelRatio * charSize.height *
        gridScaleY
      this.canvas.style.height = `${Math.ceil(height * charSize.height *
        gridScaleY)}px`
    }
  }

  resetCursorBlink () {
    this.cursor.blinkOn = true
    clearInterval(this.cursor.blinkInterval)
    this.cursor.blinkInterval = setInterval(() => {
      this.cursor.blinkOn = !this.cursor.blinkOn
      this.scheduleDraw()
    }, 500)
  }

  resetBlink () {
    this.window.blinkStyleOn = true
    clearInterval(this.window.blinkInterval)
    let intervals = 0
    this.window.blinkInterval = setInterval(() => {
      intervals++
      if (intervals >= 4 && this.window.blinkStyleOn) {
        this.window.blinkStyleOn = false
        intervals = 0
      } else if (intervals >= 1 && !this.window.blinkStyleOn) {
        this.window.blinkStyleOn = true
        intervals = 0
      }
    }, 200)
  }

  getNormalizedSelection () {
    let { start, end } = this.selection
    // if the start line is after the end line, or if they're both on the same
    // line but the start column comes after the end column, swap
    if (start[1] > end[1] || (start[1] === end[1] && start[0] > end[0])) {
      [start, end] = [end, start]
    }
    return { start, end }
  }

  isInSelection (col, line) {
    let { start, end } = this.getNormalizedSelection()
    let colAfterStart = start[0] <= col
    let colBeforeEnd = col < end[0]
    let onStartLine = line === start[1]
    let onEndLine = line === end[1]

    if (onStartLine && onEndLine) return colAfterStart && colBeforeEnd
    else if (onStartLine) return colAfterStart
    else if (onEndLine) return colBeforeEnd
    else return start[1] < line && line < end[1]
  }

  getSelectedText () {
    const screenLength = this.window.width * this.window.height
    let lines = []
    let previousLineIndex = -1

    for (let cell = 0; cell < screenLength; cell++) {
      let x = cell % this.window.width
      let y = Math.floor(cell / this.window.width)

      if (this.isInSelection(x, y)) {
        if (previousLineIndex !== y) {
          previousLineIndex = y
          lines.push('')
        }
        lines[lines.length - 1] += this.screen[cell]
      }
    }

    return lines.join('\n')
  }

  copySelectionToClipboard () {
    let selectedText = this.getSelectedText()
    // don't copy anything if nothing is selected
    if (!selectedText) return
    let textarea = document.createElement('textarea')
    document.body.appendChild(textarea)
    textarea.value = selectedText
    textarea.select()
    if (document.execCommand('copy')) {
      Notify.show(`Copied to clipboard`)
    } else {
      // unsuccessful copy
      // TODO: do something?
    }
    document.body.removeChild(textarea)
  }

  screenToGrid (x, y) {
    let charSize = this.getCharSize()
    let cellWidth = charSize.width * this.window.gridScaleX
    let cellHeight = charSize.height * this.window.gridScaleY

    return [
      Math.floor((x + cellWidth / 2) / cellWidth),
      Math.floor(y / cellHeight)
    ]
  }

  drawCell ({ x, y, charSize, cellWidth, cellHeight, text, fg, bg, attrs },
      compositeAbove = false) {
    const ctx = this.ctx
    const inSelection = this.isInSelection(x, y)
    ctx.fillStyle = inSelection ? SELECTION_BG : this.colors[bg]
    if (!compositeAbove) ctx.globalCompositeOperation = 'destination-over'
    ctx.fillRect(x * cellWidth, y * cellHeight,
      Math.ceil(cellWidth), Math.ceil(cellHeight))
    ctx.globalCompositeOperation = 'source-over'

    let fontModifiers = {}
    let underline = false
    let blink = false
    let strike = false
    if (attrs & 1) fontModifiers.weight = 'bold'
    if (attrs & 1 << 1) ctx.globalAlpha = 0.5
    if (attrs & 1 << 2) fontModifiers.style = 'italic'
    if (attrs & 1 << 3) underline = true
    if (attrs & 1 << 4) blink = true
    if (attrs & 1 << 5) text = TermScreen.alphaToFraktur(text)
    if (attrs & 1 << 6) strike = true

    if (!blink || this.window.blinkStyleOn) {
      ctx.font = this.getFont(fontModifiers)
      ctx.fillStyle = inSelection ? SELECTION_FG : this.colors[fg]
      ctx.fillText(text, (x + 0.5) * cellWidth, (y + 0.5) * cellHeight)

      if (underline || strike) {
        let lineY = underline
          ? y * cellHeight + charSize.height
          : (y + 0.5) * cellHeight

        ctx.strokeStyle = inSelection ? SELECTION_FG : this.colors[fg]
        ctx.lineWidth = this.window.fontSize / 10
        ctx.lineCap = 'round'
        ctx.beginPath()
        ctx.moveTo(x * cellWidth, lineY)
        ctx.lineTo((x + 1) * cellWidth, lineY)
        ctx.stroke()
      }
    }

    ctx.globalAlpha = 1
  }

  draw () {
    const ctx = this.ctx
    const {
      width,
      height,
      devicePixelRatio,
      fontFamily,
      fontSize,
      gridScaleX,
      gridScaleY
    } = this.window

    const charSize = this.getCharSize()
    const cellWidth = charSize.width * gridScaleX
    const cellHeight = charSize.height * gridScaleY
    const screenWidth = width * cellWidth
    const screenHeight = height * cellHeight
    const screenLength = width * height

    ctx.setTransform(this.window.devicePixelRatio, 0, 0,
      this.window.devicePixelRatio, 0, 0)
    ctx.clearRect(0, 0, screenWidth, screenHeight)

    ctx.font = this.getFont()
    ctx.textAlign = 'center'
    ctx.textBaseline = 'middle'

    for (let cell = 0; cell < screenLength; cell++) {
      let x = cell % width
      let y = Math.floor(cell / width)
      let isCursor = this.cursor.x === x && this.cursor.y === y
      if (this.cursor.hanging) isCursor = false
      let invertForCursor = isCursor && this.cursor.blinkOn &&
        this.cursor.style === 'block'

      let text = this.screen[cell]
      let fg = invertForCursor ? this.screenBG[cell] : this.screenFG[cell]
      let bg = invertForCursor ? this.screenFG[cell] : this.screenBG[cell]
      let attrs = this.screenAttrs[cell]

      // HACK: ensure cursor is visible
      if (invertForCursor && fg === bg) bg = fg === 0 ? 7 : 0

      this.drawCell({
        x, y, charSize, cellWidth, cellHeight, text, fg, bg, attrs
      })

      if (isCursor && this.cursor.blinkOn && this.cursor.style !== 'block') {
        ctx.save()
        ctx.beginPath()
        if (this.cursor.style === 'bar') {
          // vertical bar
          let barWidth = 2
          ctx.rect(x * cellWidth, y * cellHeight, barWidth, cellHeight)
        } else if (this.cursor.style === 'line') {
          // underline
          let lineHeight = 2
          ctx.rect(x * cellWidth, y * cellHeight + charSize.height,
            cellWidth, lineHeight)
        }
        ctx.clip()

        // swap foreground/background
        fg = this.screenBG[cell]
        bg = this.screenFG[cell]
        // HACK: ensure cursor is visible
        if (fg === bg) bg = fg === 0 ? 7 : 0

        this.drawCell({
          x, y, charSize, cellWidth, cellHeight, text, fg, bg, attrs
        }, true)
        ctx.restore()
      }
    }
  }

  loadContent (str) {
    // current index
    let i = 0

    // window size
    this.window.height = parse2B(str, i)
    this.window.width = parse2B(str, i + 2)
    this.updateSize()
    i += 4

    // cursor position
    let [cursorY, cursorX] = [parse2B(str, i), parse2B(str, i + 2)]
    i += 4
    let cursorMoved = (cursorX !== this.cursor.x || cursorY !== this.cursor.y)
    this.cursor.x = cursorX
    this.cursor.y = cursorY

    if (cursorMoved) this.resetCursorBlink()

    // attributes
    let attributes = parse2B(str, i)
    i += 2

    this.cursor.visible = !!(attributes & 1)
    this.cursor.hanging = !!(attributes & 1 << 1)

    Input.setAlts(
      !!(attributes & 1 << 2), // cursors alt
      !!(attributes & 1 << 3), // numpad alt
      !!(attributes & 1 << 4)  // fn keys alt
    )

    let trackMouseClicks = !!(attributes & 1 << 5)
    let trackMouseMovement = !!(attributes & 1 << 6)

    Input.setMouseMode(trackMouseClicks, trackMouseMovement)
    this.selection.selectable = !trackMouseMovement
    this.mouseMode = {
      clicks: trackMouseClicks,
      movement: trackMouseMovement
    }

    let showButtons = !!(attributes & 1 << 7)
    let showConfigLinks = !!(attributes & 1 << 8)

    $('.x-term-conf-btn').toggleClass('hidden', !showConfigLinks)
    $('#action-buttons').toggleClass('hidden', !showButtons)

    // content
    let fg = 7
    let bg = 0
    let attrs = 0
    let cell = 0 // cell index
    let text = ' '
    let screenLength = this.window.width * this.window.height

    this.screen = new Array(screenLength).fill(' ')
    this.screenFG = new Array(screenLength).fill(' ')
    this.screenBG = new Array(screenLength).fill(' ')
    this.screenAttrs = new Array(screenLength).fill(' ')

    while (i < str.length && cell < screenLength) {
      let character = str[i++]
      let charCode = character.charCodeAt(0)

      if (charCode === SEQ_SET_COLOR_ATTR) {
        let data = parse3B(str, i)
        i += 3
        fg = data & 0xF
        bg = data >> 4 & 0xF
        attrs = data >> 8 & 0xFF
      } else if (charCode == SEQ_SET_COLOR) {
        let data = parse2B(str, i)
        i += 2
        fg = data & 0xF
        bg = data >> 4 & 0xF
      } else if (charCode === SEQ_SET_ATTR) {
        let data = parse2B(str, i)
        i += 2
        attrs = data & 0xFF
      } else if (charCode === SEQ_REPEAT) {
        let count = parse2B(str, i)
        i += 2
        for (let j = 0; j < count; j++) {
          this.screen[cell] = text
          this.screenFG[cell] = fg
          this.screenBG[cell] = bg
          this.screenAttrs[cell] = attrs

          if (++cell > screenLength) break
        }
      } else {
        // unique cell character
        this.screen[cell] = text = character
        this.screenFG[cell] = fg
        this.screenBG[cell] = bg
        this.screenAttrs[cell] = attrs
        cell++
      }
    }

    this.scheduleDraw()
    if (this.onload) this.onload()
  }

  /** Apply labels to buttons and screen title (leading T removed already) */
  loadLabels (str) {
    let pieces = str.split('\x01')
    qs('h1').textContent = pieces[0]
    $('#action-buttons button').forEach((button, i) => {
      var label = pieces[i + 1].trim()
      // if empty string, use the "dim" effect and put nbsp instead to
      // stretch the button vertically
      button.innerHTML = label ? e(label) : '&nbsp;';
      button.style.opacity = label ? 1 : 0.2;
    })
  }

  load (str) {
    const content = str.substr(1)

    switch (str[0]) {
      case 'S':
        this.loadContent(content)
        break
      case 'T':
        this.loadLabels(content)
        break
      case 'B':
        this.beep()
        break
      default:
        console.warn(`Bad data message type; ignoring.\n${
          JSON.stringify(content)}`)
    }
  }

  beep () {
    const audioCtx = this.audioCtx
    if (!audioCtx) return

    let osc, gain

    // main beep
    osc = audioCtx.createOscillator()
    gain = audioCtx.createGain()
    osc.connect(gain)
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

const Screen = new TermScreen()
let didAddScreen = false
Screen.onload = function () {
  if (didAddScreen) return
  didAddScreen = true
  qs('#screen').appendChild(Screen.canvas)
  for (let item of qs('#screen').classList) {
    if (item.startsWith('theme-')) Screen.colors = themes[item.substr(6)]
  }
}
