var display = new Array();
display.push("192.168.1.50");
var root = '/0/';

var _workspace;
var _workspaceSignature;
var _selectPlaylist;


$(document).ready(function() {
	$('#error').hide();
	$('#message').hide();
	$('#playlist-selector').selectable({
		autoRefresh: true,
		tolerance: 'fit',
		stop: function() {
			_selectPlaylist = $('#playlist-selector .ui-selected').attr('playlist-id');
			setPlaylist(_selectPlaylist);
		}
	});
	$('#playlist-item-selector').selectable({
		autoRefresh: true,
		tolerance: 'fit',
		stop: function() {
			setPlaylistItem($('#playlist-item-selector div').index($('#playlist-item-selector .ui-selected')));
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
			message('操作開始できます.');
			_workspace = data;
			_workspaceSignature = null;
			setupPlaylist();
			setupPreview();
			getStatus();
		},
		error: function(request, status, ex) {
			message('failed not update workspace: ' + status);
		}
	});
}

/** プレイリストのセットアップ */
function setupPlaylist() {
	$('#playlist-selector').text('');
	$(_workspace).find('playlist').each(function() {
		var item = $('<div>').addClass('ui-widget-content');
		item.attr('playlist-id', $(this).attr('id'));
		item.text($(this).attr('name'));
		$('#playlist-selector').append(item);
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
			var workspace = data['workspace'];
			if (_workspaceSignature == null) {
				_workspaceSignature = workspace;
				message('workspace更新しました.');
			} else if (_workspaceSignature != workspace) {
				getWorkspace();
			}
			$('#stage-name').text('■' + data['stage-name']);
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
	//console.log(id);
	$.ajax({
		type: "GET",
		url: root + "set/playlist",
		data: {pl: id},
		dataType: "jsonp",
		cache: false,
		async: true,
		success: function(data) {
			if (data['playlist']) {
				//message('プレイリストを準備.');
				setupPlaylistItems(id);
			} else {
				error('プレイリストを準備できません.');
			}
		},
		error: function(request, status, ex) {
			error('エラー発生: ' + status);
		}
	});
}

/** プレイリストアイテム準備 */
function setPlaylistItem(i) {
	//console.log(_selectPlaylist + '-' + i);
	$.ajax({
		type: "GET",
		url: root + "set/playlist",
		data: {pl: _selectPlaylist, i: i},
		dataType: "jsonp",
		cache: false,
		async: true,
		success: function(data) {
			if (data['playlist']) {
				//message('プレイリストを準備.');
			} else {
				error('プレイリストを準備できません.');
			}
		},
		error: function(request, status, ex) {
			error('エラー発生: ' + status);
		}
	});
}

/** プレイリストのセットアップ */
function setupPlaylistItems(id) {
	$('#playlist-item-selector').text('');
	$(_workspace).find('playlist[id=' + id + ']>item').each(function() {
		var mediaID = $(this).text();
		$(_workspace).find('medialist>item[id=' + mediaID + ']').each(function() {
			var item = $('<div>').addClass('ui-widget-content');
			item.text($(this).attr('name'));
			$('#playlist-item-selector').append(item);
		});
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
