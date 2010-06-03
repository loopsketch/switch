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
	_w(320), _h(240), _frame(0), _samples(0),
	_frame1(NULL), _frame2(NULL), _frame3(NULL), _fx(NULL)
{
}

DiffDetectScene::~DiffDetectScene() {
	SAFE_RELEASE(_fx);
	SAFE_RELEASE(_frame1);
	SAFE_RELEASE(_frame2);
	SAFE_RELEASE(_frame3);
	_log.information("*release diff-detect-scene");
}

bool DiffDetectScene::initialize() {
	_log.information("*initialize diff-detect-scene");

	try {
		Poco::Util::XMLConfiguration* xml = new Poco::Util::XMLConfiguration("capture-config.xml");
		if (xml) {
			_w = xml->getInt("device[@w]", 640);
			_h = xml->getInt("device[@h]", 480);
			if (xml->hasProperty("diff")) {
				_previewX = xml->getInt("diff.preview.x", 0);
				_previewY = xml->getInt("diff.preview.y", 0);
				_previewW = xml->getInt("diff.preview.width", 320);
				_previewH = xml->getInt("diff.preview.height", 240);
				_w = xml->getInt("diff.width", _w);
				_h = xml->getInt("diff.height", _h);
				//_subtract = xml->getDouble("diff.subtract", 0.5);
			}
			xml->release();
		}
	} catch (Poco::Exception& ex) {
		_log.warning(ex.displayText());
	}

	_frame1 = _renderer.createRenderTarget(_w, _h, D3DFMT_A8R8G8B8);
	_frame2 = _renderer.createRenderTarget(_w, _h, D3DFMT_A8R8G8B8);
	_frame3 = _renderer.createRenderTarget(_w, _h, D3DFMT_A8R8G8B8);
	_renderer.colorFill(_frame2, 0xff000000);
	_renderer.colorFill(_frame3, 0xff000000);
	_fx = _renderer.createEffect("fx/diff.fx");
	if (_fx) {
		HRESULT hr = _fx->SetTechnique("diffTechnique");
		if FAILED(hr) _log.warning("failed not set technique diffTechnique");
		hr = _fx->SetFloat("width", F(_w));
		hr = _fx->SetFloat("height", F(_h));
		hr = _fx->SetTexture("frame1", _frame1);
		if FAILED(hr) _log.warning("failed not set texture1");
		hr = _fx->SetTexture("frame2", _frame2);
		if FAILED(hr) _log.warning("failed not set texture2");
		hr = _fx->SetTexture("frame3", _frame3);
		if FAILED(hr) _log.warning("failed not set texture3");
	}

	return true;
}

LPDIRECT3DTEXTURE9 DiffDetectScene::getResult() {
	return _frame1;
}

void DiffDetectScene::process() {
	CaptureScenePtr scene = dynamic_cast<CaptureScenePtr>(_renderer.getScene("capture"));
	if (scene) {
		LPDIRECT3DTEXTURE9 texture = scene->getCameraImage();
		if (texture) {
			// ƒLƒƒƒvƒ`ƒƒ‰f‘œŽæž
			_renderer.copyTexture(texture, _frame1);
		}
	}
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
	//drawAverage(dst, col);
	//drawEdge(dst, col);
	drawDiff(dst, col);
	hr = device->SetRenderTarget(0, orgRT);
	SAFE_RELEASE(orgRT);
}

void DiffDetectScene::drawDiff(VERTEX* dst, DWORD& col) {
	LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
	if (_frame2 && _fx) {
		LPDIRECT3DSURFACE9 surface = NULL;
		HRESULT hr = _frame2->GetSurfaceLevel(0, &surface);
		hr = device->SetRenderTarget(0, surface);
		hr = _fx->SetInt("samples", _samples);
		if FAILED(hr) _log.warning("failed not set texture3");
		_fx->Begin(NULL, 0);
		_fx->BeginPass(0);
		device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, dst, VERTEX_SIZE);
		_fx->EndPass();
		_fx->BeginPass(1);
		device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, dst, VERTEX_SIZE);
		_fx->EndPass();
		_fx->BeginPass(2);
		device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, dst, VERTEX_SIZE);
		_fx->EndPass();
		_fx->End();
		_samples++;
		SAFE_RELEASE(surface);
	}
}

void DiffDetectScene::drawEdge(VERTEX* dst, DWORD& col) {
}

void DiffDetectScene::drawAverage(VERTEX* dst, DWORD& col) {
	LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
	if (_frame3 && _fx) {
		LPDIRECT3DSURFACE9 surface = NULL;
		HRESULT hr = _frame3->GetSurfaceLevel(0, &surface);
		hr = device->SetRenderTarget(0, surface);
		_fx->Begin(NULL, 0);
		_fx->BeginPass(1);
		device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, dst, VERTEX_SIZE);
		_fx->EndPass();
		_fx->End();
		SAFE_RELEASE(surface);
	}
}

void DiffDetectScene::draw2() {
	if (_visible && _result1) {
		LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
		device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		DWORD col = 0xffffffff;
		_renderer.drawTexture(_previewX, _previewY, _previewW, _previewH, _frame1, 0, col, col, col, col);
		_renderer.drawTexture(_previewX + _previewW, _previewY, _previewW, _previewH, _frame2, 0, col, col, col, col);
		_renderer.drawTexture(_previewX + _previewW * 2, _previewY, _previewW, _previewH, _frame3, 0, col, col, col, col);
		_renderer.drawFontTextureText(_previewX + _previewW, _previewY, 8, 12, 0xccff6666, Poco::format("%d", _samples));
		device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
		device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	}
}
