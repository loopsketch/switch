#pragma once

#include <Poco/Logger.h>
#include <Poco/Mutex.h>
#include <Poco/Thread.h>
#include <Poco/Runnable.h>
#include <Poco/format.h>

#include "FFBaseDecoder.h"
#include "PerformanceTimer.h"

//#ifndef _DEBUG
//#include <omp.h>
//#endif

using std::queue;


class VideoFrame
{
friend class FFVideoDecoder;
private:
	Poco::Logger& _log;

	Renderer& _renderer;
	int _frameNumber;
	int _ow;
	int _oh;
	int _w[3];
	int _h[3];
	LPDIRECT3DTEXTURE9 texture[3];
	LPD3DXEFFECT _fx;

	const Float toTexelU(const int pixel) {
		return F(pixel) / F(_ow);
	}

	const Float toTexelV(const int pixel) {
		return F(pixel) / F(_oh);
	}


public:
	VideoFrame(Renderer& renderer, const int w, const int h, const int linesize[], const D3DFORMAT format): _log(Poco::Logger::get("")), _renderer(renderer) {
		_ow = abs(linesize[0]) / 4;
		_oh = h;
		_w[0] = w;
		_h[0] = h;
		texture[0] = renderer.createTexture(_ow, _oh, format);
		texture[1] = NULL;
		texture[2] = NULL;
		_fx = NULL;
	}

	VideoFrame(Renderer& renderer, const int w, const int h, const int linesize[], const int h2, const D3DFORMAT format, const LPD3DXEFFECT fx):
		_log(Poco::Logger::get("")), _renderer(renderer), _fx(fx)
	{
		_ow = linesize[0];
		_oh = h;
		_w[0] = w;
		_h[0] = h;
		_w[1] = w / 2;
		_h[1] = h2;
		_w[2] = w / 2;
		_h[2] = h2;
		for (int i = 0; i < 3; i++) {
			texture[i] = renderer.createTexture(linesize[i], _h[i], format);
			// _log.information(Poco::format("texture: <%d>%dx%d", i, linesize[i], _h[i]));
		}
	}

	virtual ~VideoFrame() {
		SAFE_RELEASE(texture[0]);
		SAFE_RELEASE(texture[1]);
		SAFE_RELEASE(texture[2]);
	}

	const int frameNumber() const {
		return _frameNumber;
	}

	const int width() const {
		return _w[0];
	}

	const int height() const {
		return _h[0];
	}

	const bool equals(const int w, const int h, const D3DFORMAT format) {
		if (texture[0]) {
			D3DSURFACE_DESC desc;
			HRESULT hr = texture[0]->GetLevelDesc(0, &desc);
			if (SUCCEEDED(hr) && desc.Format == format && _ow == w && _oh == h) return true;
		}
		return false;
	}

	void write(const AVFrame* frame) {
		if (texture[1]) {
			// �v���i�[
			D3DLOCKED_RECT lockRect = {0};
			int i;
			//#ifndef _DEBUG
			//#pragma omp for private(i)
			//#endif
			for (i = 0; i < 3; i++) {
				if (texture[i]) {
					HRESULT hr = texture[i]->LockRect(0, &lockRect, NULL, 0);
					if (SUCCEEDED(hr)) {
						uint8_t* dst8 = (uint8_t*)lockRect.pBits;
						uint8_t* src8 = frame->data[i];
						CopyMemory(dst8, src8, lockRect.Pitch * _h[i]);
						hr = texture[i]->UnlockRect(0);
					} else {
						_log.warning(Poco::format("failed texture[%d] unlock", i));
					}
				}
			}

		} else {
			// �p�b�N�h
			D3DLOCKED_RECT lockRect = {0};
			HRESULT hr = texture[0]->LockRect(0, &lockRect, NULL, 0);
			if (SUCCEEDED(hr)) {
				uint8_t* dst8 = (uint8_t*)lockRect.pBits;
				uint8_t* src8 = frame->data[0];
				CopyMemory(dst8, src8, lockRect.Pitch * _h[0]);
				hr = texture[0]->UnlockRect(0);
			} else {
				_log.warning("failed lock texture");
			}
		}
		_frameNumber = frame->coded_picture_number;
	}

