var display = new Array();
display.push("192.168.1.50");
var root = "http://" + display[0] + ":9090/0/";

var _workspace;

$(document).ready(function() {
	$('#error').hide();
	$('#message').hide();
	$('#playlist-selector').selectable({
		autoRefresh: true,
		tolerance: 'fit',
		stop: function() {
			setPlaylist($('.ui-selected').attr('playlist-id'));
		}
	});

	$('#doSwitch').click(function() {
		switchContent();
	});

	getWorkspace();
});


/** ワークスペース取得 */
function getWorkspace() {
	$.ajax({
		type: 'GET',
		url: root + 'download?path=workspace.xml',
		dataType: 'xml',
		cache: false,
		async: true,
		success: function(data) {
			_workspace = data;
			setupPlaylist();
			setupPreview();
			getStatus();
			message('操作開始できます.');
		},
		error: function(request, status, ex) {
			message('failed not update workspace: ' + status);
		}
	});
}

/** プレイリストのセットアップ */
function setupPlaylist() {
	$(_workspace).find('playlist').each(function() {
		var item = $('<div>').addClass('ui-widget-content');

		item.attr('playlist-id', $(this).attr('id'));
		item.text($(this).attr('name'));
		//item.click(function() {
		//	var id = $(this).attr('playlist-id');
		//	setPlaylist(id);
		//});
		//$(item).bind('selectstart', function() {return false;});
		$('#playlist-selector').append(item);
		//console.log($(this).attr('name'));
	});
}

/** プレビュー更新のセットアップ */
function setupPreview() {
	$('#preview').load(function() {
		setTimeout(updatePreview, 4000);
	});
	updatePreview();
}

//{"action":"","brightness":"100","current-content":"NHK進化論","current-content-id":"m00012","current-index":"4","current-playlist":"通常リスト","current-playlist-id":"game","next-content":"cristmas tree","next-content-id":"m00030","next-playlist":"通常リスト","next-playlist-id":"game","prepared-content-id":"m00032","stage-description":"","stage-name":"entrance","time_current":"00:01:19.23","time_remain":"00:01:10.22","transition":"","workspace":"15d9129e93a904875c0958cd56cec452"}

/** プレビュー更新 */
function updatePreview() {
	$('#preview').attr('src', root + 'get/snapshot?t=' + new Date().getTime());
}

/** プレビュー更新のセットアップ */
function getStatus() {
	$.ajax({
		type: 'GET',
		url: root + 'get/status',
		dataType: 'jsonp',
		cache: false,
		async: true,
		success: function(data) {
			$('#current-playlist').text(data['current-playlist']);
			$('#current-content').text(data['current-content']);
			$('#next-playlist').text(data['next-playlist']);
			var next = data['next-content'];
			$('#next-content').text(next);
			$('#prepared-playlist').text(data['prepared-playlist']);
			var prepared = data['prepared-content'];
			if (prepared) {
				$('#prepared-content').text(prepared);
			} else {
				$('#prepared-playlist').text('');
				$('#prepared-content').text('');
			}
			if (next || prepared) {
				$('#doSwitch').removeAttr("disabled");
			} else {
				$('#doSwitch').attr("disabled", "disabled");
			}
			setTimeout(getStatus, 500);
		},
		error: function(request, status, ex) {
			message('エラー発生: ' + status);
		}
	});
}

/** プレイリスト準備 */
function setPlaylist(id) {
	$.ajax({
		type: "GET",
		url: root + "set/playlist",
		data: {pl: id},
		dataType: "jsonp",
		cache: false,
		async: true,
		success: function(data) {
			if (data['playlist']) {
				message('プレイリストを準備.');
			} else {
				error('プレイリストを準備できません.');
			}
		},
		error: function(request, status, ex) {
			error('エラー発生: ' + status);
		}
	});
}

/** 切替 */
function switchContent() {
	$.ajax({
		type: "GET",
		url: root + "switch",
		dataType: "jsonp",
		success: function(data) {
			if (data["switched"]) {
				message('切替えました.');
			} else {
				error('切替えられません.');
			}
		},
		error: function(request, status, ex) {
			error('エラー発生: ' + status);
		}
	});
}


/** */
function message(text) {
	$('#error').hide();
	$('#message').text(text).fadeIn(1000);
}

/** */
function error(text) {
	$('#message').hide();
	$('#error').text(text).fadeIn(1000);
}
