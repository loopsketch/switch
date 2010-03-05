#include <Poco/DateTime.h>
#include <Poco/Timezone.h>
#include <Poco/Mutex.h>
#include <Poco/Thread.h>
#include <Poco/Runnable.h>
#include <Poco/Util/XMLConfiguration.h>
#include <Poco/UnicodeConverter.h>

#include "CaptureContent.h"

using Poco::Util::XMLConfiguration;


CaptureContent::CaptureContent(Renderer& renderer): Content(renderer), 
	_scene(NULL), _fx(NULL), _small1(NULL), _small2(NULL), _diff(NULL), _diff2(NULL), _image(NULL),
	_detected(false), _doShutter(0), _viewPhoto(0), _finished(true), _playing(false), _statusFrame(0)
{
	initialize();
	_scene = (CaptureScenePtr)_renderer.getScene("capture");
}

CaptureContent::~CaptureContent() {
	initialize();
}

void CaptureContent::saveConfiguration() {
	try {
		Poco::Util::XMLConfiguration* xml = new Poco::Util::XMLConfiguration("cvcap-config.xml");
		if (xml) {
			xml->setInt("detect", _detectThreshold);
			xml->setInt("intervals[@diff]", _intervalDiff);
			xml->setInt("intervals[@small]", _intervalSmall);
//			xml->setInt("clip[@x]", _clipX);
//			xml->setInt("clip[@y]", _clipY);
//			xml->setInt("clip[@w]", _clipW);
//			xml->setInt("clip[@h]", _clipH);
			xml->save("cv-config.xml");
			xml->release();
			_log.information("cv parameters saved");
		}
	} catch (Poco::Exception& ex) {
		_log.warning(ex.displayText());
	}
}

void CaptureContent::initialize() {
	close();
}

/** ファイルをオープンします */
bool CaptureContent::open(const MediaItemPtr media, const int offset) {
	initialize();

	int deviceW = 0;
	int deviceH = 0;
	try {
		XMLConfiguration* xml = new XMLConfiguration("cvcap-config.xml");
//		_deviceNo = xml->getInt("device[@no]", 0);
		deviceW = xml->getInt("device[@w]", 640);
		deviceH = xml->getInt("device[@h]", 480);
//		_deviceFPS = xml->getInt("device[@fps]", 60);
		_subtract = xml->getDouble("subtract", 0.5);
//		_samples = xml->getInt("samples", 4);
		_detectThreshold = xml->getInt("detect", 50);
		_intervalDiff = xml->getInt("intervals[@diff]", 60);
		_intervalSmall = xml->getInt("intervals[@small]", 12);
		string file = xml->getString("image", "");
		if (!file.empty()) {
			_image = _renderer.createTexture(file);
		}
//		_clipX = xml->getInt("clip[@x]", 0);
//		_clipY = xml->getInt("clip[@y]", 0);
//		_clipW = xml->getInt("clip[@w]", deviceW);
//		_clipH = xml->getInt("clip[@h]", deviceH);
		xml->release();
	} catch (Poco::Exception& ex) {
		_log.warning(ex.displayText());
	}

	std::wstring wfile;
	Poco::UnicodeConverter::toUTF16(string("subbg.fx"), wfile);
	LPD3DXBUFFER errors = NULL;
	HRESULT hr = D3DXCreateEffectFromFile(_renderer.get3DDevice(), wfile.c_str(), 0, 0, D3DXSHADER_DEBUG, 0, &_fx, &errors);
	if (errors) {
		std::vector<char> text(errors->GetBufferSize());
		memcpy(&text[0], errors->GetBufferPointer(), errors->GetBufferSize());
		text.push_back('\0');
		_log.warning(Poco::format("shader compile error: %s", string(&text[0])));
		SAFE_RELEASE(errors);
	} else if (FAILED(hr)) {
		_log.warning(Poco::format("failed shader: %s", string("")));
	}

	_small1 = _renderer.createRenderTarget(32, 32, D3DFMT_A8R8G8B8);
	_renderer.colorFill(_small1, 0);
	_small2 = _renderer.createRenderTarget(32, 32, D3DFMT_A8R8G8B8);
	_renderer.colorFill(_small2, 0);
	_diff = _renderer.createRenderTarget(32, 32, D3DFMT_A8R8G8B8);
	_diff2 = _renderer.createLockableSurface(32, 32, D3DFMT_A8R8G8B8);

	set("alpha", 1.0f);
	_duration = media->duration() * 60 / 1000;
	_current = 0;
	_mediaID = media->id();
	return true;
}


