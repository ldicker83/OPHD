// ==================================================================================
// = This file implements the functions that handle processing a turn.
// ==================================================================================

#include "MapViewState.h"
#include "ColonyShip.h"

#include "MapViewStateHelper.h"

#include "Route.h"

#include "../Map/TileMap.h"

#include "../Cache.h"
#include "../MoraleString.h"
#include "../StorableResources.h"
#include "../StructureManager.h"
#include "../Constants/Strings.h"

#include <libOPHD/DirectionOffset.h>
#include <libOPHD/EnumMoraleIndex.h>

#include <NAS2D/Utility.h>
#include <NAS2D/Renderer/Renderer.h>

#include <vector>
#include <algorithm>

namespace
{
	const std::map<std::string, IconGrid::Item> StructureItemFromString =
	{
		{"SID_FUSION_REACTOR", {constants::FusionReactor, 21, SID_FUSION_REACTOR}},
		{"SID_SOLAR_PLANT", {constants::SolarPlant, 10, StructureID::SID_SOLAR_PLANT}}
	};


	const std::map<std::array<bool, 4>, std::string> IntersectionPatternTable =
	{
		{{true, false, true, false}, "left"},
		{{true, false, false, false}, "left"},
		{{false, false, true, false}, "left"},

		{{false, true, false, true}, "right"},
		{{false, true, false, false}, "right"},
		{{false, false, false, true}, "right"},

		{{false, false, false, false}, "intersection"},
		{{true, true, false, false}, "intersection"},
		{{false, false, true, true}, "intersection"},
		{{false, true, true, true}, "intersection"},
		{{true, true, true, false}, "intersection"},
		{{true, true, true, true}, "intersection"},
		{{true, false, false, true}, "intersection"},
		{{false, true, true, false}, "intersection"},

		{{false, true, true, true}, "intersection"},
		{{true, false, true, true}, "intersection"},
		{{true, true, false, true}, "intersection"},
		{{true, true, true, false}, "intersection"}
	};


	std::string roadAnimationName(int integrity, const std::array<bool, 4>& surroundingTiles)
	{
		std::string tag = "";

		if (integrity == 0) { tag = "-destroyed"; }
		else if (integrity < constants::RoadIntegrityChange) { tag = "-decayed"; }

		return IntersectionPatternTable.at(surroundingTiles) + tag;
	}


	auto getSurroundingRoads(const TileMap& tileMap, const NAS2D::Point<int>& tileLocation)
	{
		std::array<bool, 4> surroundingTiles{false, false, false, false};
		for (size_t i = 0; i < 4; ++i)
		{
			const auto tileToInspect = tileLocation + DirectionClockwise4[i];
			const auto surfacePosition = MapCoordinate{tileToInspect, 0};
			if (!tileMap.isValidPosition(surfacePosition)) { continue; }
			const auto& tile = tileMap.getTile(surfacePosition);
			if (!tile.thingIsStructure()) { continue; }

			surroundingTiles[i] = tile.structure()->structureId() == StructureID::SID_ROAD;
		}
		return surroundingTiles;
	}


	int consumeFood(FoodProduction& producer, int amountToConsume)
	{
		const auto foodLevel = producer.foodLevel();
		const auto toTransfer = std::min(foodLevel, amountToConsume);

		producer.foodLevel(foodLevel - toTransfer);
		return toTransfer;
	}


	void consumeFood(const std::vector<FoodProduction*>& foodProducers, int amountToConsume)
	{
		for (auto* foodProducer : foodProducers)
		{
			if (amountToConsume <= 0) { break; }
			amountToConsume -= consumeFood(*foodProducer, amountToConsume);
		}
	}


	RouteList findRoutes(micropather::MicroPather* solver, Structure* mine, const std::vector<OreRefining*>& smelters)
	{
		auto& structureManager = NAS2D::Utility<StructureManager>::get();
		auto& start = structureManager.tileFromStructure(mine);

		RouteList routeList;

		for (const auto* smelter : smelters)
		{
			if (!smelter->operational()) { continue; }

			auto& end = structureManager.tileFromStructure(smelter);

			Route route;
			solver->Reset();
			solver->Solve(&start, &end, &route.path, &route.cost);

			if (!route.empty()) { routeList.push_back(route); }
		}

		return routeList;
	}


