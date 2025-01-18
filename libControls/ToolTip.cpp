#include "ToolTip.h"

#include <NAS2D/EventHandler.h>
#include <NAS2D/Utility.h>
#include <NAS2D/Renderer/Renderer.h>

#include <string>
#include <sstream>
#include <functional>

namespace
{
	constexpr int MarginTight{2};
	constexpr auto PaddingSize = NAS2D::Vector{MarginTight, MarginTight};

	void forEachLine(const std::string& text, const std::function<void(const std::string&)>& lineHandler)
	{
		std::istringstream stream(text);
		for (std::string line; std::getline(stream, line);)
		{
			lineHandler(line);
		}
	}


	// Returns the maximum width of the largest string from the string stream in pixels.
	int calculateMaxWidthStringSize(const std::string& text, const NAS2D::Font& font)
	{
		int maxWidthStringSize = 0;
		forEachLine(text, [&font, &maxWidthStringSize](const std::string& lineStr) {
			maxWidthStringSize = std::max(maxWidthStringSize, font.size(lineStr).x);
		});
		return maxWidthStringSize;
	}


	// Returns a vector of pairs <indexes where the wraps should occur> and <if a truncation occurred>.
	// Wraps occur at the last available space before the text width is exceeded.
	std::vector<std::pair<size_t, bool>> wrapIndexesBreakOnSpace (const std::string& text, const NAS2D::Font& font, int textWidth)
	{
		std::vector<std::pair<size_t, bool>> wrapIndexes;
		size_t wrapIndexOffset = 0;
		size_t lastSpaceIndex = 0;
		const int truncationSize = font.size("...").x;

		for (size_t index = 0; index < text.size(); ++index)
		{
			if (text[index] == '\n')
			{
				wrapIndexOffset = index + 1;
			}

			if (text[index] == ' ')
			{
				lastSpaceIndex = index;
			}

			if (font.size(text.substr(wrapIndexOffset, index - wrapIndexOffset + 1)).x + truncationSize > textWidth && lastSpaceIndex <= wrapIndexOffset)
			{
				wrapIndexes.push_back({index - 1, true});

				// Fast forward the index to the next space
				while (index < text.size() && text[index] != ' ')
				{
					++index;
				}

				if (index < text.size())
				{
					wrapIndexOffset = index + 1;
					lastSpaceIndex = index;
				}
			}

			if (font.size(text.substr(wrapIndexOffset, index - wrapIndexOffset + 1)).x > textWidth && lastSpaceIndex > wrapIndexOffset)
			{
				// Rewind index to the last space
				index = lastSpaceIndex;
				wrapIndexOffset = index + 1;
				wrapIndexes.push_back({lastSpaceIndex, false});
			}
		}

		return wrapIndexes;
	};


	// Returns a constructed string with line breaks at the last available space before the text width is exceeded.
	// Lines truncated will attempt to fit and end in "..." but may spill beyond the designated width
	std::string wrapTextBreakOnSpace(const std::string& text, const NAS2D::Font& font, int textWidth)
	{
		auto wrapIndexes = wrapIndexesBreakOnSpace(text, font, textWidth);
		std::string wrappedText;
		size_t wrapIndexOffset = 0;
		for (const auto& [wrapIndex, truncated] : wrapIndexes)
		{
			wrappedText += text.substr(wrapIndexOffset, wrapIndex - wrapIndexOffset);
			if (truncated)
			{
				wrappedText += "...";
				wrapIndexOffset = wrapIndex;
				while (wrapIndexOffset < text.size() && text[wrapIndexOffset] != ' ')
				{
					++wrapIndexOffset;
				}

				if (wrapIndexOffset < text.size())
				{
					wrappedText += '\n';
					wrapIndexOffset++;
				}
			}
			else
			{
				wrappedText += '\n';
				wrapIndexOffset = wrapIndex + 1;
			}
		}
		wrappedText += text.substr(wrapIndexOffset);
		return wrappedText;
	};
}


ToolTip::ToolTip():
	mFont{getDefaultFont()}
{
	NAS2D::Utility<NAS2D::EventHandler>::get().mouseMotion().connect({this, &ToolTip::onMouseMove});
}


ToolTip::~ToolTip()
{
	NAS2D::Utility<NAS2D::EventHandler>::get().mouseMotion().disconnect({this, &ToolTip::onMouseMove});
}


void ToolTip::add(Control& c, const std::string& str)
{
	for (auto& item : mControls)
	{
		if (item.first == &c)
		{
			item.second = str;
			return;
		}
	}

	mControls.push_back(std::make_pair(&c, str));
}


void ToolTip::buildDrawParams(std::pair<Control*, std::string>& item, int mouseX)
{
	const auto toolTipLineHeight = mFont.height();
	const auto numberOfLines = static_cast<int>(std::count(item.second.begin(), item.second.end(), '\n') + 1);
	const auto toolTipTextHeight = toolTipLineHeight * numberOfLines;
	const auto toolTipTextWidth = calculateMaxWidthStringSize(item.second, mFont);
	const auto toolTipSize = NAS2D::Vector{toolTipTextWidth, toolTipTextHeight} + PaddingSize * 2;

	auto tooltipPosition = item.first->position();

	auto& renderer = NAS2D::Utility<NAS2D::Renderer>::get();
	const auto maxX = renderer.size().x - toolTipSize.x;
	tooltipPosition.x = (mouseX > maxX) ? maxX : mouseX;

	tooltipPosition.y += (tooltipPosition.y < toolTipSize.y) ? toolTipSize.y : -toolTipSize.y;

	position(tooltipPosition);
	size(toolTipSize);
}


void ToolTip::onMouseMove(NAS2D::Point<int> position, NAS2D::Vector<int> relative)
{
	if (relative != NAS2D::Vector{0, 0})
	{
		if (mFocusedControl)
		{
			if (mFocusedControl->first->rect().contains(position)) { return; }
			else { mFocusedControl = nullptr; }
		}

		mTimer.reset();
	}

	for (auto& item : mControls)
	{
		if (mFocusedControl) { break; }
		if (item.first->rect().contains(position))
		{
			mFocusedControl = &item;
			buildDrawParams(item, position.x);
			return;
		}
	}

	mFocusedControl = nullptr;
}


void ToolTip::update()
{
	if (mTimer.elapsedTicks() < 1000)
	{
		return;
	}
	draw();
}


void ToolTip::draw() const
{
	if (mFocusedControl)
	{
		auto& renderer = NAS2D::Utility<NAS2D::Renderer>::get();
		renderer.drawBoxFilled(rect(), NAS2D::Color::DarkGray);
		renderer.drawBox(rect(), NAS2D::Color::Black);

		auto linePosition = position() + PaddingSize;

		forEachLine(mFocusedControl->second, [this, &renderer, &linePosition](const std::string& lineStr) {
				renderer.drawText(mFont, lineStr, linePosition);
				linePosition.y += mFont.height();
		});
	}
}


void ToolTip::commandSize(int width, int height)
{
	mCommandedWidth = width;
	mCommandedHeight = height;
}


void ToolTip::commandWidth(int width)
{
	mCommandedWidth = width;
}


void ToolTip::commandHeight(int height)
{
	mCommandedHeight = height;
}
