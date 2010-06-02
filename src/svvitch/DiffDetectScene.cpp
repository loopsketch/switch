#include "DiffDetectScene.h"
#include <Poco/Util/XMLConfiguration.h>
#include <Poco/UnicodeConverter.h>
#include <Poco/FileStream.h>
#include <Poco/Buffer.h>
#include <Poco/LocalDateTime.h>
#include <Poco/DateTimeFormat.h>
#include <Poco/DateTimeFormatter.h>

#include "MainScene.h"
#include "CaptureScene.h"
#include "Utils.h"


DiffDetectScene::DiffDetectScene(Renderer& renderer): Scene(renderer),
	_w(320), _h(240), _frame(0), _first(true), _subtract(0),
	_frame1(NULL), _frame2(NULL), _frame3(NULL), _result1(NULL), _result2(NULL), _fx(NULL)
{
}

DiffDetectScene::~DiffDetectScene() {
	SAFE_RELEASE(_fx);
	SAFE_RELEASE(_frame1);
	SAFE_RELEASE(_frame2);
	SAFE_RELEASE(_frame3);
	SAFE_RELEASE(_result1);
	SAFE_RELEASE(_result2);
	_log.information("*release diff-detect-scene");
}

bool DiffDetectScene::initialize() {
	_log.information("*initialize diff-detect-scene");

	try {
		Poco::Util::XMLConfiguration* xml = new Poco::Util::XMLConfiguration("capture-config.xml");
		if (xml) {
			_w = xml->getInt("device[@w]", 640) / 4;
			_h = xml->getInt("device[@h]", 480) / 4;
			if (xml->hasProperty("diff")) {
				_previewX = xml->getInt("diff.preview.x", 0);
				_previewY = xml->getInt("diff.preview.y", 0);
				_previewW = xml->getInt("diff.preview.width", _w);
				_previewH = xml->getInt("diff.preview.height", _h);
				_w = xml->getInt("diff.width", _w);
				_h = xml->getInt("diff.height", _h);
				_subtract = xml->getDouble("diff.subtract", 0.5);
			}
			xml->release();
		}
	} catch (Poco::Exception& ex) {
		_log.warning(ex.displayText());
	}

	_frame1 = _renderer.createRenderTarget(_w, _h, D3DFMT_A8R8G8B8);
	_frame2 = _renderer.createRenderTarget(_w, _h, D3DFMT_A8R8G8B8);
	_frame3 = _renderer.createRenderTarget(_w, _h, D3DFMT_A8R8G8B8);
	_result1 = _renderer.createRenderTarget(_w, _h, D3DFMT_A8R8G8B8);
	_result2 = _renderer.createRenderTarget(_w, _h, D3DFMT_A8R8G8B8);
	_renderer.colorFill(_frame1, 0xff000000);
	_renderer.colorFill(_frame2, 0xff000000);
	_fx = _renderer.createEffect("fx/diff.fx");
	if (_fx) {
		HRESULT hr = _fx->SetTechnique("diffTechnique");
		if FAILED(hr) _log.warning("failed not set technique diffTechnique");
		hr = _fx->SetFloat("width", F(_w));
		hr = _fx->SetFloat("height", F(_h));
		hr = _fx->SetFloat("subtract", F(_subtract));
		if FAILED(hr) _log.warning("failed not set subtract");
		hr = _fx->SetTexture("frame1", _frame1);
		if FAILED(hr) _log.warning("failed not set texture1");
		hr = _fx->SetTexture("frame2", _frame2);
		if FAILED(hr) _log.warning("failed not set texture2");
		hr = _fx->SetTexture("frame3", _frame3);
		if FAILED(hr) _log.warning("failed not set texture3");
		hr = _fx->SetTexture("result1", _result1);
		if FAILED(hr) _log.warning("failed not set result1 texture");
		hr = _fx->SetTexture("result2", _result2);
		if FAILED(hr) _log.warning("failed not set result2 texture");
	}

	return true;
}

