<div class="Box fold">
	<h2>Screen Behavior & Refreshing</h2>

	<div class="Row v">
		<p>
			The initial screen size, title text and button labels can be configured in <a href="<?= url('cfg_term') ?>">Terminal Settings</a>.
		</p>

		<p>
			Screen updates are sent to the browser through a WebSocket after some time of inactivity on the communication UART
			(called "Redraw Delay"). After an update is sent, at least a time of "Redraw Cooldown" must elapse before the next
			update can be sent. Those delays are used is to avoid burdening the server with tiny updates during a large screen
			repaint. If you experience issues (broken image due to dropped bytes), try adjusting those config options. It may also
			be useful to try different baud rates.
		</p>
	</div>
</div>
