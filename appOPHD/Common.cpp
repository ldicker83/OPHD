#include "Common.h"
#include "Constants/Numbers.h"
#include "EnumMineProductionRate.h"
#include "EnumMoraleIndex.h"
#include "EnumTerrainType.h"
#include "StructureManager.h"

#include "MapObjects/Structure.h"
#include "MapObjects/Structures/Warehouse.h"

#include <libOPHD/XmlSerializer.h>

#include <NAS2D/Utility.h>
#include <NAS2D/Xml/XmlDocument.h>
#include <NAS2D/Xml/XmlElement.h>

#include <stdexcept>
#include <algorithm>


using namespace NAS2D;


namespace
{
	float meanSolarDistance = 1;
}


const std::map<StructureState, Color> STRUCTURE_COLOR_TABLE
{
	{StructureState::UnderConstruction, Color{150, 150, 150, 100}},
	{StructureState::Operational, Color{0, 185, 0}},
	{StructureState::Idle, Color{0, 185, 0, 100}},
	{StructureState::Disabled, Color{220, 0, 0}},
	{StructureState::Destroyed, Color{220, 0, 0}}
};


const std::map<StructureState, Color> STRUCTURE_TEXT_COLOR_TABLE
{
	{StructureState::UnderConstruction, Color{185, 185, 185, 100}},
	{StructureState::Operational, Color{0, 185, 0}},
	{StructureState::Idle, Color{0, 185, 0, 100}},
	{StructureState::Disabled, Color{220, 0, 0}},
	{StructureState::Destroyed, Color{220, 0, 0}}
};


const std::map<TerrainType, std::string> TILE_INDEX_TRANSLATION =
{
	{TerrainType::Dozed, constants::TileBulldozed},
	{TerrainType::Clear, constants::TileClear},
	{TerrainType::Rough, constants::TileRough},
	{TerrainType::Difficult, constants::TileDifficult},
	{TerrainType::Impassable, constants::TileImpassable},
};


const std::map<MineProductionRate, std::string> MINE_YIELD_TRANSLATION =
{
	{MineProductionRate::High, constants::MineYieldHigh},
	{MineProductionRate::Low, constants::MineYieldLow},
	{MineProductionRate::Medium, constants::MineYieldMedium}
};


const std::array MoraleStringTable =
{
	std::string("Terrible"),
	std::string("Poor"),
	std::string("Fair"),
	std::string("Good"),
	std::string("Excellent"),

	std::string("Morale is "),
	std::string("Births"),
	std::string("Deaths"),
	std::string("No active Food Production"),
	std::string("Parks & Arboretums"),
	std::string("Recreational Facilities"),
	std::string("Luxury Availability"),
	std::string("Residential Over Capacity"),
	std::string("Biowaste Overflowing"),
	std::string("Structures Disabled"),
	std::string("Structures Destroyed")
};


const std::string& moraleString(std::size_t index)
{
	return MoraleStringTable[index];
}


const std::string& moraleString(MoraleIndexs morale)
{
	return MoraleStringTable[static_cast<std::size_t>(morale)];
}


std::size_t moraleStringTableCount()
{
	return MoraleStringTable.size();
}


const std::array<std::string, 4> ResourceNamesRefined =
{
	{"Common Metals", "Common Minerals", "Rare Metals", "Rare Minerals"}
};


const std::array<std::string, 4> ResourceNamesOre =
{
	{"Common Metals Ore", "Common Minerals Ore", "Rare Metals Ore", "Rare Minerals Ore"}
};


const std::array<NAS2D::Rectangle<int>, 4> ResourceImageRectsRefined =
{
	NAS2D::Rectangle<int>{{64, 16}, {16, 16}},
	NAS2D::Rectangle<int>{{96, 16}, {16, 16}},
	NAS2D::Rectangle<int>{{80, 16}, {16, 16}},
	NAS2D::Rectangle<int>{{112, 16}, {16, 16}},
};


const std::array<NAS2D::Rectangle<int>, 4> ResourceImageRectsOre =
{
	NAS2D::Rectangle<int>{{64, 0}, {16, 16}},
	NAS2D::Rectangle<int>{{96, 0}, {16, 16}},
	NAS2D::Rectangle<int>{{80, 0}, {16, 16}},
	NAS2D::Rectangle<int>{{112, 0}, {16, 16}},
};


Color structureColorFromIndex(StructureState structureState)
{
	return STRUCTURE_COLOR_TABLE.at(structureState);
}


Color structureTextColorFromIndex(StructureState structureState)
{
	return STRUCTURE_TEXT_COLOR_TABLE.at(structureState);
}


void checkSavegameVersion(const std::string& filename)
{
	// openSavegame checks version number after opening file
	openSavegame(filename);
}


/**
 * Open a saved game and validate version.
 *
 * \throws	Throws a std::runtime_error if there are any errors with a savegame version, formation or missing root nodes.
 */
NAS2D::Xml::XmlDocument openSavegame(const std::string& filename)
{
	auto xmlDocument = openXmlFile(filename, constants::SaveGameRootNode);

	auto savegameVersion = xmlDocument.firstChildElement(constants::SaveGameRootNode)->attribute("version");

	if (savegameVersion != constants::SaveGameVersion)
	{
		throw std::runtime_error("Savegame version mismatch: '" + filename + "'. Expected " + constants::SaveGameVersion + ", found " + savegameVersion + ".");
	}

	return xmlDocument;
}


void setMeanSolarDistance(float newMeanSolarDistance)
{
	if (newMeanSolarDistance <= 0)
	{
		throw std::runtime_error("Must set a positive value for `meanSolarDistance`: " + std::to_string(newMeanSolarDistance));
	}
	meanSolarDistance = newMeanSolarDistance;
}


float getMeanSolarDistance()
{
	return meanSolarDistance;
}


void drawProgressBar(int value, int max, NAS2D::Rectangle<int> rect, int padding)
{
	const auto clippedValue = std::clamp(value, 0, max);
	auto& renderer = Utility<Renderer>::get();
	renderer.drawBox(rect, NAS2D::Color{0, 185, 0});

	if (max > 0)
	{
		auto innerRect = rect.inset(padding);
		innerRect.size.x *= clippedValue / max;
		renderer.drawBoxFilled(innerRect, NAS2D::Color{0, 100, 0});
	}
}


int getTruckAvailability()
{
	int trucksAvailable = 0;

	const auto& warehouseList = NAS2D::Utility<StructureManager>::get().getStructures<Warehouse>();
	for (auto* warehouse : warehouseList)
	{
		trucksAvailable += warehouse->products().count(ProductType::PRODUCT_TRUCK);
	}

	return trucksAvailable;
}


int pullTruckFromInventory()
{
	int trucksAvailable = getTruckAvailability();

	if (trucksAvailable == 0) { return 0; }

	const auto& warehouseList = NAS2D::Utility<StructureManager>::get().getStructures<Warehouse>();
	for (auto* warehouse : warehouseList)
	{
		if (warehouse->products().pull(ProductType::PRODUCT_TRUCK, 1) > 0)
		{
			return 1;
		}
	}

	return 0;
}


int pushTruckIntoInventory()
{
	const int storageNeededForTruck = storageRequiredPerUnit(ProductType::PRODUCT_TRUCK);

	const auto& warehouseList = NAS2D::Utility<StructureManager>::get().getStructures<Warehouse>();
	for (auto* warehouse : warehouseList)
	{
		if (warehouse->products().availableStorage() >= storageNeededForTruck)
		{
			warehouse->products().store(ProductType::PRODUCT_TRUCK, 1);
			return 1;
		}
	}

	return 0;
}
