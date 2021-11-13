jQuery(document).ready(function () {
	var settingsModal = new bootstrap.Modal(document.getElementById('settingsModal'), {
	  keyboard: false
	});
	$('#settingsButton').click(function () {
		$.get( "/get_settings", function(data) {
			var response = JSON.parse(data);
			$('#tempRange').val(response.temperature);
			$("#tempRange" ).change();
			$('#timeoutRange').val(response.timeoutTime);
			$("#timeoutRange" ).change();
			$('#pumpTime').val(response.pumpTime);
			$("#pumpTime" ).change();
			$('#disablePumpCB').prop('checked', response.turnOffPump);
			$("#disablePumpCB" ).change();
			$('#offTemp').val(response.diffTemp);
			$("#offTemp" ).change();
			settingsModal.show();
		});
		return false;
	});
	$( "#tempRange" ).change(function() {
		$('#tempRangeLabel').html("Температура - " + $( "#tempRange" ).val() + "°C");
	});
	$( "#timeoutRange" ).change(function() {
		$('#timeoutRangeLabel').html("Таймаут - " + $( "#timeoutRange" ).val() + " минут");
	});
	$( "#pumpTime" ).change(function() {
		$('#pumpTimeLabel').html("Время работы насоса - " + $( "#pumpTime" ).val() + " минут");
	});
	$( "#disablePumpCB" ).change(function() {
		$('#offTemp').prop('disabled', !$('#disablePumpCB').prop('checked'));
	});
	$( "#offTemp" ).change(function() {
		$('#offTempLabel').html("Выключать насос если температура упадёт на " + $( "#offTemp" ).val() + "°C");
	});
	$('#saveBtn').click(function () {
		$('#saveBtn').html('<div class="spinner-border spinner-border-sm text-light" role="status"><span class="visually-hidden">Загрузка...</span></div>');
		$.get( "/put_settings", { pumpTime: $( "#pumpTime" ).val(), timeoutTime: $( "#timeoutRange" ).val(), temperature: $( "#tempRange" ).val(), turnOffPump: $('#disablePumpCB').prop('checked'), diffTemp: $( "#offTemp" ).val() }, function(data) {
			if(data == "ok"){
				$('#saveBtn').html('Сохранить');
				settingsModal.hide();
			}else location.reload();
		});
		return false;
	});
	var myVar = setInterval(myTimer, 1000);
	function myTimer() {
		var d = new Date();
		var t = d.toLocaleTimeString();
		$('#currentDate').html(t);
		$.get( "/get_state", function(data) {
			var response = JSON.parse(data);
			$('#tempLabel').html('Температура ' + response.temperature + '°C');
			$('#onTimeLabel').html('Насос включается на ' + response.pumpTime + ' минут.');
			if(response.pump_state){
				$('#pumpState').html('Насос включен.');
				$('#pumpState').addClass('active');
			}else{
				$('#pumpState').html('Насос выключен.');
				$('#pumpState').removeClass('active');
			}
			if(response.timeout_state){
				$('#timeoutState').html('Таймаут ' + response.timeoutTime + ' минут активен.');
				$('#timeoutState').addClass('active');
			}else{
				$('#timeoutState').html('Таймаут ' + response.timeoutTime + ' минут неактивен.');
				$('#timeoutState').removeClass('active');
			}
			//console.log(response);
			//location.reload();
		});
	}
	myTimer();
	setTimeout(function () {
		$('#alertMain').slideUp( 'slow' );
	}, 1000);
	$('#btn_reset_pump').click(function () {
		$.get( "/reset_pump", function(data) {
			$('#alertMain').html('Таймер насоса сброшен.');
			$('#alertMain').slideDown( 'slow' );
			setTimeout(function () {
				$('#alertMain').slideUp( 'slow' );
			}, 2000);
		  console.log(data);
		  //location.reload();
		});
		return false;
		});
	$('#btn_reset_timeout').click(function () {
		$.get( "/reset_timeout", function(data) {
			$('#alertMain').html('Таймаут сброшен.');
			$('#alertMain').slideDown( 'slow' );
			setTimeout(function () {
				$('#alertMain').slideUp( 'slow' );
			}, 2000);
			console.log(data);
			//location.reload();
		});
		return false;
	});
});