	Route findLowestCostRoute(RouteList& routeList)
	{
		if (routeList.empty()) { return Route(); }

		std::sort(routeList.begin(), routeList.end(), [](const Route& a, const Route& b) { return a.cost < b.cost; });
		return routeList.front();
	}


	bool routeObstructed(Route& route)
	{
		for (auto tileVoidPtr : route.path)
		{
			auto& tile = *static_cast<Tile*>(tileVoidPtr);

			// \note	Tile being occupied by a robot is not an obstruction for the
			//			purposes of routing/pathing.
			if (tile.thingIsStructure() && !tile.structure()->isRoad()) { return true; }
			if (tile.index() == TerrainType::Impassable) { return true; }
		}

		return false;
	}
}


void MapViewState::updatePopulation()
{
	StructureManager& structureManager = NAS2D::Utility<StructureManager>::get();

	int residences = structureManager.getCountInState(Structure::StructureClass::Residence, StructureState::Operational);
	int universities = structureManager.getCountInState(Structure::StructureClass::University, StructureState::Operational);
	int nurseries = structureManager.getCountInState(Structure::StructureClass::Nursery, StructureState::Operational);
	int hospitals = structureManager.getCountInState(Structure::StructureClass::MedicalCenter, StructureState::Operational);

	auto foodProducers = structureManager.getStructures<FoodProduction>();
	const auto& commandCenters = structureManager.getStructures<CommandCenter>();
	foodProducers.insert(foodProducers.end(), commandCenters.begin(), commandCenters.end());

	int amountToConsume = mPopulation.update(mMorale.currentMorale(), mFood, residences, universities, nurseries, hospitals);
	consumeFood(foodProducers, amountToConsume);
}


void MapViewState::updateCommercial()
{
	StructureManager& structureManager = NAS2D::Utility<StructureManager>::get();

	const auto& warehouses = structureManager.getStructures<Warehouse>();
	const auto& commercial = structureManager.getStructures<Commercial>();

	// No need to do anything if there are no commercial structures.
	if (commercial.empty()) { return; }

	int luxuryCount = structureManager.getCountInState(Structure::StructureClass::Commercial, StructureState::Operational);
	int commercialCount = luxuryCount;

	for (auto* warehouse : warehouses)
	{
		ProductPool& productPool = warehouse->products();

		/**
		 * inspect for luxury products.
		 *
		 * FIXME: I feel like this could be done better. At the moment there
		 * is only one luxury item, clothing, but as this changes more
		 * items may be seen as luxury.
		 */
		int clothing = productPool.count(ProductType::PRODUCT_CLOTHING);

		if (clothing >= luxuryCount)
		{
			productPool.pull(ProductType::PRODUCT_CLOTHING, luxuryCount);
			luxuryCount = 0;
			break;
		}
		else if (clothing < luxuryCount)
		{
			productPool.pull(ProductType::PRODUCT_CLOTHING, clothing);
			luxuryCount -= clothing;
		}

		if (luxuryCount == 0)
		{
			break;
		}
	}

	auto commercialReverseIterator = commercial.rbegin();
	for (std::size_t i = 0; i < static_cast<std::size_t>(luxuryCount) && commercialReverseIterator != commercial.rend(); ++i, ++commercialReverseIterator)
	{
		if ((*commercialReverseIterator)->operational())
		{
			(*commercialReverseIterator)->idle(IdleReason::InsufficientLuxuryProduct);
		}
	}

	mMorale.journalMoraleChange({"Commercial - Luxury", commercialCount - luxuryCount});
}


