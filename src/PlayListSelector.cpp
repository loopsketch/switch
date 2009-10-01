#include "PlayListSelector.h"

using Gdiplus::Color;


PlayListSelector::PlayListSelector(Renderer& renderer): Content(renderer), _playlist(-1), _item(-1), _oldPlaylist(0), _changePlaylist(-1), _changePlaylistItem(-1) {
}

PlayListSelector::~PlayListSelector() {
}


void PlayListSelector::update(WorkspacePtr workspace) {
	_workspace = workspace;
	_playlist = -1;
	_item = -1;
	_playlist = 0;
	_item = 0;
}

int PlayListSelector::getPlayList() {
	return _playlist;
}

int PlayListSelector::getPlayListItem() {
	return _item;
}

void PlayListSelector::nextPlayList() {
	if (_playlist >= 0) {
		_oldPlaylist = _playlist;
		_changePlaylist = 0;
		_moveSpeed = 5;
		_playlist = (_playlist + 1) % _workspace->getPlayListCount();
	}
	int max = _workspace->getPlayList(_playlist)->itemCount() - 1;
	if (_item < 0) {
		_item = 0;
	} else if (_item > max) {
		_item = max;
	}
}

void PlayListSelector::beforePlayList() {
	if (_playlist >= 0) {
		_oldPlaylist = _playlist;
		_changePlaylist = 0;
		_moveSpeed = 5;
		if (_playlist == 0) {
			_playlist = _workspace->getPlayListCount() - 1;
		} else {
			_playlist--;
		}
	}
	int max = _workspace->getPlayList(_playlist)->itemCount() - 1;
	if (_item < 0) {
		_item = 0;
	} else if (_item > max) {
		_item = max;
	}
}

void PlayListSelector::nextPlayListItem() {
	if (_playlist >= 0 && _item < _workspace->getPlayList(_playlist)->itemCount() - 1) {
		_item++;
	}
}

void PlayListSelector::beforePlayListItem() {
	if (_playlist >= 0 && _item > 0) {
		_item--;
	}
}


void PlayListSelector::draw(const DWORD& frame) {
	if (_playlist >= 0) {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		PlayListPtr oldPlaylist = _workspace->getPlayList(_oldPlaylist);
		LPDIRECT3DTEXTURE9 oldName = _renderer.getCachedTexture(oldPlaylist->name());
		PlayListPtr playlist = _workspace->getPlayList(_playlist);
		LPDIRECT3DTEXTURE9 name = _renderer.getCachedTexture(playlist->name());
		if (_changePlaylist >= 0) {
			int alpha1 = 255 * (30 - _changePlaylist) / 30;
			int alpha2 = 255 * _changePlaylist / 30;
			if (_oldPlaylist < _playlist) {
				D3DSURFACE_DESC desc;
				HRESULT hr = oldName->GetLevelDesc(0, &desc);
				Uint32 tw = desc.Width;
				Uint32 th = desc.Height;
				D3DCOLOR col = alpha1 << 24 | 0xffffff;
				_renderer.drawTexture(700      - F(tw * _changePlaylist / 30), 380, oldName, col, col, col, col);
				col = alpha2 << 24 | 0xffffff;
				_renderer.drawTexture(700 + tw - F(tw * _changePlaylist / 30), 380, name, col, col, col, col);
			} else {
				D3DSURFACE_DESC desc;
				HRESULT hr = name->GetLevelDesc(0, &desc);
				Uint32 tw = desc.Width;
				Uint32 th = desc.Height;
				D3DCOLOR col = alpha1 << 24 | 0xffffff;
				_renderer.drawTexture(700      + F(tw * _changePlaylist / 30), 380, oldName, col, col, col, col);
				col = alpha2 << 24 | 0xffffff;
				_renderer.drawTexture(700 - tw + F(tw * _changePlaylist / 30), 380, name, col, col, col, col);
			}
			_changePlaylist += _moveSpeed;
			_moveSpeed /= 1.185;
			if (_changePlaylist > 30) _changePlaylist = -1;
		} else {
			D3DCOLOR col = 0xffffffff;
			_renderer.drawTexture(700, 380, name, col, col, col, col);
		}

		VERTEX p1[2] = {
			{F( 700), F(405), F(0), F(1), D3DCOLOR(0x99ffffff), F(0), F(0)},
			{F(1020), F(405), F(0), F(1), D3DCOLOR(0x33ffffff), F(0), F(0)}
		};
		_renderer.get3DDevice()->DrawPrimitiveUP(D3DPT_LINELIST, 1, p1, sizeof(VERTEX));
		VERTEX p2[2] = {
			{F( 700), F(515), F(0), F(1), D3DCOLOR(0x99ffffff), F(0), F(0)},
			{F(1020), F(515), F(0), F(1), D3DCOLOR(0x33ffffff), F(0), F(0)}
		};
		_renderer.get3DDevice()->DrawPrimitiveUP(D3DPT_LINELIST, 1, p2, sizeof(VERTEX));

		const int alpha[] = {0x66, 0x99, 0xff, 0x99, 0x66};
		for (int i = - 2; i <= 2; i++) {
			if ((_item + i) >= 0 && (_item + i) < playlist->itemCount()) {
				LPDIRECT3DTEXTURE9 texture = _renderer.getCachedTexture(playlist->items()[_item + i]->media()->id());
				if (texture) {
					int x = 700;
					int y = 450 + i * 20;
					if (i == 0) {
						float size = 1 + 3 * abs(sin(PI2 * (frame % 360) / F(360)));
						for (float j = - size; j <= size; j+=0.4) {
							D3DCOLOR col = L(0x33 * (size - abs(j)) / size) << 24 | 0xccffcc;
							_renderer.drawTexture(x + j, y + j, texture, col, col, col, col);
							_renderer.drawTexture(x - j, y + j, texture, col, col, col, col);
							_renderer.drawTexture(x + j, y, texture, col, col, col, col);
							_renderer.drawTexture(x, y + j, texture, col, col, col, col);
						}
					}
					D3DCOLOR col = alpha[i + 2] << 24 | 0xffffff;
					_renderer.drawTexture(x, y, texture, col, col, col, col);
				}
			}
		}
	}
}
