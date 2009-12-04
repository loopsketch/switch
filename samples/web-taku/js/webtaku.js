var root = "http://" + display[0] + ":9090/";

$(document).ready(function() {
	$.ajax({
		type: "GET",
		url: root + "get/playlist",
		dataType: "jsonp",
		cache: false,
		async: false,
		success: function(data) {
			$.each(data["playlists"], function() {
				var id = this["id"];
				var item = $("<div>").addClass("playlist-item");
				item.attr("playlist-id", this["id"]);
				item.text("プレイリスト(" + this["id"] + ")");
				item.click(function() {
					setPlaylist(id);
					$("div[playlist-id]").removeClass("playlist-item-selected");
					item.addClass("playlist-item-selected");
				});
				$("#playlist-selector").append(item);
			});
		}
	});

	$("#doSwitch").click(function() {
		$.get(root + 'switch', {}, function(data) {
			$("#message").text('切替えました.');
		});
	});
	$("#message").text('操作開始できます.');
});

function setPlaylist(id) {
	$.ajax({
		type: "GET",
		url: root + "set/playlist",
		data: {pl: id},
		dataType: "jsonp",
		success: function(data) {
			$("#message").text(data["playlist"] + "に設定しました.");
		},
		error: function(request, status, ex) {
			$("#message").text("エラー発生: " + status);
		}
	});
}
