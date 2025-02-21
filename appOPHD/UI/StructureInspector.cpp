#include "StructureInspector.h"

#include "../Cache.h"
#include "../Constants/Strings.h"
#include "../MapObjects/Structure.h"
#include "StringTable.h"
#include "TextRender.h"

#include <NAS2D/Utility.h>

#include <stdexcept>


using namespace NAS2D;


namespace
{
	const std::map<DisabledReason, std::string> disabledReasonTable =
	{
		{DisabledReason::None, constants::StructureDisabledNone},

		{DisabledReason::Chap, constants::StructureDisabledChap},
		{DisabledReason::Disconnected, constants::StructureDisabledDisconnected},
		{DisabledReason::Energy, constants::StructureDisabledEnergy},
		{DisabledReason::Population, constants::StructureDisabledPopulation},
		{DisabledReason::RefinedResources, constants::StructureDisabledRefinedResources},
		{DisabledReason::StructuralIntegrity, constants::StructureDisabledStructuralIntegrity}
	};

	const std::map<IdleReason, std::string> idleReadonTable =
	{
		{IdleReason::None, constants::StructureIdleNone},

		{IdleReason::PlayerSet, constants::StructureIdlePlayerSet},
		{IdleReason::InternalStorageFull, constants::StructureIdleInternalStorageFull},
		{IdleReason::FactoryProductionComplete, constants::StructureIdleFactoryProductionComplete},
		{IdleReason::FactoryInsufficientResources, constants::StructureIdleFactoryInsufficientResources},
		{IdleReason::FactoryInsufficientRobotCommandCapacity, constants::StructureIdleFactoryInsufficientRobotCommandCapacity},
		{IdleReason::FactoryInsufficientWarehouseSpace, constants::StructureIdleFactoryInsufficnetWarehouseCapacity},
		{IdleReason::MineExhausted, constants::StructureIdleMineExhausted},
		{IdleReason::MineInactive, constants::StructureIdleMineInactive},
		{IdleReason::InsufficientLuxuryProduct, constants::StructureIdleInsufficientLuxuryProduct}
	};


	const std::string& disabledReasonToString(DisabledReason disabledReason)
	{
		return disabledReasonTable.at(disabledReason);
	}


	const std::string& idleReasonToString(IdleReason idleReason)
	{
		return idleReadonTable.at(idleReason);
	}


	std::string getDisabledReason(const Structure& structure)
	{
		if (structure.disabled())
		{
			return disabledReasonToString(structure.disabledReason());
		}
		else if (structure.isIdle())
		{
			return idleReasonToString(structure.idleReason());
		}

		return "";
	}


	std::string formatAge(const Structure& structure)
	{
		return structure.ages() ? std::to_string(structure.age()) + " of " + std::to_string(structure.maxAge()) : "N/A";
	}


	StringTable buildGenericStructureAttributesStringTable(const Structure& structure)
	{
		StringTable stringTable{4, 6};

		stringTable[{0, 0}].text = "Type:";
		stringTable[{1, 0}].text = structure.classDescription();

		if (structure.underConstruction())
		{
			stringTable[{2, 0}].text = "Turns Remaining:";
			stringTable[{3, 0}].text = std::to_string(structure.turnsToBuild() - structure.age());
		}
		else
		{
			stringTable[{2, 0}].text = "Age:";
			stringTable[{3, 0}].text = formatAge(structure);
		}

		stringTable[{0, 1}].text = "Power Required:";
		stringTable[{1, 1}].text = std::to_string(structure.energyRequirement());

		stringTable[{2, 1}].text = "State:";
		stringTable[{3, 1}].text = structure.stateDescription(structure.state());

		stringTable[{3, 2}].text = getDisabledReason(structure);

		if (!structure.underConstruction() && !structure.destroyed())
		{
			stringTable[{0, 2}].text = "Integrity:";
			stringTable[{1, 2}].text = std::to_string(structure.integrity());
		}

		const auto& populationAvailable = structure.populationAvailable();
		const auto& populationRequirements = structure.populationRequirements();

		if (populationRequirements.workers > 0)
		{
			stringTable[{0, 3}].text = "Workers:";
			stringTable[{1, 3}].text = std::to_string(populationAvailable.workers) + " / " + std::to_string(populationRequirements.workers);
			stringTable[{1, 3}].textColor = populationAvailable.workers >= populationRequirements.workers ? Color::White : Color::Red;
		}

		if (populationRequirements.scientists > 0)
		{
			stringTable[{0, 4}].text = "Scientists:";
			stringTable[{1, 4}].text = std::to_string(populationAvailable.scientists) + " / " + std::to_string(populationRequirements.scientists);
			stringTable[{1, 4}].textColor = populationAvailable.scientists >= populationRequirements.scientists ? Color::White : Color::Red;
		}

		if (structure.hasCrime())
		{
			stringTable[{0, 5}].text = "Crime Rate:";
			stringTable[{1, 5}].text = std::to_string(structure.crimeRate()) + "%";
		}

		return stringTable;
	}
}


StructureInspector::StructureInspector() :
	Window{constants::WindowStructureInspector},
	btnClose{"Close", {this, &StructureInspector::onClose}},
	mIcons{imageCache.load("ui/icons.png")}
{
	size({350, 250});

	btnClose.size({50, 20});
	add(btnClose, {rect().size.x - btnClose.rect().size.x - 5, rect().size.y - btnClose.rect().size.y - 5,});
}


void StructureInspector::structure(Structure* structure)
{
	mStructure = structure;

	if (!mStructure) { return; }

	auto stringTable = buildStringTable();

	auto windowWidth = stringTable.screenRect().size.x + 10;
	size({windowWidth < 350 ? 350 : windowWidth, rect().size.y});

	btnClose.position({positionX() + rect().size.x - 55, btnClose.positionY()});
}


void StructureInspector::onClose()
{
	visible(false);
}


StringTable StructureInspector::buildStringTable() const
{
	auto stringTable = buildGenericStructureAttributesStringTable(*mStructure);
	stringTable.position(mRect.position + NAS2D::Vector{5, 25});
	stringTable.setVerticalPadding(5);
	stringTable.setColumnFont(2, stringTable.GetDefaultTitleFont());
	stringTable.computeRelativeCellPositions();
	return stringTable;
}


void StructureInspector::update()
{
	if (!visible()) { return; }
	Window::update();

	auto& renderer = Utility<Renderer>::get();

	if (mStructure == nullptr)
	{
		throw std::runtime_error("Null pointer to structure within StructureInspector");
	}
	title(mStructure->name());

	const auto genericStructureAttributes = buildStringTable();
	genericStructureAttributes.draw(renderer);

	const auto specificAttributeTablePosition = genericStructureAttributes.screenRect().crossYPoint() + NAS2D::Vector{0, 25};
	StringTable specificStructureAttributes = mStructure->createInspectorViewTable();
	specificStructureAttributes.computeRelativeCellPositions();
	specificStructureAttributes.position(specificAttributeTablePosition);
	specificStructureAttributes.draw(renderer);
}
