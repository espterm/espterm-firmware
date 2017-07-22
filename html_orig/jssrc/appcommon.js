/** Global generic init */
$.ready(function () {
	// Checkbox UI (checkbox CSS and hidden input with int value)
	$('.Row.checkbox').forEach(function(x) {
		var inp = x.querySelector('input');
		var box = x.querySelector('.box');

		$(box).toggleClass('checked', inp.value);

		$(x).on('click', function() {
			inp.value = 1 - inp.value;
			$(box).toggleClass('checked', inp.value)
		});
	});

	// Expanding boxes on mobile
	$('.Box.mobcol').forEach(function(x) {
		var h = x.querySelector('h2');
		$(h).on('click', function() {
			$(x).toggleClass('expanded');
		});
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

	Modal.init();
});

$._loader = function(vis) {
	$('#loader').toggleClass('show', vis);
};
