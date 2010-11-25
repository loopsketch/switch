#include "Configuration.h"

#include <Poco/Util/XMLConfiguration.h>
#include <Poco/ConsoleChannel.h>
#include <Poco/File.h>
#include <Poco/FileStream.h>
#include <Poco/format.h>
#include <Poco/LineEndingConverter.h>
#include <Poco/NullChannel.h>
#include <Poco/FileChannel.h>
#include <Poco/FormattingChannel.h>
#include <Poco/PatternFormatter.h>
#include <Poco/string.h>
#include <Poco/UnicodeConverter.h>
#include "Utils.h"


Configuration::Configuration(): _log(Poco::Logger::get(""))
{
}

Configuration::~Configuration() {
	release();
}

void Configuration::release() {
//	_log.shutdown();
	if (logFile) {
		logFile->release(); logFile = NULL;
	}
}

bool Configuration::initialize() {
	try {
		Poco::Util::XMLConfiguration* xml = new Poco::Util::XMLConfiguration("switch-config.xml");
		Poco::PatternFormatter* pat = new Poco::PatternFormatter(xml->getString("log.pattern", "%Y-%m-%d %H:%M:%S.%c %N[%T]:%t"));
		pat->setProperty(Poco::PatternFormatter::PROP_TIMES, "local");
		Poco::FormattingChannel* fc = new Poco::FormattingChannel(pat);
		string path = xml->getString("log.file", "switch.log");
		if (path.empty()) {
			logFile = new Poco::NullChannel();
		} else {
			logFile = new Poco::FileChannel(path);
		}
		fc->setChannel(logFile);
		_log.setChannel(fc);
		// ローカル時刻指定
		fc->setProperty(Poco::FileChannel::PROP_TIMES, "local");
		// アーカイブファイル名への付加文字列[number/timestamp] (日付)
		fc->setProperty(Poco::FileChannel::PROP_ARCHIVE, xml->getString("log.archive", "timestamp"));
		// 圧縮[true/false] (あり)
		fc->setProperty(Poco::FileChannel::PROP_COMPRESS, xml->getString("log.compress", "true"));
		// ローテーション単位[never/[day,][hh]:mm/daily/weekly/monthly/<n>minutes/hours/days/weeks/months/<n>/<n>K/<n>M] (日)
		fc->setProperty(Poco::FileChannel::PROP_ROTATION, xml->getString("log.rotation", "daily"));
		// 保持期間[<n>seconds/<n>minutes/<n>hours/<n>days/<n>weeks/<n>months] (5日間)
		fc->setProperty(Poco::FileChannel::PROP_PURGEAGE, xml->getString("log.purgeage", "5days"));
		fc->release();
		pat->release();
		_log.information("*** configuration");

		windowTitle = xml->getString("display.title", "switch");
		mainRect.left = xml->getInt("display.x", 0);
		mainRect.top = xml->getInt("display.y", 0);
		int w = xml->getInt("display.width", 1024);
		int h = xml->getInt("display.height", 768);
		mainRect.right = w;
		mainRect.bottom = h;
		mainRate = xml->getInt("display.rate", 0);
		subRect.left = xml->getInt("display[1].x", mainRect.left);
		subRect.top = xml->getInt("display[1].y", mainRect.top);
		subRect.right = xml->getInt("display[1].width", mainRect.right);
		subRect.bottom = xml->getInt("display[1].height", mainRect.bottom);
		subRate = xml->getInt("display[1].rate", mainRate);
		frameIntervals = xml->getInt("display.frameIntervals", 3);
		frame = xml->getBool("display.frame", true);
		fullsceen = xml->getBool("display.fullscreen", true);
		draggable = xml->getBool("display.draggable", !fullsceen);
		mouse = xml->getBool("display.mouse", !fullsceen);
		string windowStyles(fullsceen?"fullscreen":"window");
		_log.information(Poco::format("display %dx%d@%d %s", w, h, mainRate, windowStyles));
		useClip = xml->getBool("display.clip.use", false);
		clipRect.left = xml->getInt("display.clip.x1", 0);
		clipRect.top = xml->getInt("display.clip.y1", 0);
		clipRect.right = xml->getInt("display.clip.x2", 0);
		clipRect.bottom = xml->getInt("display.clip.y2", 0);
		string useClip(useClip?"use":"not use");
		_log.information(Poco::format("clip [%s] %ld,%ld %ldx%ld", useClip, clipRect.left, clipRect.top, clipRect.right, clipRect.bottom));

		name = xml->getString("stage.name", "");
		description = xml->getString("stage.description", "");
		int cw = xml->getInt("stage.split.width", w);
		int ch = xml->getInt("stage.split.height", h);
		int cycles = xml->getInt("stage.split.cycles", h / ch);
		splitSize.cx = cw;
		splitSize.cy = ch;
		stageRect.left = xml->getInt("stage.x", 0);
		stageRect.top = xml->getInt("stage.y", 0);
		stageRect.right = xml->getInt("stage.width", w * cycles);
		stageRect.bottom = xml->getInt("stage.height", ch);
		splitCycles = cycles;
		string st = xml->getString("stage.split.type", "none");
		if (st == "vertical" || st == "vertical-down") {
			splitType = 1;
		} else if (st == "vertical-up") {
			splitType = 2;
		} else if (st == "horizontal") {
			splitType = 11;
		} else if (st == "matrix") {
			splitType = 21;
		} else {
			splitType = 0;
		}
		_log.information(Poco::format("stage (%ld,%ld) %ldx%ld", stageRect.left, stageRect.top, stageRect.right, stageRect.bottom));
		if (splitType != 0) _log.information(Poco::format("split <%s:%d> %dx%d x%d", st, splitType, cw, ch, cycles));

		string engines = xml->getString("movieEngines", "ffmpeg");
		svvitch::split(engines, ',', movieEngines);
		string scenesParams = xml->getString("scenes", "");
		svvitch::split(scenesParams, ',', scenes);
		brightness = xml->getInt("stage.brightness", 100);
		dimmer = xml->getDouble("stage.dimmer", 1);
		viewStatus = xml->getBool("stage.viewStatus", false);
		captureQuality = xml->getDouble("stage.captureQuality", 0.25f);
		captureFilter = Poco::toLower(xml->getString("stage.captureQuality[@filter]", ""));

		imageSplitWidth = xml->getInt("stage.imageSplitWidth", 0);
		if (xml->hasProperty("stage.text")) {
			string s;
			Poco::UnicodeConverter::toUTF8(L"ＭＳ ゴシック", s);
			textFont = xml->getString("stage.text.font", s);
			textStyle = xml->getString("stage.text.style", "");
			textHeight = xml->getInt("stage.text.height", stageRect.bottom - 2);
		} else {
			string s;
			Poco::UnicodeConverter::toUTF8(L"ＭＳ ゴシック", s);
			textFont = s;
			textStyle = "";
			textHeight = stageRect.bottom - 2;
		}

		string font = xml->getString("ui.defaultFont", "");
		wstring ws;
		Poco::UnicodeConverter::toUTF16(font, ws);
		defaultFont = ws;
		asciiFont = xml->getString("ui.asciiFont", "Defactica");
		multiByteFont = xml->getString("ui.multiByteFont", "A-OTF-ShinGoPro-Regular.ttf");
//		vpCommandFile = xml->getString("vpCommand", "");
//		monitorFile = xml->getString("monitor", "");
		dataRoot = Path(xml->getString("data-root", "datas")).absolute();
		stockRoot = Path(xml->getString("stock-root", "stocks")).absolute();
		_log.information(Poco::format("data root: %s (stock:%s)", dataRoot.toString(), stockRoot.toString()));
		workspaceFile = Path(dataRoot, xml->getString("workspace", "workspace.xml"));
		_log.information(Poco::format("workspace: %s", workspaceFile.toString()));
		newsURL = xml->getString("newsURL", "https://led.avix.co.jp:8080/news");

		serverPort = xml->getInt("server.port", 9090);
		maxQueued = xml->getInt("server.max-queued", 50);
		maxThreads = xml->getInt("server.max-threads", 8);

		if (xml->hasProperty("schedule")) {
			outCastLog = xml->getBool("schedule.castingLog", true);
		} else {
			outCastLog = true;
		}

		xml->release();
		return true;

	} catch (Poco::Exception& ex) {
		string s;
		Poco::UnicodeConverter::toUTF8(L"設定ファイル(switch-config.xml)を確認してください\n「%s」", s);
		wstring utf16;
		Poco::UnicodeConverter::toUTF16(Poco::format(s, ex.displayText()), utf16);
		::MessageBox(HWND_DESKTOP, utf16.c_str(), L"エラー", MB_OK);
	}
	return false;
}

