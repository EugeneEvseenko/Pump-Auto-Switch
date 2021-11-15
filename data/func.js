jQuery(document).ready(function () {
	var settingsModal = new bootstrap.Modal(document.getElementById('settingsModal'), {
	  keyboard: false
	});
	$('#settingsButton').click(function () {
		$('#settingsButton').html('<div class="spinner-border spinner-border-sm text-danger" role="status"><span class="visually-hidden">Загрузка...</span></div>');
		$('#settingsButton').prop('disabled', true);
		$.get( "/get_settings", function(response) {
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
			$('#settingsButton').html('Настройки');
			$('#settingsButton').prop('disabled', false);
		});
		return false;
	});
	$(document).on('input change', '#tempRange', function() {
		$('#tempRangeLabel').html("Температура - " + $( "#tempRange" ).val() + "°C");
	});
	$(document).on('input change', '#timeoutRange', function() {
		$('#timeoutRangeLabel').html("Таймаут - " + $( "#timeoutRange" ).val() + declOfNum($( "#timeoutRange" ).val(), [' минута',' минуты',' минут']));
	});
	$(document).on('input change', '#pumpTime', function() {
		$('#pumpTimeLabel').html("Время работы насоса - " + $( "#pumpTime" ).val() + declOfNum($( "#pumpTime" ).val(), [' минута',' минуты',' минут']));
	});
	$( "#disablePumpCB" ).change(function() {
		$('#offTemp').prop('disabled', !$('#disablePumpCB').prop('checked'));
	});
	$(document).on('input change', '#offTemp', function() {
		$('#offTempLabel').html("Отключать насос если температура упадёт на " + $( "#offTemp" ).val() + "°C");
	});
	$('#saveBtn').click(function () {
		$('#saveBtn').html('<div class="spinner-border spinner-border-sm text-light" role="status"><span class="visually-hidden">Загрузка...</span></div>');
		$.post( "/put_settings", { pumpTime: $( "#pumpTime" ).val(), timeoutTime: $( "#timeoutRange" ).val(), temperature: $( "#tempRange" ).val(), turnOffPump: $('#disablePumpCB').prop('checked'), diffTemp: $( "#offTemp" ).val() }, function(data) {
			if(data == "ok"){
				$('#saveBtn').html('Сохранить');
				settingsModal.hide();
			}else location.reload();
		});
		return false;
	});
	var myVar = setInterval(myTimer, 1000);
	function myTimer() {
		$.get( "/get_state", function(response) {
			$('#rssiProgress').css('width', response.rssi + "%");
			$('#rssiProgress').html("Качество связи " + response.rssi + "%");
			$('#tempLabel').html('Температура ' + response.temperature + '°C');
			if(response.pump_state){
				$('#pumpState').html('Насос включен.<br>До конца ' + millisToMinutesAndSeconds(response.pumpLeft) + '.');
				$('#pumpState').addClass('active');
			}else{
				$('#pumpState').html('Насос выключен.');
				$('#pumpState').removeClass('active');
			}
			if(response.timeout_state){
				$('#timeoutState').html('Таймаут активен.<br>До конца ' + millisToMinutesAndSeconds(response.timeoutLeft) + '.');
				$('#timeoutState').addClass('active');
			}else{
				$('#timeoutState').html('Таймаут неактивен.');
				$('#timeoutState').removeClass('active');
			}
		});
	}
	myTimer();
	setTimeout(function () {
		$('#alertMain').slideUp( 'slow' );
	}, 1000);
	$('#btn_reset_pump').click(function () {
		$('#btn_reset_pump').html('<div class="spinner-border spinner-border-sm text-danger" role="status"><span class="visually-hidden">Загрузка...</span></div>');
		$('#btn_reset_pump').prop('disabled', true);
		$.get( "/reset_pump", function(data) {
			$('#alertMain').html('Таймер насоса сброшен.');
			$('#alertMain').slideDown( 'slow' );
			setTimeout(function () {
				$('#alertMain').slideUp( 'slow' );
			}, 2000);
			$('#btn_reset_pump').html('Сброс таймера насоса');
			$('#btn_reset_pump').prop('disabled', false);
		});
		return false;
		});
	$('#btn_reset_timeout').click(function () {
		$('#btn_reset_timeout').html('<div class="spinner-border spinner-border-sm text-danger" role="status"><span class="visually-hidden">Загрузка...</span></div>');
		$('#btn_reset_timeout').prop('disabled', true);
		$.get( "/reset_timeout", function(data) {
			$('#alertMain').html('Таймаут сброшен.');
			$('#alertMain').slideDown( 'slow' );
			setTimeout(function () {
				$('#alertMain').slideUp( 'slow' );
			}, 2000);
			$('#btn_reset_timeout').html('Сброс таймера таймаута');
			$('#btn_reset_timeout').prop('disabled', false);
		});
		return false;
	});
	function declOfNum(number, titles) {
		cases = [2, 0, 1, 1, 1, 2];
		return titles[ (number%100>4 && number%100<20)? 2 : cases[(number%10<5)?number%10:5] ];
	}
	function millisToMinutesAndSeconds(millis) {
		var seconds = millis / 1000;
		if (seconds >= 60){
			var minutes = Math.floor(seconds / 60) + 1;
			return minutes + declOfNum(minutes, [' минута',' минуты',' минут']);
		}else{
			var seconds = (seconds % 60).toFixed(0);
			return seconds + declOfNum(seconds, [' секунда',' секунды',' секунд']);
		}
	}
});