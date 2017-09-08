$.ready(() => {
  const input = qs('#softkb-input')
  let keyboardOpen = false

  let updateInputPosition = function () {
    if (!keyboardOpen) return

    let [x, y] = Screen.gridToScreen(Screen.cursor.x, Screen.cursor.y)
    input.style.transform = `translate(${x}px, ${y}px)`
  }

  input.addEventListener('focus', () => {
    keyboardOpen = true
    updateInputPosition()
  })
  input.addEventListener('blur', () => (keyboardOpen = false))
  Screen.on('cursor-moved', updateInputPosition)

  window.kbOpen = function openSoftKeyboard (open) {
    keyboardOpen = open
    updateInputPosition()
    if (open) input.focus()
    else input.blur()
  }

  input.addEventListener('keydown', e => {
    if (e.key === 'Backspace') {
      e.preventDefault()
      e.stopPropagation()
      Input.sendString('\b')
    }
  })
  input.addEventListener('keypress', e => {
    e.stopPropagation()
  })
})
