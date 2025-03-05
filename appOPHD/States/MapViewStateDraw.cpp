// ==================================================================================
// = This file implements the functions that handle processing a turn.
// ==================================================================================

#include "MapViewState.h"

#include "Route.h"

#include "../Constants/UiConstants.h"
#include "../StructureManager.h"
#include "../Map/TileMap.h"
#include "../MapObjects/Mine.h"

#include <NAS2D/Utility.h>
#include <NAS2D/Renderer/Renderer.h>

#include <string>
#include <vector>
#include <algorithm>

extern NAS2D::Point<int> MOUSE_COORDS;


void MapViewState::drawSystemButton() const
{
	auto& renderer = NAS2D::Utility<NAS2D::Renderer>::get();

	auto xOffsetFromRight = 100;
	auto position = NAS2D::Point{renderer.size().x - xOffsetFromRight, constants::MarginTight};
	constexpr auto textOffset = NAS2D::Vector{constants::ResourceIconSize + constants::Margin, 3 - constants::MarginTight};

	// Turns
	const auto turnImageRect = NAS2D::Rectangle<int>{{128, 0}, {constants::ResourceIconSize, constants::ResourceIconSize}};
	renderer.drawSubImage(mUiIcons, position, turnImageRect);
	const auto& font = Control::getDefaultFont();
	renderer.drawText(font, std::to_string(mTurnCount), position + textOffset, NAS2D::Color::White);
}
