#include "LineView.h"

using namespace ci;
using namespace ci::app;
using namespace std;

namespace bluecadet {
namespace views {

LineView::LineView() : BaseView(),
	mLineWidth(1.0f),
	mBackgroundColor(ColorA::white())
{
}

LineView::~LineView() {
}

inline void LineView::setup(const ci::vec2 & endPoint, const ci::ColorA & lineColor, const float lineWidth) {
	setEndPoint(endPoint);
	setBackgroundColor(lineColor);
	setLineWidth(lineWidth);
}

void LineView::draw() {
	gl::ScopedColor color(mBackgroundColor);
	gl::ScopedLineWidth lineWidth(mLineWidth);

	gl::drawLine(vec2(0, 0), getEndPoint());
}

}
}
