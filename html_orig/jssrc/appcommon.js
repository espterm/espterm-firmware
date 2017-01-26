/** Global generic init */
$.ready(function () {
	// loader dots...
	setInterval(function () {
		$('.anim-dots').each(function (x) {
			var $x = $(x);
			var dots = $x.html() + '.';
			if (dots.length == 5) dots = '.';
			$x.html(dots);
		});
	}, 1000);

	$('input[type=number]').on('mousewheel', function(e) {
		var val = +$(this).val();
		if (isNaN(val)) val = 1;

		var step = +($(this).attr('step') || 1);
		var min = +$(this).attr('min');
		var max = +$(this).attr('max');
		if(e.wheelDelta > 0) {
			val += step;
		} else {
			val -= step;
		}

		if (typeof min != 'undefined') val = Math.max(val, +min);
		if (typeof max != 'undefined') val = Math.min(val, +max);
		$(this).val(val);

		if ("createEvent" in document) {
			var evt = document.createEvent("HTMLEvents");
			evt.initEvent("change", false, true);
			$(this)[0].dispatchEvent(evt);
		} else {
			$(this)[0].fireEvent("onchange");
		}

		e.preventDefault();
	});
});

$._loader = function(vis) {
	console.log("loader fn", vis);
	if(vis)
		$('#loader').addClass('show');
	else
		$('#loader').removeClass('show');
};
