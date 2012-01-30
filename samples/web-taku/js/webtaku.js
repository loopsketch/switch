var display = new Array();
display.push("192.168.1.106");
var root = "http://" + display[0] + ":9090/0/";

$(document).ready(function() {
	$("#error").hide();
	$("#message").hide();
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
				item.text(this["name"]);
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
		switchContent();
	});
	$("#error").hide();
	$("#message").text('操作開始できます.').fadeIn(1000);
});

function switchContent() {
	$.ajax({
		type: "GET",
		url: root + "switch",
		dataType: "jsonp",
		success: function(data) {
			if (data["switched"]) {
				$("#error").hide();
				$("#message").text('切替えました.').fadeIn(1000);
			} else {
				$("#message").hide();
				$("#error").text('切替えられません.').fadeIn(1000);
			}
		},
		error: function(request, status, ex) {
			$("#message").hide();
			$("#error").text("エラー発生: " + status).fadeIn(1000);
		}
	});
}

function setPlaylist(id) {
	$.ajax({
		type: "GET",
		url: root + "set/playlist",
		data: {pl: id},
		dataType: "jsonp",
		success: function(data) {
			if (data["playlist"]) {
				$("#error").hide();
				$("#message").text(data["name"] + "のプレイリストを準備しました.").fadeIn(1000);
			} else {
				$("#message").hide();
				$("#error").text('プレイリストを準備できません.').fadeIn(1000);
			}
		},
		error: function(request, status, ex) {
			$("#message").hide();
			$("#error").text("エラー発生: " + status).fadeIn(1000);
		}
	});
}
