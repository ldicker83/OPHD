#include "TextRender.h"

#include "../Cache.h"
#include "../Constants/UiConstants.h"

#include <NAS2D/Resource/Font.h>
#include <NAS2D/Renderer/Renderer.h>
#include <NAS2D/Utility.h>

#include <libControls/Control.h>


void drawLabelAndValue(NAS2D::Point<int> position, const std::string& title, const std::string& text, NAS2D::Color color)
{
	auto& renderer = NAS2D::Utility<NAS2D::Renderer>::get();

	const auto& font = Control::getDefaultFont();
	const auto& fontBold = Control::getDefaultFontBold();

	renderer.drawText(fontBold, title, position, color);
	position.x += fontBold.width(title);
	renderer.drawText(font, text, position, color);
}

void drawLabelAndValueLeftJustify(NAS2D::Point<int> position, int labelWidth, const std::string& title, const std::string& text, NAS2D::Color color)
{
	auto& renderer = NAS2D::Utility<NAS2D::Renderer>::get();

	const auto& font = Control::getDefaultFont();
	const auto& fontBold = Control::getDefaultFontBold();

	renderer.drawText(fontBold, title, position, color);
	position.x += labelWidth;
	renderer.drawText(font, text, position, color);
}

void drawLabelAndValueRightJustify(NAS2D::Point<int> position, int labelWidth, const std::string& title, const std::string& text, NAS2D::Color color)
{
	auto& renderer = NAS2D::Utility<NAS2D::Renderer>::get();

	const auto& font = Control::getDefaultFont();
	const auto& fontBold = Control::getDefaultFontBold();

	renderer.drawText(fontBold, title, position, color);
	position.x += labelWidth - font.width(text);
	renderer.drawText(font, text, position, color);
}