void MapViewState::updateMorale()
{
	StructureManager& structureManager = NAS2D::Utility<StructureManager>::get();

	// POSITIVE MORALE EFFECTS
	// =========================================
	const int birthCount = mPopulation.birthCount();
	const int parkCount = structureManager.getCountInState(Structure::StructureClass::Park, StructureState::Operational);
	const int recreationCount = structureManager.getCountInState(Structure::StructureClass::RecreationCenter, StructureState::Operational);
	const int foodProducingStructures = structureManager.getCountInState(Structure::StructureClass::FoodProduction, StructureState::Operational);
	const int commercialCount = structureManager.getCountInState(Structure::StructureClass::Commercial, StructureState::Operational);

	// NEGATIVE MORALE EFFECTS
	// =========================================
	const int deathCount = mPopulation.deathCount();
	const int structuresDisabled = structureManager.disabled();
	const int structuresDestroyed = structureManager.destroyed();
	const int residentialOverCapacityHit = mPopulation.getPopulations().size() > mResidentialCapacity ? 2 : 0;
	const int foodProductionHit = foodProducingStructures > 0 ? 0 : 5;

	const auto& residences = NAS2D::Utility<StructureManager>::get().getStructures<Residence>();
	int bioWasteAccumulation = 0;
	for (const auto* residence : residences)
	{
		if (residence->wasteOverflow() > 0) { ++bioWasteAccumulation; }
	}

	// positive
	mMorale.journalMoraleChange({moraleString(MoraleIndexs::Births), birthCount});
	mMorale.journalMoraleChange({moraleString(MoraleIndexs::Parks), parkCount});
	mMorale.journalMoraleChange({moraleString(MoraleIndexs::Recreation), recreationCount});
	mMorale.journalMoraleChange({moraleString(MoraleIndexs::Commercial), commercialCount});

	// negative
	mMorale.journalMoraleChange({moraleString(MoraleIndexs::Deaths), -deathCount});
	mMorale.journalMoraleChange({moraleString(MoraleIndexs::ResidentialOverflow), -residentialOverCapacityHit});
	mMorale.journalMoraleChange({moraleString(MoraleIndexs::BiowasteOverflow), -bioWasteAccumulation * 2}); // TODO 2 is a magic number
	mMorale.journalMoraleChange({moraleString(MoraleIndexs::StructuresDisabled), -structuresDisabled});
	mMorale.journalMoraleChange({moraleString(MoraleIndexs::StructuresDestroyed), -structuresDestroyed});
	mMorale.journalMoraleChange({"Food Production Issues", -foodProductionHit});


	for (const auto& moraleReason : mCrimeRateUpdate.moraleChanges())
	{
		mMorale.journalMoraleChange({moraleReason.first, moraleReason.second});
	}

	mPopulationPanel.crimeRate(mCrimeRateUpdate.meanCrimeRate());

	for (const auto& moraleReason : mCrimeExecution.moraleChanges())
	{
		mMorale.journalMoraleChange({moraleReason.first, moraleReason.second});
	}
}


void MapViewState::notifyBirthsAndDeaths()
{
	const int birthCount = mPopulation.birthCount();
	const int deathCount = mPopulation.deathCount();

	// Push notifications
	if (birthCount)
	{
		mNotificationArea.push({
			"Baby Born",
			std::to_string(birthCount) + (birthCount > 1 ? " babies were born." : " baby was born."),
			{{-1, -1}, 0},
			NotificationArea::NotificationType::Information});
	}

	if (deathCount)
	{
		mNotificationArea.push({
			"Colonist Died",
			std::to_string(deathCount) + (birthCount > 1 ? " colonists met their demise." : " colonist met their demise."),
			{{-1, -1}, 0},
			NotificationArea::NotificationType::Warning});
	}
}


void MapViewState::findMineRoutes()
{
	const auto& smelterList = NAS2D::Utility<StructureManager>::get().getStructures<OreRefining>();
	auto& routeTable = NAS2D::Utility<std::map<class MineFacility*, Route>>::get();
	mTruckRouteOverlay.clear();

	for (auto* mine : NAS2D::Utility<StructureManager>::get().getStructures<MineFacility>())
	{
		if (!mine->operational() && !mine->isIdle()) { continue; } // consider a different control path.

		auto routeIt = routeTable.find(mine);
		bool findNewRoute = routeIt == routeTable.end();

		if (!findNewRoute && routeObstructed(routeIt->second))
		{
			routeTable.erase(mine);
			findNewRoute = true;
		}

		if (findNewRoute)
		{
			auto routeList = findRoutes(mPathSolver.get(), mine, smelterList);
			auto newRoute = findLowestCostRoute(routeList);

			if (newRoute.empty()) { continue; } // give up and move on to the next mine

			routeTable[mine] = newRoute;

			for (auto tile : newRoute.path)
			{
				mTruckRouteOverlay.push_back(static_cast<Tile*>(tile));
			}
		}
	}
}


