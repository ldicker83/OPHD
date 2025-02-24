#include "MineReport.h"

#include "../TextRender.h"
#include "../ProgressBar.h"

#include "../../Constants/Numbers.h"
#include "../../Constants/Strings.h"
#include "../../Constants/UiConstants.h"

#include "../../Cache.h"
#include "../../Resources.h"
#include "../../StructureManager.h"
#include "../../TruckAvailability.h"
#include "../../ProductionCost.h"

#include "../../States/Route.h"

#include "../../Map/Tile.h"
#include "../../MapObjects/Mine.h"
#include "../../MapObjects/Structures/MineFacility.h"

#include <NAS2D/Utility.h>
#include <NAS2D/Renderer/Renderer.h>

#include <array>
#include <cfloat>
#include <map>


using namespace NAS2D;


namespace
{
	std::string getStructureDescription(const Structure& structure)
	{
		const auto& structureManager = NAS2D::Utility<StructureManager>::get();
		const auto& surfaceLocation = structureManager.tileFromStructure(&structure).xy();
		return structure.name() + " at " + std::string{surfaceLocation};
	}
}


MineReport::MineReport() :
	font{Control::getDefaultFont()},
	fontBold{Control::getDefaultFontBold()},
	fontMedium{fontCache.load(constants::FontPrimary, constants::FontPrimaryMedium)},
	fontMediumBold{fontCache.load(constants::FontPrimaryBold, constants::FontPrimaryMedium)},
	fontBigBold{fontCache.load(constants::FontPrimaryBold, constants::FontPrimaryHuge)},
	mineFacility{imageCache.load("ui/interface/mine.png")},
	uiIcons{imageCache.load("ui/icons.png")},
	btnShowAll{"All", {this, &MineReport::onShowAll}},
	btnShowActive{"Active", {this, &MineReport::onShowActive}},
	btnShowIdle{"Idle", {this, &MineReport::onShowIdle}},
	btnShowTappedOut{"Tapped Out", {this, &MineReport::onShowTappedOut}},
	btnShowDisabled{"Disabled", {this, &MineReport::onShowDisabled}},
	btnIdle{constants::Idle, {this, &MineReport::onIdle}},
	btnDigNewLevel{"Dig New Level", {this, &MineReport::onDigNewLevel}},
	btnTakeMeThere{constants::TakeMeThere, {this, &MineReport::onTakeMeThere}},
	btnAddTruck{constants::AddTruck, {this, &MineReport::onAddTruck}},
	btnRemoveTruck{constants::RemoveTruck, {this, &MineReport::onRemoveTruck}},
	chkResources{{
		{"Mine " + ResourceNamesRefined[0], {this, &MineReport::onCheckBoxCommonMetalsChange}},
		{"Mine " + ResourceNamesRefined[1], {this, &MineReport::onCheckBoxCommonMineralsChange}},
		{"Mine " + ResourceNamesRefined[2], {this, &MineReport::onCheckBoxRareMetalsChange}},
		{"Mine " + ResourceNamesRefined[3], {this, &MineReport::onCheckBoxRareMineralsChange}}
	}},
	mSelectedFacility{nullptr},
	mAvailableTrucks{0}
{
	auto buttonOffset = NAS2D::Vector{10, 10};
	const auto buttons = std::array{&btnShowAll, &btnShowActive, &btnShowIdle, &btnShowTappedOut, &btnShowDisabled};
	for (auto button : buttons)
	{
		button->size({94, 20});
		button->type(Button::Type::Toggle);
		add(*button, buttonOffset);
		buttonOffset.x += button->size().x + constants::MarginTight;
	}

	btnShowAll.toggle(true);

	lstMineFacilities.selectionChanged().connect({this, &MineReport::onMineFacilitySelectionChange});
	add(lstMineFacilities, {10, 40});

	// DETAIL PANE
	btnIdle.type(Button::Type::Toggle);
	btnIdle.size({140, 30});

	btnDigNewLevel.size({140, 30});
	btnTakeMeThere.size({140, 30});

	add(btnIdle, {0, 40});
	add(btnDigNewLevel, {0, 75});
	add(btnTakeMeThere, {0, 110});

	add(chkResources[0], {0, 210});
	add(chkResources[1], {0, 280});
	add(chkResources[2], {0, 350});
	add(chkResources[3], {0, 420});

	// Truck Management Pane
	btnAddTruck.size({140, 30});
	btnRemoveTruck.size({140, 30});

	add(btnAddTruck, {0, 215});
	add(btnRemoveTruck, {0, 250});

	fillLists();
}


