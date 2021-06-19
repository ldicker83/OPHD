#include "CrimeExecution.h"
#include "../StructureManager.h"
#include <NAS2D/StringUtils.h>
#include <NAS2D/Utility.h>


CrimeExecution::CrimeExecution(NotificationArea& notificationArea) : mNotificationArea(notificationArea) {}


void CrimeExecution::executeCrimes(const std::vector<Structure*>& structuresCommittingCrime)
{
	for (auto& structure : structuresCommittingCrime)
	{
		if (structure == nullptr) 
		{
			continue;
		}

		switch (structure->structureId())
		{
		case StructureID::SID_AGRIDOME:
			stealFood(static_cast<FoodProduction&>(*structure));
			break;
		case StructureID::SID_SMELTER:
			stealRawResources(*structure);
			break;
		case StructureID::SID_STORAGE_TANKS:
			stealRefinedResources(*structure);
			break;
		default:
			break;
		}
	}
}


void CrimeExecution::stealFood(FoodProduction& structure)
{
	if (structure.foodLevel() > 0)
	{
		int foodStolen = 100;
		if (foodStolen > structure.foodLevel())
		{
			foodStolen = structure.foodLevel();
		}
		
		structure.foodLevel(-foodStolen);

		const auto& structureTile = NAS2D::Utility<StructureManager>::get().tileFromStructure(&structure);
		
		mNotificationArea.push("Food Stolen",
			NAS2D::stringFrom(foodStolen) + " units of food was pilfered from a " + structure.name() + ".",
			structureTile.position(),
			NotificationArea::NotificationType::Warning);
	}
}


void CrimeExecution::stealRefinedResources(Structure& structure)
{
	stealResources(structure, ResourceNamesRefined);
}


void CrimeExecution::stealRawResources(Structure& structure)
{
	stealResources(structure, ResourceNamesOre);
}


void CrimeExecution::stealResources(Structure& structure, const std::array<std::string, 4>& resourceNames)
{
	if (structure.storage().isEmpty())
	{
		return;
	}

	auto resourceIndicesWithStock = structure.storage().getIndicesWithStock();

	// TODO: Rob randomly from stock, for now it just robs from first available indice with resources
	auto indexToStealFrom = resourceIndicesWithStock[0];

	int amountStolen = 100;
	if (amountStolen > structure.storage().resources[indexToStealFrom])
	{
		amountStolen = structure.storage().resources[indexToStealFrom];
	}

	structure.storage().resources[0] -= amountStolen;

	const auto& structureTile = NAS2D::Utility<StructureManager>::get().tileFromStructure(&structure);

	mNotificationArea.push("Resources Stolen",
		NAS2D::stringFrom(amountStolen) + " units of " + resourceNames[indexToStealFrom] + " were stolen from a " + structure.name() + ".",
		structureTile.position(),
		NotificationArea::NotificationType::Warning);
}
