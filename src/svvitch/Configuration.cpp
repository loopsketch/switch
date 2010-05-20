#include "Configuration.h"

#include <Poco/Util/XMLConfiguration.h>
#include <Poco/ConsoleChannel.h>
#include <Poco/File.h>
#include <Poco/FileStream.h>
#include <Poco/format.h>
#include <Poco/LineEndingConverter.h>
#include <Poco/Logger.h>
#include <Poco/NullChannel.h>
#include <Poco/FileChannel.h>
#include <Poco/FormattingChannel.h>
#include <Poco/PatternFormatter.h>
#include <Poco/UnicodeConverter.h>
#include "Utils.h"


Configuration::Configuration() {
}

Configuration::~Configuration() {
	logFile->release();
//	_log.shutdown();
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
		Poco::Logger& log = Poco::Logger::get("");
		log.setChannel(fc);
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
		log.information("*** configuration");

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
		draggable = xml->getBool("display.draggable", true);
		mouse = xml->getBool("display.mouse", true);
		string windowStyles(fullsceen?"fullscreen":"window");
		log.information(Poco::format("display %dx%d@%d %s", w, h, mainRate, windowStyles));
		useClip = xml->getBool("display.clip.use", false);
		clipRect.left = xml->getInt("display.clip.x1", 0);
		clipRect.top = xml->getInt("display.clip.y1", 0);
		clipRect.right = xml->getInt("display.clip.x2", 0);
		clipRect.bottom = xml->getInt("display.clip.y2", 0);
		string useClip(useClip?"use":"not use");
		log.information(Poco::format("clip [%s] %ld,%ld %ldx%ld", useClip, clipRect.left, clipRect.top, clipRect.right, clipRect.bottom));

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
		} else {
			splitType = 0;
		}
		log.information(Poco::format("stage (%ld,%ld) %ldx%ld", stageRect.left, stageRect.top, stageRect.right, stageRect.bottom));
		log.information(Poco::format("split <%s:%d> %dx%d x%d", st, splitType, cw, ch, cycles));

		string engines = xml->getString("movieEngines", "ffmpeg");
		svvitch::split(engines, ',', movieEngines);
		string scenesParams = xml->getString("scenes", "");
		svvitch::split(scenesParams, ',', scenes);
		brightness = xml->getInt("stage.brightness", 100);
		viewStatus = xml->getBool("stage.viewStatus", false);

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
		dataRoot = Path(xml->getString("data-root", "")).absolute();
		log.information(Poco::format("data root: %s", dataRoot.toString()));
		workspaceFile = Path(dataRoot, xml->getString("workspace", "workspace.xml"));
		log.information(Poco::format("workspace: %s", workspaceFile.toString()));
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
	Poco::Logger& log = Poco::Logger::get("");
	log.information("save configuration");
	try {
		Poco::File f("switch-config.xml");
		Poco::Util::XMLConfiguration* xml = new Poco::Util::XMLConfiguration(f.path());
		if (xml) {
			bool update = false;
			if (xml->getInt("stage.brightness", -1) != brightness) {
				xml->setInt("stage.brightness", brightness);
				update = true;
			}
			if (xml->getBool("stage.viewStatus", !viewStatus) != viewStatus) {
				xml->setBool("stage.viewStatus",  viewStatus);
				update = true;
			}
			if (update) {
				Poco::File old("switch-config.xml.old");
				if (old.exists()) old.remove();
				f.renameTo(old.path());
				Poco::FileOutputStream fos("switch-config.xml");
				Poco::OutputLineEndingConverter os(fos, Poco::LineEnding::NEWLINE_CRLF);
				xml->save(os);
				xml->release();
				log.information("saved configuration");
			}
		}
	} catch (Poco::Exception& ex) {
		log.warning(Poco::format("failed save configuration file: %s", ex.displayText()));
	}
}

bool Configuration::hasScene(string s) {
	return std::find(scenes.begin(), scenes.end(), s) != scenes.end();
}