void MapViewState::transportOreFromMines()
{
	auto& routeTable = NAS2D::Utility<std::map<class MineFacility*, Route>>::get();
	for (auto* mine : NAS2D::Utility<StructureManager>::get().getStructures<MineFacility>())
	{
		auto routeIt = routeTable.find(mine);
		if (routeIt != routeTable.end())
		{
			const auto& route = routeIt->second;
			auto& smelter = *static_cast<OreRefining*>(static_cast<Tile*>(route.path.back())->structure());
			auto& mineFacility = *static_cast<MineFacility*>(static_cast<Tile*>(route.path.front())->structure());

			if (!smelter.operational()) { break; }

			/* clamp route cost to minimum of 1.0f for next computation to avoid
			   unintended multiplication. */
			const float routeCost = std::clamp(routeIt->second.cost, 1.0f, FLT_MAX);

			/* intentional truncation of fractional component*/
			const int totalOreMovement = static_cast<int>(constants::ShortestPathTraversalCount / routeCost) * mineFacility.assignedTrucks();
			const int oreMovementPart = totalOreMovement / 4;
			const int oreMovementRemainder = totalOreMovement % 4;
			const auto movementCap = StorableResources{oreMovementPart, oreMovementPart, oreMovementPart, oreMovementPart + oreMovementRemainder};

			auto& mineStored = mineFacility.storage();
			auto& smelterStored = smelter.production();

			const auto oreAvailable = smelterStored + mineStored.cap(movementCap);
			const auto newSmelterStored = oreAvailable.cap(250);
			const auto movedOre = newSmelterStored - smelterStored;

			mineStored -= movedOre;
			smelterStored = newSmelterStored;
		}
	}
}


void MapViewState::transportResourcesToStorage()
{
	const auto& smelterList = NAS2D::Utility<StructureManager>::get().getStructures<OreRefining>();
	for (auto* smelter : smelterList)
	{
		if (!smelter->operational() && !smelter->isIdle()) { continue; }

		auto& stored = smelter->storage();
		const auto toMove = stored.cap(25);

		const auto unmoved = addRefinedResources(toMove);
		stored -= (toMove - unmoved);
	}
}


void MapViewState::updateResources()
{
	findMineRoutes();
	transportOreFromMines();
	transportResourcesToStorage();
	updatePlayerResources();
}


void MapViewState::checkWarehouseCapacity()
{
	StructureManager& structureManager = NAS2D::Utility<StructureManager>::get();
	const auto& warehouses = structureManager.getStructures<Warehouse>();

	if (warehouses.size() == 0) { return; } // no divisions by zero, pl0x

	int availableStorageTotal = 0;
	for (const auto warehouse : warehouses)
	{
		availableStorageTotal += warehouse->products().availableStoragePercent();
	}

	const int availableStorage = availableStorageTotal / static_cast<int>(warehouses.size());

	if (availableStorage == 0) // FIXME -- Magic Number
	{
		mNotificationArea.push({
			"No Warehouse Space",
			"You are out of storage space at your warehouses! Your Factories will go idle until you build more Warehouses or reduce inventory.",
			{{-1, -1}, 0},
			NotificationArea::NotificationType::Critical
		});
	}
	else if (availableStorage < 5) // FIXME -- Ditto
	{
		mNotificationArea.push({
			"Warehouse Space Critically Low",
			"Warehouse space is critically low! You only have " + std::to_string(availableStorage) + "% storage capacity remaining!",
			{{-1, -1}, 0},
			NotificationArea::NotificationType::Critical
		});
	}
	else if (availableStorage < 15) // FIXME -- Ditto
	{
		mNotificationArea.push({
			"Warehouse Space Low",
			"Warehouse space is running low. Current available storage capacity is at " + std::to_string(availableStorage) + "%.",
			{{-1, -1}, 0},
			NotificationArea::NotificationType::Warning
		});
	}
}


void MapViewState::updateResidentialCapacity()
{
	mResidentialCapacity = 0;
	const auto& residences = NAS2D::Utility<StructureManager>::get().getStructures<Residence>();
	for (const auto* residence : residences)
	{
		if (residence->operational()) { mResidentialCapacity += residence->capacity(); }
	}

	if (residences.empty()) { mResidentialCapacity = constants::CommandCenterPopulationCapacity; }

	mPopulationPanel.residentialCapacity(mResidentialCapacity);
}


