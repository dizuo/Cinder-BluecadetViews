#include "BaseApp.h"
#include "SettingsManager.h"
#include "ScreenLayout.h"
#include "ScreenCamera.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace bluecadet::views;
using namespace bluecadet::touch;
using namespace bluecadet::touch::drivers;

namespace bluecadet {
namespace core {

BaseApp::BaseApp() :
	ci::app::App(),
	mLastUpdateTime(0),
	mDebugUiPadding(16.0f),
	mRootView(new BaseView()),
	mMiniMap(new MiniMapView(0.025f)),
	mStats(new GraphView(ivec2(128, 48))) {
}

BaseApp::~BaseApp() {
}

void BaseApp::prepareSettings(ci::app::App::Settings *settings) {
	fs::path appSettingsPath = ci::app::getAssetPath("appSettings.json");
	SettingsManager::getInstance()->setup(appSettingsPath, settings);

	// Apply pre-launch settings
#ifdef CINDER_MSW
	settings->setConsoleWindowEnabled(SettingsManager::getInstance()->mConsoleWindowEnabled);
#endif
//	settings->setFrameRate((float)SettingsManager::getInstance()->mFps);
//	settings->setWindowSize(SettingsManager::getInstance()->mWindowSize);
//	settings->setBorderless(SettingsManager::getInstance()->mBorderless);
//	settings->setFullScreen(SettingsManager::getInstance()->mFullscreen);
//	settings->setHighDensityDisplayEnabled(true);

	// Keep window top-left within display bounds
	if (settings->getWindowPos().x == 0 && settings->getWindowPos().y == 0) {
		ivec2 windowPos = (Display::getMainDisplay()->getSize() - settings->getWindowSize()) / 2;
		windowPos = glm::max(windowPos, ivec2(0));
		settings->setWindowPos(windowPos);
	}
}


void BaseApp::setup() {
	auto settings = SettingsManager::getInstance();

	// Set up screen layout
	int displayWidth = settings->hasField("settings.display.width") ? settings->getField<int>("settings.display.width") : ScreenLayout::getInstance()->getDisplayWidth();
	int displayHeight = settings->hasField("settings.display.height") ? settings->getField<int>("settings.display.height") : ScreenLayout::getInstance()->getDisplayHeight();
	int rows = settings->hasField("settings.display.rows") ? settings->getField<int>("settings.display.rows") : ScreenLayout::getInstance()->getNumRows();
	int cols = settings->hasField("settings.display.columns") ? settings->getField<int>("settings.display.columns") : ScreenLayout::getInstance()->getNumColumns();

	ScreenLayout::getInstance()->getAppSizeChangedSignal().connect(bind(&BaseApp::handleAppSizeChange, this, placeholders::_1));
	ScreenLayout::getInstance()->setup(ivec2(displayWidth, displayHeight), rows, cols);

	ScreenCamera::getInstance()->setup(ScreenLayout::getInstance());
	ScreenCamera::getInstance()->getViewportChangedSignal().connect(bind(&BaseApp::handleViewportChange, this, placeholders::_1));
	ScreenCamera::getInstance()->zoomToFitWindow();

	// Apply run-time settings
	if (settings->mShowMouse) {
		showCursor();
	} else {
		hideCursor();
	}

#if defined(CINDER_MSW)
	// Move window to foreground so that console output doesn't obstruct it
	HWND nativeWindow = (HWND)getWindow()->getNative();
	::SetForegroundWindow(nativeWindow);
#endif

	// Set up graphics
	gl::enableVerticalSync(settings->mVerticalSync);
	gl::enableAlphaBlending();

	// Set up touches
	mMouseDriver.connect();
	mTuioDriver.connect();
	mSimulatedTouchDriver.setup(Rectf(vec2(0), getWindowSize()), 60);

	// Debugging
	const float fps = (float)SettingsManager::getInstance()->mFps;
	mStats->setBackgroundColor(ColorA(0, 0, 0, 0.1f));
	//mStats->addGraph("FPS", 0, fps, ColorA(1, 1, 1, 0.75f), ColorA(1, 1, 1, 0.25f));
	mStats->addGraph("Delta Time", 1.0f / fps, 0.1f, ColorA(1, 1, 0, 0.5f), ColorA(1, 0, 0, 0.5f));
}

void BaseApp::update() {
	const double currentTime = getElapsedSeconds();
	const double deltaTime = mLastUpdateTime == 0 ? 0 : currentTime - mLastUpdateTime;
	mLastUpdateTime = currentTime;

	// get the screen layout's transform and apply it to all
	// touch events to convert touches from window into app space
	const auto appTransform = glm::inverse(ScreenCamera::getInstance()->getTransform());
	const auto appSize = ScreenLayout::getInstance()->getAppSize();
	touch::TouchManager::getInstance()->update(mRootView, appSize, appTransform);
	mRootView->updateScene(deltaTime);

	//mStats->addValue("FPS", getAverageFps());
	mStats->addValue("Delta Time", (float)deltaTime);
}

void BaseApp::draw(const bool clear) {
	auto settings = SettingsManager::getInstance();

	if (clear) {
		gl::clear(settings->mClearColor);
	}

	{
		gl::ScopedModelMatrix scopedMatrix;
		// apply screen layout transform to root view
		gl::multModelMatrix(ScreenCamera::getInstance()->getTransform());
		mRootView->drawScene();

		// draw debug touches in app coordinate space
		if (settings->mDebugMode && settings->mDrawTouches) {
			touch::TouchManager::getInstance()->debugDrawTouches();
		}
	}

	if (settings->mDebugMode) {
		// draw params and debug layers in window coordinate space
		if (settings->mDrawScreenLayout) {
			gl::ScopedModelMatrix scopedMatrix;
			gl::multModelMatrix(ScreenCamera::getInstance()->getTransform());
			ScreenLayout::getInstance()->draw();
		}
		if (settings->mDrawMinimap) {
			mMiniMap->drawScene();
		}
		if (settings->mDrawStats) {
			mStats->drawScene();
		}
		settings->getParams()->draw();
	}
}

void BaseApp::keyDown(KeyEvent event) {
	if (event.isHandled()) {
		// don't do anything on previously handled events
		return;
	}

	switch (event.getCode()) {
		case KeyEvent::KEY_q:
			quit();
			break;
		case KeyEvent::KEY_f:
			SettingsManager::getInstance()->mFullscreen = !isFullScreen();
			setFullScreen(SettingsManager::getInstance()->mFullscreen);
			ScreenCamera::getInstance()->zoomToFitWindow();
			break;
	}
}

void BaseApp::handleAppSizeChange(const ci::ivec2 & appSize) {
	mRootView->setSize(vec2(appSize));
	mMiniMap->setLayout(
		ScreenLayout::getInstance()->getNumColumns(),
		ScreenLayout::getInstance()->getNumRows(),
		ScreenLayout::getInstance()->getDisplaySize()
	);
}

void BaseApp::handleViewportChange(const ci::Area & viewport) {
	mMiniMap->setViewport(viewport);
	mMiniMap->setPosition(vec2(getWindowSize()) - mMiniMap->getSize() - vec2(mDebugUiPadding));
	mStats->setPosition(vec2(mDebugUiPadding, (float)getWindowHeight() - mStats->getHeight() - mDebugUiPadding));
	SettingsManager::getInstance()->getParams()->setPosition(vec2(mDebugUiPadding));
}

void BaseApp::addTouchSimulatorParams(float touchesPerSecond) {

	mSimulatedTouchDriver.setTouchesPerSecond(touchesPerSecond);

	const string groupName = "Touch-Sim";

	SettingsManager::getInstance()->getParams()->addParam<bool>("Enabled", [&](bool v) {
		if (!mSimulatedTouchDriver.isRunning()) {
			SettingsManager::getInstance()->mDrawTouches = true;
			mSimulatedTouchDriver.setBounds(Rectf(vec2(0), getWindowSize()));
			mSimulatedTouchDriver.start();
		} else {
			SettingsManager::getInstance()->mDrawTouches = false;
			mSimulatedTouchDriver.stop();
		}
	}, [&] {
		return SettingsManager::getInstance()->mDrawTouches && mSimulatedTouchDriver.isRunning();
	}).group(groupName);

	static int stressTestMode = 0;
	static vector<string> stressTestModes = { "Tap & Drag", "Slow Drag", "Tap" };

	SettingsManager::getInstance()->getParams()->addParam("Mode", stressTestModes, &stressTestMode).updateFn([&] {
		if (stressTestMode == 0) {
			mSimulatedTouchDriver.setMinTouchDuration(0);
			mSimulatedTouchDriver.setMaxTouchDuration(1.f);
			mSimulatedTouchDriver.setMaxDragDistance(200.f);
		} else if (stressTestMode == 1) {
			mSimulatedTouchDriver.setMinTouchDuration(4.f);
			mSimulatedTouchDriver.setMaxTouchDuration(8.f);
			mSimulatedTouchDriver.setMaxDragDistance(200.f);
		} else if (stressTestMode == 2) {
			mSimulatedTouchDriver.setMinTouchDuration(0);
			mSimulatedTouchDriver.setMaxTouchDuration(0.1f);
			mSimulatedTouchDriver.setMaxDragDistance(0.f);
		}
	}).group(groupName);

	SettingsManager::getInstance()->getParams()->addParam<float>("Touches/s", [&](float v) {
		mSimulatedTouchDriver.setTouchesPerSecond(v);
	}, [&]() {
		return mSimulatedTouchDriver.getTouchesPerSecond();
	}).group(groupName);

	SettingsManager::getInstance()->getParams()->addParam<bool>("Show Missed Touches", [&](bool v) {
		TouchManager::getInstance()->setDiscardMissedTouches(!v);
	}, [&]() {
		return !TouchManager::getInstance()->getDiscardMissedTouches();
	}).group(groupName);

}

}
}