void MineReport::selectStructure(Structure* structure)
{
	lstMineFacilities.setSelected(structure);
}


void MineReport::clearSelected()
{
	lstMineFacilities.clearSelected();
	mSelectedFacility = nullptr;
}


void MineReport::refresh()
{
	onShowAll();
}


void MineReport::fillLists()
{
	lstMineFacilities.clear();
	const auto& structureManager = NAS2D::Utility<StructureManager>::get();
	for (auto* facility : structureManager.getStructures<MineFacility>())
	{
		lstMineFacilities.addItem(facility);
		lstMineFacilities.last()->text = getStructureDescription(*facility);
	}

	lstMineFacilities.setSelected(mSelectedFacility);
	mAvailableTrucks = getTruckAvailability();
}


void MineReport::onResize()
{
	Control::onResize();

	lstMineFacilities.size({area().center().x - 20, area().size.y - 51});

	int positionX = area().size.x - 150;
	btnIdle.position({positionX, btnIdle.position().y});
	btnDigNewLevel.position({positionX, btnDigNewLevel.position().y});
	btnTakeMeThere.position({positionX, btnTakeMeThere.position().y});

	auto& renderer = NAS2D::Utility<Renderer>::get();
	btnAddTruck.position({positionX, renderer.size().y - 130});
	btnRemoveTruck.position({positionX, renderer.size().y - 95});

	positionX -= 20;
	chkResources[0].position({positionX, chkResources[0].position().y});
	chkResources[1].position({positionX, chkResources[1].position().y});
	chkResources[2].position({positionX, chkResources[2].position().y});
	chkResources[3].position({positionX, chkResources[3].position().y});
}


void MineReport::onVisibilityChange(bool /*visible*/)
{
	onManagementButtonsVisibilityChange();
}


void MineReport::onManagementButtonsVisibilityChange()
{
	bool isVisible = visible() && mSelectedFacility;

	btnIdle.visible(isVisible);
	btnDigNewLevel.visible(isVisible);
	btnTakeMeThere.visible(isVisible);

	bool isTruckButtonVisible = isVisible &&
		!(mSelectedFacility->destroyed() || mSelectedFacility->underConstruction());

	btnAddTruck.visible(isTruckButtonVisible);
	btnRemoveTruck.visible(isTruckButtonVisible);

	for (auto& chkResource : chkResources)
	{
		chkResource.visible(isVisible);
	}
}


void MineReport::onFilterButtonClicked()
{
	btnShowAll.toggle(false);
	btnShowActive.toggle(false);
	btnShowIdle.toggle(false);
	btnShowTappedOut.toggle(false);
	btnShowDisabled.toggle(false);
}


void MineReport::onShowAll()
{
	onFilterButtonClicked();
	btnShowAll.toggle(true);

	fillLists();
}


void MineReport::onShowActive()
{
	onFilterButtonClicked();
	btnShowActive.toggle(true);
}


void MineReport::onShowIdle()
{
	onFilterButtonClicked();
	btnShowIdle.toggle(true);
}


void MineReport::onShowTappedOut()
{
	onFilterButtonClicked();
	btnShowTappedOut.toggle(true);
}


void MineReport::onShowDisabled()
{
	onFilterButtonClicked();
	btnShowDisabled.toggle(true);
}