void MapViewState::updateBiowasteRecycling()
{
	const auto& residences = NAS2D::Utility<StructureManager>::get().getStructures<Residence>();
	const auto& recyclingFacilities = NAS2D::Utility<StructureManager>::get().getStructures<Recycling>();

	if (residences.empty() || recyclingFacilities.empty()) { return; }

	auto residenceIterator = residences.begin();
	for (const auto* recycling : recyclingFacilities)
	{
		if (!recycling->operational()) { continue; } // Consider a different control structure

		for (int count = 0; count < recycling->residentialSupportCount(); ++count)
		{
			if (residenceIterator == residences.end())
			{
				return; // No more residences, so don't waste time iterating over remaining recycling facilities
			}

			Residence* residence = static_cast<Residence*>(*residenceIterator);
			residence->pullWaste(recycling->wasteProcessingCapacity());
			++residenceIterator;
		}
	}
}


void MapViewState::updateFood()
{
	mFood = 0;

	auto foodProducers = NAS2D::Utility<StructureManager>::get().getStructures<FoodProduction>();
	const auto& command = NAS2D::Utility<StructureManager>::get().getStructures<CommandCenter>();

	foodProducers.insert(foodProducers.begin(), command.begin(), command.end());

	for (const auto* foodProdcer : foodProducers)
	{
		if (foodProdcer->operational() || foodProdcer->isIdle())
		{
			mFood += foodProdcer->foodLevel();
		}
	}
}


void MapViewState::transferFoodToCommandCenter()
{
	const auto& foodProducers = NAS2D::Utility<StructureManager>::get().getStructures<FoodProduction>();
	const auto& commandCenters = NAS2D::Utility<StructureManager>::get().getStructures<CommandCenter>();

	auto foodProducerIterator = foodProducers.begin();
	for (auto* commandCenter : commandCenters)
	{
		if (!commandCenter->operational()) { continue; }

		int foodToMove = commandCenter->foodCapacity() - commandCenter->foodLevel();

		while (foodProducerIterator != foodProducers.end())
		{
			auto foodProducer = static_cast<FoodProduction*>(*foodProducerIterator);
			const int foodMoved = std::clamp(foodToMove, 0, foodProducer->foodLevel());
			foodProducer->foodLevel(foodProducer->foodLevel() - foodMoved);
			commandCenter->foodLevel(commandCenter->foodLevel() + foodMoved);

			foodToMove -= foodMoved;

			if (foodToMove == 0) { break; }

			++foodProducerIterator;
		}
	}
}


/**
 * Update road intersection patterns
 */
void MapViewState::updateRoads()
{
	const auto& roads = NAS2D::Utility<StructureManager>::get().getStructures<Road>();

	for (auto* road : roads)
	{
		if (!road->operational()) { continue; }

		const auto tileLocation = NAS2D::Utility<StructureManager>::get().tileFromStructure(road).xy();
		const auto surroundingTiles = getSurroundingRoads(*mTileMap, tileLocation);

		road->sprite().play(roadAnimationName(road->integrity(), surroundingTiles));
	}
}


void MapViewState::checkAgingStructures()
{
	const auto& structures = NAS2D::Utility<StructureManager>::get().agingStructures();

	for (const auto* structure : structures)
	{
		const auto& structureTile = NAS2D::Utility<StructureManager>::get().tileFromStructure(structure);

		if (structure->age() == structure->maxAge() - 10)
		{
			mNotificationArea.push({
				"Aging Structure",
				structure->name() + " is getting old. You should replace it soon.",
				structureTile.xyz(),
				NotificationArea::NotificationType::Warning});
		}
		else if (structure->age() == structure->maxAge() - 5)
		{
			mNotificationArea.push({
				"Aging Structure",
				structure->name() + " is about to collapse. You should replace it right away or consider demolishing it.",
				structureTile.xyz(),
				NotificationArea::NotificationType::Critical});
		}
	}
}


void MapViewState::checkNewlyBuiltStructures()
{
	const auto& structures = NAS2D::Utility<StructureManager>::get().newlyBuiltStructures();

	for (const auto* structure : structures)
	{
		const auto& structureTile = NAS2D::Utility<StructureManager>::get().tileFromStructure(structure);

		mNotificationArea.push({
			"Construction Finished",
			structure->name() + " completed construction.",
			structureTile.xyz(),
			NotificationArea::NotificationType::Success});
	}
}


