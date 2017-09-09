/** Handle connections */
var Conn = (function () {
  var ws;
  var heartbeatTout;
  var pingIv;
  var xoff = false;
  var autoXoffTout;
  var reconTout;

  var pageShown = false;

  function onOpen(evt) {
    console.log("CONNECTED");
    heartbeat();
    doSend("i");
  }

  function onClose(evt) {
    console.warn("SOCKET CLOSED, code " + evt.code + ". Reconnecting...");
    clearTimeout(reconTout);
    reconTout = setTimeout(function () {
      init();
    }, 2000);
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
          if(!pageShown) {
            showPage();
            pageShown = true;
          }
          break;

        case '-':
          //console.log('xoff');
          xoff = true;
          autoXoffTout = setTimeout(function () {
            xoff = false;
          }, 250);
          break;

        case '+':
          //console.log('xon');
          xoff = false;
          clearTimeout(autoXoffTout);
          break;
      }
      heartbeat();
    } catch (e) {
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

    clearTimeout(reconTout);
    clearTimeout(heartbeatTout);

    ws = new WebSocket("ws://" + _root + "/term/update.ws");
    ws.onopen = onOpen;
    ws.onclose = onClose;
    ws.onmessage = onMessage;
    console.log("Opening socket.");
    heartbeat();
  }

  function heartbeat() {
    clearTimeout(heartbeatTout);
    heartbeatTout = setTimeout(heartbeatFail, 2000);
  }

  function heartbeatFail() {
    console.error("Heartbeat lost, probing server...");
    pingIv = setInterval(function () {
      console.log("> ping");
      $.get('http://' + _root + '/system/ping', function (resp, status) {
        if (status == 200) {
          clearInterval(pingIv);
          console.info("Server ready, reloading page...");
          location.reload();
        }
      }, {
        timeout: 100,
      });
    }, 1000);
  }

  return {
    ws: null,
    init: init,
    send: doSend,
    canSend: canSend, // check flood control
  };
})();