void MineReport::onMineFacilitySelectionChange()
{
	mSelectedFacility = dynamic_cast<MineFacility*>(lstMineFacilities.selectedStructure());

	onManagementButtonsVisibilityChange();

	if (!mSelectedFacility) { return; }

	const auto& facility = *mSelectedFacility;
	btnIdle.toggle(facility.isIdle());
	btnIdle.enabled(facility.operational() || facility.isIdle());

	btnDigNewLevel.toggle(facility.extending());
	btnDigNewLevel.enabled(facility.canExtend() && (facility.operational() || facility.isIdle()));

	const auto enabledBits = facility.mine()->miningEnabled();
	chkResources[0].checked(enabledBits[0]);
	chkResources[1].checked(enabledBits[1]);
	chkResources[2].checked(enabledBits[2]);
	chkResources[3].checked(enabledBits[3]);
}


void MineReport::onIdle()
{
	mSelectedFacility->forceIdle(btnIdle.isPressed());
}


void MineReport::onDigNewLevel()
{
	auto& facility = *mSelectedFacility;
	facility.extend();

	btnDigNewLevel.toggle(facility.extending());
	btnDigNewLevel.enabled(facility.canExtend());
}


void MineReport::onTakeMeThere()
{
	mTakeMeThereSignal(mSelectedFacility);
}


void MineReport::onCheckBoxCommonMetalsChange()
{
	mSelectedFacility->mine()->miningEnabled(Mine::OreType::CommonMetals, chkResources[0].checked());
}


void MineReport::onCheckBoxCommonMineralsChange()
{
	mSelectedFacility->mine()->miningEnabled(Mine::OreType::CommonMinerals, chkResources[1].checked());
}


void MineReport::onCheckBoxRareMetalsChange()
{
	mSelectedFacility->mine()->miningEnabled(Mine::OreType::RareMetals, chkResources[2].checked());
}


void MineReport::onCheckBoxRareMineralsChange()
{
	mSelectedFacility->mine()->miningEnabled(Mine::OreType::RareMinerals, chkResources[3].checked());
}


void MineReport::onAddTruck()
{
	if (!mSelectedFacility) { return; }

	auto& facility = *mSelectedFacility;

	if (facility.assignedTrucks() == facility.maxTruckCount()) { return; }

	if (pullTruckFromInventory())
	{
		facility.addTruck();
		mAvailableTrucks = getTruckAvailability();
	}
}


void MineReport::onRemoveTruck()
{
	if (!mSelectedFacility) { return; }

	auto& facility = *mSelectedFacility;

	if (facility.assignedTrucks() == 1) { return; }

	if (pushTruckIntoInventory())
	{
		facility.removeTruck();
		mAvailableTrucks = getTruckAvailability();
	}
}


void MineReport::drawMineFacilityPane(const NAS2D::Point<int>& origin)
{
	auto& r = Utility<Renderer>::get();

	r.drawImage(mineFacility, origin);
	const auto text = lstMineFacilities.isItemSelected() ? getStructureDescription(*lstMineFacilities.selectedStructure()) : "";
	r.drawText(fontBigBold, text, origin + NAS2D::Vector{0, -33}, constants::PrimaryTextColor);

	r.drawText(fontMediumBold, "Status", origin + NAS2D::Vector{138, 0}, constants::PrimaryTextColor);

	bool isStatusHighlighted = mSelectedFacility->disabled() || mSelectedFacility->destroyed();
	auto statusPosition = btnIdle.position() - NAS2D::Vector{fontMedium.width(mSelectedFacility->stateDescription()) + 5, 0};
	r.drawText(fontMedium, mSelectedFacility->stateDescription(), statusPosition, (isStatusHighlighted ? NAS2D::Color::Red : constants::PrimaryTextColor));

	auto resourceTextOrigin = origin + NAS2D::Vector{138, 30};

	const auto& mine = *mSelectedFacility->mine();

	const auto barOrigin = resourceTextOrigin.x + 125;
	const auto barWidth = btnIdle.position().x - barOrigin - 10;

	const auto oreAvailable = mine.availableResources();
	const auto oreTotalYield = mine.totalYield();
	for (size_t i = 0; i < ResourceNamesOre.size(); ++i)
	{
		r.drawText(font, ResourceNamesOre[i], resourceTextOrigin, constants::PrimaryTextColor);

		drawProgressBar(
			oreAvailable.resources[i],
			oreTotalYield.resources[i],
			{{barOrigin, resourceTextOrigin.y}, {barWidth, 12}},
			2
		);
		resourceTextOrigin.y += 20;
	}
}