void Configuration::save() {
	_log.information("save configuration");
	Poco::File config("switch-config.xml");
	Poco::File save("switch-config-new.xml");
	bool update = false;
	try {
		Poco::Util::XMLConfiguration* xml = new Poco::Util::XMLConfiguration(config.path());
		if (xml) {
			if (xml->getInt("stage.brightness", -1) != brightness) {
				xml->setInt("stage.brightness", brightness);
				update = true;
			}
			if (xml->getBool("stage.viewStatus", !viewStatus) != viewStatus) {
				xml->setBool("stage.viewStatus",  viewStatus);
				update = true;
			}
			if (update) {
				Poco::FileOutputStream fos(save.path());
				Poco::OutputLineEndingConverter os(fos, Poco::LineEnding::NEWLINE_CRLF);
				xml->save(os);
				_log.information("saved configuration");
			}
			xml->release();
		}
	} catch (Poco::IOException& ex) {
		_log.warning(Poco::format("failed save configuration file: %s", ex.displayText()));
		return;
	}

	try {
		if (update) {
			Poco::File old("switch-config-old.xml");
			if (old.exists()) old.remove();
			config.renameTo(old.path());
			save.renameTo(config.path());
		}
	} catch (Poco::IOException& ex) {
		_log.warning(Poco::format("failed save configuration file(rename step): %s", ex.displayText()));
	}
}

bool Configuration::hasScene(string s) {
	return std::find(scenes.begin(), scenes.end(), s) != scenes.end();
}
