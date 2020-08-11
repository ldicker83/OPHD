// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "Window.h"

#include "../../Cache.h"
#include "../../Common.h"
#include "../../Constants.h"
#include "../../FontManager.h"

#include <NAS2D/Utility.h>
#include <NAS2D/Renderer/Renderer.h>

using namespace NAS2D;

static const Font* WINDOW_TITLE_FONT;

Window::Window(std::string newTitle) :
	mTitleBarLeft{imageCache.load("ui/skin/window_title_left.png")},
	mTitleBarCenter{imageCache.load("ui/skin/window_title_middle.png")},
	mTitleBarRight{imageCache.load("ui/skin/window_title_right.png")},
	mBody{
		imageCache.load("ui/skin/window_top_left.png"),
		imageCache.load("ui/skin/window_top_middle.png"),
		imageCache.load("ui/skin/window_top_right.png"),
		imageCache.load("ui/skin/window_middle_left.png"),
		imageCache.load("ui/skin/window_middle_middle.png"),
		imageCache.load("ui/skin/window_middle_right.png"),
		imageCache.load("ui/skin/window_bottom_left.png"),
		imageCache.load("ui/skin/window_bottom_middle.png"),
		imageCache.load("ui/skin/window_bottom_right.png")
	}
{
	text(newTitle);
	_init();
}


Window::~Window()
{
	Utility<EventHandler>::get().mouseButtonUp().disconnect(this, &Window::onMouseUp);
	Utility<EventHandler>::get().mouseMotion().disconnect(this, &Window::onMouseMove);
}


void Window::_init()
{
	Utility<EventHandler>::get().mouseButtonUp().connect(this, &Window::onMouseUp);
	Utility<EventHandler>::get().mouseMotion().connect(this, &Window::onMouseMove);

	WINDOW_TITLE_FONT = &Utility<FontManager>::get().load(constants::FONT_PRIMARY_BOLD, constants::FONT_PRIMARY_NORMAL);
}


void Window::onMouseDown(EventHandler::MouseButton button, int x, int y)
{
	if (!visible() || !hasFocus()) { return; }

	UIContainer::onMouseDown(button, x, y);

	const auto titleBarBounds = NAS2D::Rectangle{mRect.x, mRect.y, mRect.width, sWindowTitleBarHeight};
	mMouseDrag = (button == EventHandler::MouseButton::BUTTON_LEFT && titleBarBounds.contains(NAS2D::Point{x, y}));
}


void Window::onMouseUp(EventHandler::MouseButton /*button*/, int /*x*/, int /*y*/)
{
	mMouseDrag = false;
}


void Window::onMouseMove(int /*x*/, int /*y*/, int dX, int dY)
{
	if (!visible() || !hasFocus()) { return; }

	if (mMouseDrag && !mAnchored)
	{
		position(position() + NAS2D::Vector{dX, dY});
	}
}

void Window::anchored(bool isAnchored)
{
	mAnchored = isAnchored;
}


void Window::show()
{
	Control::show();
	hasFocus(true);
}


void Window::update()
{
	if (!visible()) { return; }

	auto& renderer = Utility<Renderer>::get();

	renderer.drawImage(mTitleBarLeft, mRect.startPoint());
	renderer.drawImageRepeated(mTitleBarCenter, NAS2D::Rectangle{mRect.x + 4, mRect.y, mRect.width - 8, sWindowTitleBarHeight});
	renderer.drawImage(mTitleBarRight, NAS2D::Point{mRect.x + mRect.width - 4, mRect.y});

	mBody.draw(renderer, NAS2D::Rectangle{mRect.x, mRect.y + 20, mRect.width, mRect.height - 20});

	renderer.drawText(*WINDOW_TITLE_FONT, text(), NAS2D::Point{mRect.x + 5, mRect.y + 2}, NAS2D::Color::White);

	UIContainer::update();
}