void MineReport::drawOreProductionPane(const NAS2D::Point<int>& origin)
{
	auto& renderer = Utility<Renderer>::get();
	const auto& mine = *mSelectedFacility->mine();

	renderer.drawText(fontMediumBold, "Ore Production", origin, constants::PrimaryTextColor);
	renderer.drawLine(origin + NAS2D::Vector{0, 21}, {static_cast<float>(renderer.size().x - 10), static_cast<float>(origin.y + 21)}, constants::PrimaryTextColor, 1);

	const auto oreAvailable = mine.availableResources();
	const auto oreTotalYield = mine.totalYield();

	int offsetY = 0;
	const int barWidth = renderer.size().x - origin.x - 10;
	for (size_t i = 0; i < 4; ++i)
	{
		renderer.drawSubImage(uiIcons, origin + NAS2D::Vector{0, 30 + offsetY}, ResourceImageRectsOre[i]);
		renderer.drawText(fontBold, ResourceNamesOre[i], origin + NAS2D::Vector{20, 30 + offsetY}, constants::PrimaryTextColor);

		drawProgressBar(
			oreAvailable.resources[i],
			oreTotalYield.resources[i],
			{{origin.x, origin.y + 50 + offsetY}, {barWidth, 25}}
		);

		const std::string str = std::to_string(oreAvailable.resources[i]) + " of " + std::to_string(oreTotalYield.resources[i]) + " Remaining";
		const int strOffsetX = (barWidth / 2) - (fontBold.width(str) / 2);
		const int strOffsetY = (fontBold.height() / 2) - 1;
		renderer.drawText(fontBold, str, origin + NAS2D::Vector{strOffsetX, 50 + offsetY + strOffsetY});

		offsetY += 70;
	}
}


void MineReport::drawTruckManagementPane(const NAS2D::Point<int>& origin)
{
	const auto miningFacility = mSelectedFacility;

	if (miningFacility->destroyed() ||
		miningFacility->underConstruction())
	{
		return;
	}

	auto& r = Utility<Renderer>::get();
	r.drawText(fontMediumBold, "Trucks & Routing", origin, constants::PrimaryTextColor);
	r.drawLine(origin + NAS2D::Vector{0, 21}, {static_cast<float>(r.size().x - 10), static_cast<float>(origin.y + 21)}, constants::PrimaryTextColor, 1);

	r.drawText(fontBold, "Trucks Assigned to Facility", origin + NAS2D::Vector{0, 30}, constants::PrimaryTextColor);

	const auto labelWidth = btnAddTruck.position().x - origin.x - 10;
	drawLabelAndValueRightJustify(origin + NAS2D::Vector{0, 30}, labelWidth, "Trucks Assigned to Facility", std::to_string(miningFacility->assignedTrucks()), constants::PrimaryTextColor);
	drawLabelAndValueRightJustify(origin + NAS2D::Vector{0, 45}, labelWidth, "Trucks Available in Storage", std::to_string(mAvailableTrucks), constants::PrimaryTextColor);

	auto& routeTable = NAS2D::Utility<std::map<class MineFacility*, Route>>::get();
	bool routeAvailable = routeTable.find(miningFacility) != routeTable.end();

	if (miningFacility->operational() || miningFacility->isIdle())
	{
		drawLabelAndValueRightJustify(origin + NAS2D::Vector{0, 65},
			labelWidth,
			"Route Available",
			routeAvailable ? "Yes" : "No",
			routeAvailable ? constants::PrimaryTextColor : NAS2D::Color::Red);

		if (routeAvailable)
		{
			drawTruckHaulInfo(origin + NAS2D::Vector{0, 80});
		}
	}
}


