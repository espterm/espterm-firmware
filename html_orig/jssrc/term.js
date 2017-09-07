/** Init the terminal sub-module - called from HTML */
window.termInit = function () {
	Conn.init();
	Input.init();
	TermUpl.init();
};