void MapViewState::updateMaintenance()
{
	auto sortLambda = [](const Structure* lhs, const Structure* rhs) -> bool
	{
		return lhs->integrity() < rhs->integrity();
	};

	auto& structureManager = NAS2D::Utility<StructureManager>::get();
	auto structures = structureManager.allStructures();
	std::sort(structures.begin(), structures.end(), sortLambda);

	const auto& maintenanceFacilities = structureManager.getStructures<MaintenanceFacility>();
	for (auto* maintenanceFacility : maintenanceFacilities)
	{
		maintenanceFacility->repairStructures(structures);
	}
}


void MapViewState::updateOverlays()
{
	updateCommRangeOverlay();
	updatePoliceOverlay();

	if (mBtnToggleConnectedness.isPressed()) { onToggleConnectedness(); }
	if (mBtnToggleCommRangeOverlay.isPressed()) { onToggleCommRangeOverlay(); }
	if (mBtnToggleRouteOverlay.isPressed()) { onToggleRouteOverlay(); }
	if (mBtnTogglePoliceOverlay.isPressed()) { onTogglePoliceOverlay(); }
}


void MapViewState::updateResearch()
{
	// Update research points
	// get list of completed technologies
	const auto& completedTechs = mResearchTracker.completedResearch();
	std::vector<const Technology*> techList;
	for (const auto techId : completedTechs)
	{
		techList.push_back(&mTechnologyReader.technologyFromId(techId));
	}

	// get list of completed technologies that unlock buildings
	std::vector<const Technology*> unlockedStructures;

	for (auto tech : techList)
	{
		for (const auto& unlock : (*tech).unlocks)
		{
			if (unlock.unlocks == Technology::Unlock::Unlocks::Structure)
			{
				unlockedStructures.push_back(tech);
			}
		}
	}

	for (const auto& tech : unlockedStructures)
	{
		for (const auto& unlock : tech->unlocks)
		{
			const auto& structureItem = StructureItemFromString.at(unlock.value);
			mStructureTracker.addUnlockedSurfaceStructure(structureItem);
		}
	}

	// remove obsolete structures from available structure list
}


void MapViewState::nextTurn()
{
	auto& renderer = NAS2D::Utility<NAS2D::Renderer>::get();
	const auto imageProcessingTurn = &imageCache.load("sys/processing_turn.png");
	renderer.drawImage(*imageProcessingTurn, renderer.center() - imageProcessingTurn->size() / 2);
	renderer.update();

	mNotificationWindow.hide();
	mNotificationArea.clear();

	mMorale.closeJournal();

	clearMode();

	mPopulationPool.clear();

	mResourceBreakdownPanel.previousResources(mResourcesCount);

	updateConnectedness();
	NAS2D::Utility<StructureManager>::get().update(mResourcesCount, mPopulationPool);

	checkAgingStructures();
	checkNewlyBuiltStructures();

	transferFoodToCommandCenter();

	updateResidentialCapacity();

	// Colony will not have morale or crime effects until at least n turns from landing, depending on difficulty
	bool isMoraleEnabled = mTurnCount > mTurnNumberOfLanding + GracePeriod.at(mDifficulty);

	if (isMoraleEnabled)
	{
		mCrimeRateUpdate.update(mPoliceOverlays);
		auto structuresCommittingCrimes = mCrimeRateUpdate.structuresCommittingCrimes();
		mCrimeExecution.executeCrimes(structuresCommittingCrimes);
	}

	updateFood();
	updatePopulation();

	updateMaintenance();
	updateCommercial();
	updateBiowasteRecycling();

	if (isMoraleEnabled)
	{
		updateMorale();
	}

	notifyBirthsAndDeaths();

	updateRobots();
	updateResources();
	updateStructuresAvailability();
	updateRoads();

	updateOverlays();

	const auto& factories = NAS2D::Utility<StructureManager>::get().getStructures<Factory>();
	for (auto* factory : factories)
	{
		factory->updateProduction();
	}

	updateResearch();

	mColonyShip.updateColonyShip(mDifficulty, mMorale, mWindowStack, mAnnouncement);
	checkWarehouseCapacity();

	mMineOperationsWindow.updateTruckAvailability();

	// Check for Game Over conditions
	if (mPopulation.getPopulations().size() <= 0 && mColonyShip.colonistLanders() == 0)
	{
		hideUi();
		mGameOverDialog.show();
	}

	populateRobotMenu();
	populateStructureMenu();

	mMorale.commitMoraleChanges();

	mTurnCount++;

	mResourceInfoBar.ignoreGlow(false);
}