void MineReport::drawTruckHaulInfo(const NAS2D::Point<int>& origin)
{
	auto& r = Utility<Renderer>::get();
	auto& routeTable = NAS2D::Utility<std::map<class MineFacility*, Route>>::get();
	const auto facility = mSelectedFacility;

	auto& route = routeTable[facility];
	drawLabelAndValueRightJustify(origin,
		btnAddTruck.position().x - origin.x - 10,
		"Route Cost",
		std::to_string(route.cost).substr(0, std::to_string(route.cost).find(".") + 3), // hack-ish and probably slow, this could be cached
		constants::PrimaryTextColor);


	const float routeCost = std::clamp(route.cost, 1.0f, FLT_MAX);
	const int totalOreMovement = static_cast<int>(constants::ShortestPathTraversalCount / routeCost) * facility->assignedTrucks();
	const int oreMovementLabelWidth = r.size().x - origin.x - 10;
	const int oreMovementPart = totalOreMovement / 4;
	const int oreLabelWidth = (oreMovementLabelWidth - 10) / 2;

	const NAS2D::Rectangle<int> tableRect({{origin.x - 2, origin.y + 18}, {oreMovementLabelWidth + 5, 47}});

	r.drawBoxFilled(tableRect, {0, 0, 0, 100});
	r.drawBox(tableRect, constants::PrimaryTextColor);

	r.drawLine(origin + NAS2D::Vector{0, 34}, origin + NAS2D::Vector{oreMovementLabelWidth, 34}, constants::PrimaryTextColor);
	r.drawLine(origin + NAS2D::Vector{0, 50}, origin + NAS2D::Vector{oreMovementLabelWidth, 50}, constants::PrimaryTextColor);
	r.drawLine(origin + NAS2D::Vector{oreLabelWidth + 5, 37}, origin + NAS2D::Vector{oreLabelWidth + 5, 63}, constants::PrimaryTextColor);

	drawLabelAndValueRightJustify(origin + NAS2D::Vector{0, 20},
		oreMovementLabelWidth,
		"Total Haul Capacity per Turn",
		std::to_string(totalOreMovement),
		constants::PrimaryTextColor);

	drawLabelAndValueRightJustify(origin + NAS2D::Vector{0, 35},
		oreLabelWidth,
		ResourceNamesOre[0] + " Haul Capacity",
		std::to_string(oreMovementPart),
		constants::PrimaryTextColor);

	drawLabelAndValueRightJustify(origin + NAS2D::Vector{oreLabelWidth + 10, 35},
		oreLabelWidth,
		ResourceNamesOre[1] + " Haul Capacity",
		std::to_string(oreMovementPart),
		constants::PrimaryTextColor);

	drawLabelAndValueRightJustify(origin + NAS2D::Vector{0, 50},
		oreLabelWidth,
		ResourceNamesOre[2] + " Haul Capacity",
		std::to_string(oreMovementPart),
		constants::PrimaryTextColor);

	drawLabelAndValueRightJustify(origin + NAS2D::Vector{oreLabelWidth + 10, 50},
		oreLabelWidth,
		ResourceNamesOre[3] + " Haul Capacity",
		std::to_string(oreMovementPart + (totalOreMovement % 4)),
		constants::PrimaryTextColor);
}


void MineReport::update()
{
	if (!visible()) { return; }

	auto& r = Utility<Renderer>::get();

	const auto startPoint = NAS2D::Point{area().center().x , area().position.y + 10};

	r.drawLine(startPoint, startPoint + NAS2D::Vector{0, area().size.y - 20}, constants::PrimaryTextColor);

	if (mSelectedFacility)
	{
		drawMineFacilityPane(startPoint + NAS2D::Vector{10, 30});
		drawOreProductionPane(startPoint + NAS2D::Vector{10, 170});
		drawTruckManagementPane(startPoint + NAS2D::Vector{10, r.size().y - 214});
	}

	UIContainer::update();
}