/**
 * 再生
 */
void CaptureContent::play() {
	_playing = true;
	_playTimer.start();
}

/**
 * 停止
 */
void CaptureContent::stop() {
	_playing = false;
}

bool CaptureContent::useFastStop() {
	return true;
}

/**
 * 再生中かどうか
 */
const bool CaptureContent::playing() const {
	return _playing;
}

const bool CaptureContent::finished() {
	return _current >= _duration;
}

/** ファイルをクローズします */
void CaptureContent::close() {
	stop();
	_mediaID.clear();

	{
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		SAFE_RELEASE(_fx);
		SAFE_RELEASE(_small1);
		SAFE_RELEASE(_small2);
		SAFE_RELEASE(_diff);
		SAFE_RELEASE(_diff2);
		SAFE_RELEASE(_image);
	}
}

void CaptureContent::process(const DWORD& frame) {
	if (_playing) {
		if (_keycode != 0) {
			switch (_keycode) {
				case '8':
					_detectThreshold--;
					break;
				case '9':
					_detectThreshold++;
					break;

				case '0':
					_intervalDiff--;
					break;
				case VK_OEM_MINUS:
					_intervalDiff++;
					break;

				case VK_OEM_7:
					_intervalSmall--;
					break;
				case VK_OEM_5:
					_intervalSmall++;
					break;

				case 'S':
					saveConfiguration();
					_statusFrame = frame;
					_status = "PARAMETER SAVED.";
					break;

				default:
					_log.information(Poco::format("key: %?x", _keycode));
			}
		}

		_current++;
	}
}