	void draw(const int x, const int y, int w = -1, int h = -1, int aspectMode = 0, DWORD col = 0xffffffff, int tx = 0, int ty = 0, int tw = -1, int th = -1) {
		if (w < 0) w = _w[0];
		if (h < 0) h = _h[0];
		int dx = 0;
		int dy = 0;
		switch (aspectMode) {
			case 0:
				break;

			case 1:
				// float srcZ =_w[0] / _h[0];

				if (w < _w[0]) {
					float z = F(w) / _w[0];
					float hh = L(_h[0] * z);
					dy = (h - hh) / 2;
					h = hh;
				} else if (h < _h[0]) {
					float z = F(h) / _h[0];
					float ww = L(_w[0] * z);
					dx = (w - ww) / 2;
					w = ww;
				} else {
					if (w > _w[0]) w = _w[0];
					if (h > _h[0]) h = _h[0];
				}
				break;
			default:
				w = _w[0];
				h = _h[0];
		}
		if (tw == -1) tw = _w[0];
		if (th == -1) th = _h[0];

		LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
		if (texture[1]) {
			// �v���i�[
			VERTEX dst[] =
				{
					{F(x     + dx - 0.5), F(y     + dy - 0.5), 0.0f, 1.0f, col, toTexelU(tx     ), toTexelV(ty     )},
					{F(x + w + dx - 0.5), F(y     + dy - 0.5), 0.0f, 1.0f, col, toTexelU(tx + tw), toTexelV(ty     )},
					{F(x     + dx - 0.5), F(y + h + dy - 0.5), 0.0f, 1.0f, col, toTexelU(tx     ), toTexelV(ty + th)},
					{F(x + w + dx - 0.5), F(y + h + dy - 0.5), 0.0f, 1.0f, col, toTexelU(tx + tw), toTexelV(ty + th)}
				};
			device->SetTexture(0, texture[0]);
			device->SetTexture(1, texture[1]);
			device->SetTexture(2, texture[2]);
			if (_fx) {
				_fx->SetTechnique("conversionTech");
				_fx->SetTexture("stage0", texture[0]);
				_fx->SetTexture("stage1", texture[1]);
				_fx->SetTexture("stage2", texture[2]);
				_fx->Begin(NULL, 0);
				_fx->BeginPass(0);
			}
			device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, dst, sizeof(VERTEX));
			if (_fx) {
				_fx->EndPass();
				_fx->End();
			}

			device->SetTexture(0, NULL);
			device->SetTexture(1, NULL);
			device->SetTexture(2, NULL);

		} else {
			// �p�b�N�h
			VERTEX dst[] =
				{
					{F(x     - 0.5), F(y     - 0.5), 0.0f, 1.0f, col, toTexelU(tx     ), toTexelV(ty     )},
					{F(x + w - 0.5), F(y     - 0.5), 0.0f, 1.0f, col, toTexelU(tx + tw), toTexelV(ty     )},
					{F(x     - 0.5), F(y + h - 0.5), 0.0f, 1.0f, col, toTexelU(tx     ), toTexelV(ty + th)},
					{F(x + w - 0.5), F(y + h - 0.5), 0.0f, 1.0f, col, toTexelU(tx + tw), toTexelV(ty + th)}
				};
			device->SetTexture(0, texture[0]);
			device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, dst, sizeof(VERTEX));
			device->SetTexture(0, NULL);
		}
		// _renderer.drawFontTextureText(0, 0, 12, 16, 0xffffffff, Poco::format("COLOR: %08lx", col));
	}
};
