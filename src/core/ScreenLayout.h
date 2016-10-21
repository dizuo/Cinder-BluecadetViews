//+---------------------------------------------------------------------------
//  Bluecadet Interactive 2016
//  Contents: 
//  Comments: 
//----------------------------------------------------------------------------
#pragma once
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include <views/BaseView.h>

namespace bluecadet {
namespace core {

typedef std::shared_ptr<class ScreenLayout> ScreenLayoutRef;

class ScreenLayout {

public:


	ScreenLayout();
	~ScreenLayout();


	static ScreenLayoutRef getInstance() {
		static ScreenLayoutRef instance = nullptr;
		if (!instance) instance = ScreenLayoutRef(new ScreenLayout());
		return instance;
	}


	//! Must be called before calling draw. Adds a key-up event listener.
	void			setup(const ci::ivec2& dislaySize = ci::ivec2(1920, 1080), const int numRows = 1, const int numColumns = 1);

	//! Draws the current screen layout, transformed appropriately to match the position and scale of rootView
	void			draw();



	//! Dispatched whenever a property chnage affects the app size.
	ci::signals::Signal<void(const ci::ivec2 & appSize)> & getAppSizeChangedSignal() { return mAppSizeChanged; };



	//! The width of a single display in the display matrix
	int				getDisplayWidth() const { return mDisplaySize.x; };
	void			setDisplayWidth(const int width) { mDisplaySize.x = width; updateLayout(); };

	//! The height of a single display in the display matrix
	int				getDisplayHeight() const { return mDisplaySize.y; };
	void			setDisplayHeight(const int height) { mDisplaySize.y = height; updateLayout(); };

	//! The size of a single display in the display matrix
	ci::ivec2		getDisplaySize() const { return mDisplaySize; }
	void			setDisplaySize(const ci::ivec2 value) { mDisplaySize = value; }

	//! The number of rows of displays in the display matrix.
	int				getNumRows() const { return mNumRows; };
	void			setNumRows(const int numRows) { mNumRows = numRows; updateLayout(); };

	//! The number of columns of displays in the display matrix.
	int				getNumColumns() const { return mNumColumns; };
	void			setNumColumns(const int numColumns) { mNumColumns = numColumns; updateLayout(); };



	//! Helper to retrieve a display id from a row/col. Ids start at 0 and increment in right-to-left, top-to-bottom sequence.
	inline int		getDisplayId(const int row, const int col) const { return row * mNumColumns + col; };
	//! Helper to extract the column from a display id. Ids start at 0 and increment in right-to-left, top-to-bottom sequence.
	inline int		getColFromDisplayId(const int displayId) const { if (mNumColumns <= 0) return 0; return displayId % mNumColumns; };
	//! Helper to extract the row from a display id. Ids start at 0 and increment in right-to-left, top-to-bottom sequence.
	inline int		getRowFromDisplayId(const int displayId) const { if (mNumRows <= 0) return 0; return displayId / mNumColumns; };



	//! Absolute bounds of the display with the given id
	ci::Rectf		getDisplayBounds(const int displayId);

	//! Absolute bounds of the display at col/row
	ci::Rectf		getDisplayBounds(const int row, const int col);



	//! The total app size when scaled at 100%
	const ci::ivec2&	getAppSize() const { return mAppSize; };

	//! Overall app width when scaled at 100%
	int				getAppWidth() const { return getAppSize().x; }

	//! Overall app height when scaled at 100%
	int				getAppHeight() const { return getAppSize().y; }

	
	//! Zooms to fit the display at displayId into the current application window.
	void			zoomToDisplay(const int displayId);
	
	//! Zooms to fit the display at col/row into the current application window.
	void			zoomToDisplay(const int row, const int col);

	//! Zooms around a location in window coordinate space
	void			zoomAtLocation(const float scale, const ci::vec2 location);

	//! Zooms around the application window's center
	inline void		zoomAtWindowCenter(const float scale) { zoomAtLocation(scale, ci::app::getWindowCenter()); }

	//! Zooms the app to fit centered into the current window
	void			zoomToFitWindow();



	//! The border color used when drawing the display bounds. Defaults to opaque magenta.
	const ci::ColorA&	getBorderColor() { return mBorderColor; };
	void				setBorderColor(const ci::ColorA& color) { mBorderColor = color; };
	
	//! The border size used when drawing the display bounds. Defaults to 4.
	float				getBordeSize() const { return mBorderSize; }
	void				setBorderSize(const float value) { mBorderSize = value; }

	const ci::mat4&		getTransform() const { return mPlaceholderView->getTransform(); };

protected:
	float				getScaleToFitBounds(const ci::Rectf &bounds, const ci::vec2 &maxSize, const float padding = 0.0f) const;

	void				updateLayout();
	void				handleKeyDown(ci::app::KeyEvent event);


	//! Layout
	int			mNumRows;
	int			mNumColumns;

	ci::ivec2	mDisplaySize;
	ci::ivec2	mAppSize;

	float		mBorderSize;
	ci::ColorA	mBorderColor;

private:
	//! Used to draw bounds of each display
	std::vector<ci::Rectf>	mDisplayBounds;
	views::BaseViewRef		mPlaceholderView;
	ci::signals::Signal<void(const ci::ivec2 & appSize)> mAppSizeChanged;
};

}
}