void CaptureContent::draw(const DWORD& frame) {
	if (!_mediaID.empty() && _playing) {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		int x = config().stageRect.left;
		int y = config().stageRect.top;
		int w = config().stageRect.right;
		int h = config().stageRect.bottom;

		LPDIRECT3DTEXTURE9 cameraImage = _scene->getCameraImage();
		if (!cameraImage) return;

		DWORD col = 0xffffffff;
		_renderer.drawTexture(x, y, w, h, cameraImage, 0, col, col, col, col);
		if (_image) _renderer.drawTexture(x, y, _image, 0, col, col, col, col);
		LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
		if (_fx) {
			LPDIRECT3DSURFACE9 orgRT;
			HRESULT hr = device->GetRenderTarget(0, &orgRT);

			// 単純化
			col = 0xffffffff;
			D3DSURFACE_DESC desc;
			hr = _small1->GetLevelDesc(0, &desc);
			VERTEX dst[] =
			{
				{F(0              - 0.5), F(0               - 0.5), 0.0f, 1.0f, col, 0, 0},
				{F(0 + desc.Width - 0.5), F(0               - 0.5), 0.0f, 1.0f, col, 1, 0},
				{F(0              - 0.5), F(0 + desc.Height - 0.5), 0.0f, 1.0f, col, 0, 1},
				{F(0 + desc.Width - 0.5), F(0 + desc.Height - 0.5), 0.0f, 1.0f, col, 1, 1}
			};
			LPDIRECT3DSURFACE9 surface;
			if ((frame % _intervalDiff) == 0) {
				_small1->GetSurfaceLevel(0, &surface);
				hr = device->SetRenderTarget(0, surface);
				_renderer.drawTexture(0, 0, desc.Width, desc.Height, cameraImage, 0, col, col, col, col);
				SAFE_RELEASE(surface);
			}
			if ((frame % _intervalSmall) == 0) {
				_small2->GetSurfaceLevel(0, &surface);
				hr = device->SetRenderTarget(0, surface);
				_renderer.drawTexture(0, 0, desc.Width, desc.Height, cameraImage, 0, col, col, col, col);
				SAFE_RELEASE(surface);

				// 差分
				_diff->GetSurfaceLevel(0, &surface);
				hr = device->SetRenderTarget(0, surface);
				_fx->SetTechnique("BasicTech");
				_fx->SetFloat("subtract", _subtract);
				_fx->SetTexture("bgTex", _small1);
				_fx->SetTexture("currentTex", _small2);
				hr = _fx->Begin(0, 0);
				if (SUCCEEDED(hr)) {
					_fx->BeginPass(1);
					device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, dst, VERTEX_SIZE);
					_fx->EndPass();
					_fx->End();
				}
				SAFE_RELEASE(surface);

				if (_renderer.getRenderTargetData(_diff, _diff2)) {
					D3DSURFACE_DESC desc;
					_diff2->GetDesc(&desc);
					D3DLOCKED_RECT lockedRect;
					hr = _diff2->LockRect(&lockedRect, NULL, 0);
					if (SUCCEEDED(hr)) {
						_diffCount = 0;
						DWORD* buf = (DWORD*)lockedRect.pBits;
						int pitch = lockedRect.Pitch / 4;
						DWORD p, p0, p1, p2, p3;
						for (int i = 0; i < 2; i++) {
							for (int y = 0; y < desc.Height; y++) {
								for (int x = 0; x < pitch; x++) {
									p0 = 0; p1 = 0; p2 = 0; p3 = 0;
									if (y > 0) p0 = buf[(y - 1) * pitch + x];
									if (y < desc.Height - 1) p3 = buf[(y + 1) * pitch + x];
									if (x > 0) p1 = buf[y * pitch + x - 1];
									if (x < pitch - 1) p2 = buf[y * pitch + x + 1];
									p = buf[y * pitch + x];
									if (p == 0xff000000 && p1 == p2 && p2 == 0xffffffff) buf[y * pitch + x] = 0xffffffff;
									if (p == 0xff000000 && p0 == p3 && p3 == 0xffffffff) buf[y * pitch + x] = 0xffffffff;
									if (p == 0xffffffff && p0 == p1 && p1 == p2 && p2 == p3 && p3 == 0xff000000) buf[y * pitch + x] = 0xff000000;
								}
							}
						}

						for (int i = 0; i < pitch * desc.Height; i++) {
							if (buf[i] == 0xffffffff) _diffCount++;
						}
						_diff2->UnlockRect();
						if (!_renderer.updateRenderTargetData(_diff, _diff2)) {
							_log.warning("failed updateRenderTargetData");
						}
						if (desc.Width * desc.Height <= _diffCount) _diffCount = 0;
					}
				}
			}
			hr = device->SetRenderTarget(0, orgRT);
			SAFE_RELEASE(orgRT);
		}

		// ステータス表示
		col = 0xffffffff;
		device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
		device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
		_renderer.drawTexture(320, 480, 320, 240, _diff, 0, col, col, col, col);
		_renderer.drawFontTextureText(320, 480, 12, 16, 0xccffffff, Poco::format("<%04d/%04d>", _diffCount, _detectThreshold));
		_renderer.drawTexture(640, 480, 320, 240, _small1, 0, col, col, col, col);
		float interval = _intervalDiff / 60.0f;
		_renderer.drawFontTextureText(640, 480, 12, 16, 0xccffffff, Poco::format("%d(%0.2hfs)", _intervalDiff, interval));
		_renderer.drawTexture(960, 480, 320, 240, _small2, 0, col, col, col, col);
		interval = _intervalSmall / 60.0f;
		_renderer.drawFontTextureText(960, 480, 12, 16, 0xccffffff, Poco::format("%d(%0.2hfs)", _intervalSmall, interval));
		device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		if (_statusFrame > 0 && (frame - _statusFrame) < 200) {
			_renderer.drawFontTextureText(640, config().mainRect.bottom - 32, 24, 32, 0xccff3333, _status);
		}
	}
}
