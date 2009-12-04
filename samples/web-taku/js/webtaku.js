$(document).ready(function() {
	var root = "http://" + display[0] + ":9090/";
	$.ajax({
		type: "GET",
		url: root + "get/playlist",
		dataType: "jsonp",
		cache: false,
		async: false,
		success: function(data) {
			$.each(data["playlists"], function() {
				$("#message").append(this["id"] + "<br>");
			});
		}
	});

	$("#setPlaylist").click(function() {
		var playlist = $("#playlist-id").val();
		$.ajax({
			type: "GET",
			url: root + "set/playlist",
			data: {pl: playlist},
			dataType: "jsonp",
			success: function(data) {
				$("#message").text(data["playlist"] + "に設定しました.");
			},
			error: function(request, status, ex) {
				$("#message").text("エラー発生: " + status);
			}
		});
	});

	$("#doSwitch").click(function() {
		$.get(root + 'switch', {}, function(data) {
			$("#message").text('切替えました.');
		});
	});
	$("#message").text('操作開始できます.');
});
