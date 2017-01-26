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