LPDIRECT3DTEXTURE9 DiffDetectScene::getResult() {
	return _result1;
}

void DiffDetectScene::process() {
	CaptureScenePtr scene = dynamic_cast<CaptureScenePtr>(_renderer.getScene("capture"));
	if (scene) {
		LPDIRECT3DTEXTURE9 texture = scene->getCameraImage();
		if (texture) {
			_renderer.copyTexture(texture, _frame1);
		}
	}
	_renderer.copyTexture(_result1, _result2);
	_frame++;
}

void DiffDetectScene::draw1() {
	LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
	LPDIRECT3DSURFACE9 orgRT;
	HRESULT hr = device->GetRenderTarget(0, &orgRT);
	DWORD col = 0xffffffff;
	VERTEX dst[] =
	{
		{F(0      - 0.5), F(0      - 0.5), 0.0f, 1.0f, col, 0, 0},
		{F(0 + _w - 0.5), F(0      - 0.5), 0.0f, 1.0f, col, 1, 0},
		{F(0      - 0.5), F(0 + _h - 0.5), 0.0f, 1.0f, col, 0, 1},
		{F(0 + _w - 0.5), F(0 + _h - 0.5), 0.0f, 1.0f, col, 1, 1}
	};
	drawAverage(dst, col);
	drawOpenning(dst, col);
	//drawEdge(dst, col);
	drawDiff(dst, col);
	hr = device->SetRenderTarget(0, orgRT);
	SAFE_RELEASE(orgRT);
}

void DiffDetectScene::drawAverage(VERTEX* dst, DWORD& col) {
	if (_frame2) {
		LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
		LPDIRECT3DSURFACE9 surface;
		HRESULT hr = _frame2->GetSurfaceLevel(0, &surface);
		hr = device->SetRenderTarget(0, surface);
		if (_fx) {
			_fx->Begin(NULL, 0);
			_fx->BeginPass(0);
			device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, dst, VERTEX_SIZE);
			_fx->EndPass();
			_fx->End();
		}
		SAFE_RELEASE(surface);
	}
}

void DiffDetectScene::drawOpenning(VERTEX* dst, DWORD& col) {
	if (_result1) {
		LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
		LPDIRECT3DSURFACE9 surface;
		HRESULT hr = _result1->GetSurfaceLevel(0, &surface);
		hr = device->SetRenderTarget(0, surface);
		device->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
		if (_fx) {
			_fx->Begin(NULL, 0);
			_fx->BeginPass(1);
			device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, dst, VERTEX_SIZE);
			_fx->EndPass();
			_fx->BeginPass(2);
			device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, dst, VERTEX_SIZE);
			_fx->EndPass();
			_fx->End();
		}
		SAFE_RELEASE(surface);
	}
}

void DiffDetectScene::drawDiff(VERTEX* dst, DWORD& col) {
	LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
	if (_frame3) {
		LPDIRECT3DSURFACE9 surface;
		HRESULT hr = _frame3->GetSurfaceLevel(0, &surface);
		hr = device->SetRenderTarget(0, surface);
		device->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
		if (_fx) {
			_fx->Begin(NULL, 0);
			_fx->BeginPass(3);
			device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, dst, VERTEX_SIZE);
			_fx->EndPass();
			_fx->End();
		}
		SAFE_RELEASE(surface);
	}
}

void DiffDetectScene::draw2() {
	if (_visible && _result1) {
		LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
		//device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		//device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		DWORD col = 0xffffffff;
		_renderer.drawTexture(_previewX, _previewY, _previewW, _previewH, _frame2, 0, col, col, col, col);
		_renderer.drawTexture(_previewX + _previewW, _previewY, _previewW, _previewH, _frame3, 0, col, col, col, col);
		_renderer.drawTexture(_previewX + _previewW * 2, _previewY, _previewW, _previewH, _result1, 0, col, col, col, col);
		//device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
		//device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	}
}
