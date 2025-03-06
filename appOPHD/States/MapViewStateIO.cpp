// ==================================================================================
// = This file implements the File Input/Outpost used by MapViewState. I separated it
// = into its own file because the MapViewState.cpp file was starting to get a little
// = out of control.
// ==================================================================================

#include "MapViewState.h"
#include "ColonyShip.h"

#include "MapViewStateHelper.h"

#include "Route.h"

#include "../Cache.h"
#include "../OpenSaveGame.h"
#include "../MeanSolarDistance.h"
#include "../Constants/Strings.h"
#include "../IOHelper.h"
#include "../ProductCatalogue.h"
#include "../StructureCatalogue.h"
#include "../StructureManager.h"
#include "../Map/TileMap.h"
#include "../Map/MapView.h"

#include "../UI/DetailMap.h"
#include "../UI/NavControl.h"

#include <libOPHD/Technology/ResearchTracker.h>
#include <libOPHD/XmlSerializer.h>

#include <NAS2D/Utility.h>
#include <NAS2D/Filesystem.h>
#include <NAS2D/StringUtils.h>
#include <NAS2D/Xml/XmlDocument.h>
#include <NAS2D/Xml/XmlMemoryBuffer.h>
#include <NAS2D/Dictionary.h>
#include <NAS2D/ParserHelper.h>
#include <NAS2D/ContainerUtils.h>

#include <string>
#include <vector>
#include <stdexcept>


namespace
{
	const std::map<std::string, Difficulty> difficultyTable
	{
		{"Beginner", Difficulty::Beginner},
		{"Easy", Difficulty::Easy},
		{"Medium", Difficulty::Medium},
		{"Hard", Difficulty::Hard}
	};


	std::string difficultyEnumToString(Difficulty difficulty)
	{
		for (const auto& difficultyPair : difficultyTable)
		{
			if (difficultyPair.second == difficulty)
			{
				return difficultyPair.first;
			}
		}

		throw std::runtime_error("Provided difficulty does not exist in the difficultyMap");
	}


	Difficulty difficultyStringToEnum(const std::string& value)
	{
		return stringToEnum(difficultyTable, value);
	}


	MapCoordinate loadMapCoordinate(const NAS2D::Dictionary& dictionary)
	{
		const auto x = dictionary.get<int>("x");
		const auto y = dictionary.get<int>("y");
		const auto depth = dictionary.get<int>("depth");
		return MapCoordinate{{x, y}, depth};
	}


	NAS2D::Xml::XmlElement* writeResearch(const ResearchTracker& tracker)
	{
		auto* research = new NAS2D::Xml::XmlElement("research");

		const auto intToStr = [](const auto& x){return std::to_string(x);};
		const auto completedResearch = NAS2D::join(NAS2D::mapToVector(tracker.completedResearch(), intToStr), ",");

		research->attribute("completed_techs", completedResearch);

		for (const auto& [techId, values] : tracker.currentResearch())
		{
			research->linkEndChild(NAS2D::dictionaryToAttributes(
				"current",
				{{
					{"tech_id", techId},
					{"progress", values.progress},
					{"assigned", values.scientistsAssigned},
				}}
			));
		}

		return research;
	}


	ResearchTracker readResearch(NAS2D::Xml::XmlElement* element)
	{
		ResearchTracker tracker;

		if (!element) { return tracker; }

		const auto researchList = NAS2D::split(element->attribute("completed_techs"));

		for (auto& item : researchList)
		{
			tracker.addCompletedResearch(std::stoi(item));
		}

		for (auto currentResearch = element->firstChildElement();
			currentResearch != nullptr;
			currentResearch = currentResearch->nextSiblingElement())
		{
			const auto dictionary = NAS2D::attributesToDictionary(*currentResearch);

			tracker.startResearch(
				dictionary.get<int>("tech_id"),
				dictionary.get<int>("progress"),
				dictionary.get<int>("assigned")
			);
		}

		return tracker;
	}
}


void MapViewState::save(const std::string& filePath)
{
	auto& renderer = NAS2D::Utility<NAS2D::Renderer>::get();
	renderer.drawBoxFilled(NAS2D::Rectangle{{0, 0}, renderer.size()}, NAS2D::Color{0, 0, 0, 100});
	const auto imageSaving = &imageCache.load("sys/saving.png");
	renderer.drawImage(*imageSaving, renderer.center() - imageSaving->size() / 2);
	renderer.update();

	NAS2D::Xml::XmlDocument doc;

	auto* root = NAS2D::dictionaryToAttributes(
		constants::SaveGameRootNode,
		{{{"version", constants::SaveGameVersion}}}
	);
	doc.linkEndChild(root);

	root->linkEndChild(serializeProperties());
	mTileMap->serialize(root);
	mMapView->serialize(root);
	root->linkEndChild(NAS2D::Utility<StructureManager>::get().serialize());
	root->linkEndChild(mRobotPool.writeRobots(mRobotList));
	root->linkEndChild(writeResources(mResourceBreakdownPanel.previousResources(), "prev_resources"));
	root->linkEndChild(writeResearch(mResearchTracker));
	root->linkEndChild(NAS2D::dictionaryToAttributes("turns", {{{"count", mTurnCount}}}));

	const auto& population = mPopulation.getPopulations();
	root->linkEndChild(NAS2D::dictionaryToAttributes(
		"population",
		{{
			{"morale", mMorale.currentMorale()},
			{"prev_morale", mMorale.previousMorale()},
			{"colonist_landers", mColonyShip.colonistLanders()},
			{"cargo_landers", mColonyShip.cargoLanders()},
			{"turn_number_of_landing", mTurnNumberOfLanding},
			{"children", population.child},
			{"students", population.student},
			{"workers", population.worker},
			{"scientists", population.scientist},
			{"retired", population.retiree},
			{"mean_crime", mPopulationPanel.crimeRate()},
		}}
	));

	auto moraleChangeReasons = new NAS2D::Xml::XmlElement("morale_change");
	auto& moraleChangeList = mMorale.moraleChangeJournal();
	for (auto& [message, value] : moraleChangeList)
	{
		moraleChangeReasons->linkEndChild(NAS2D::dictionaryToAttributes(
			"change", {{{"message", message}, {"val", value}}}
		));
	}
	root->linkEndChild(moraleChangeReasons);

	// Write out the XML file.
	NAS2D::Xml::XmlMemoryBuffer buff;
	doc.accept(&buff);

	NAS2D::Utility<NAS2D::Filesystem>::get().writeFile(filePath, buff.buffer());
}


NAS2D::Xml::XmlElement* MapViewState::serializeProperties()
{
	return NAS2D::dictionaryToAttributes(
		"properties",
		{{
			{"sitemap", mPlanetAttributes.mapImagePath},
			{"tset", mPlanetAttributes.tilesetPath},
			{"diggingdepth", mPlanetAttributes.maxDepth},
			{"meansolardistance", mPlanetAttributes.meanSolarDistance},
			{"difficulty", difficultyEnumToString(difficulty())},
		}}
	);
}


void MapViewState::load(const std::string& filePath)
{
	resetUi();

	auto& renderer = NAS2D::Utility<NAS2D::Renderer>::get();
	renderer.drawBoxFilled(NAS2D::Rectangle{{0, 0}, renderer.size()}, NAS2D::Color{0, 0, 0, 100});
	const auto imageLoading = &imageCache.load("sys/loading.png");
	renderer.drawImage(*imageLoading, renderer.center() - imageLoading->size() / 2);
	renderer.update();

	mBtnToggleConnectedness.toggle(false);
	mBtnToggleCommRangeOverlay.toggle(false);
	mBtnToggleRouteOverlay.toggle(false);
	mBtnTogglePoliceOverlay.toggle(false);
	mBtnToggleHeightmap.toggle(false);
	mMorale.closeJournal();

	if (!NAS2D::Utility<NAS2D::Filesystem>::get().exists(filePath))
	{
		throw std::runtime_error("File '" + filePath + "' was not found.");
	}

	scrubRobotList();
	NAS2D::Utility<StructureManager>::get().dropAllStructures();
	ccLocation() = CcNotPlaced;

	mStructureTracker = StructureTracker{};

	mTileMap.reset();

	auto xmlDocument = openSavegame(filePath);
	auto* root = xmlDocument.firstChildElement(constants::SaveGameRootNode);

	NAS2D::Xml::XmlElement* map = root->firstChildElement("properties");
	const auto dictionary = NAS2D::attributesToDictionary(*map);

	mPlanetAttributes = Planet::Attributes();
	mPlanetAttributes.maxDepth = dictionary.get<int>("diggingdepth");
	mPlanetAttributes.mapImagePath = dictionary.get("sitemap");
	mPlanetAttributes.tilesetPath = dictionary.get("tset");
	mPlanetAttributes.meanSolarDistance = dictionary.get<float>("meansolardistance");

	setMeanSolarDistance(mPlanetAttributes.meanSolarDistance);

	difficulty(difficultyStringToEnum(dictionary.get("difficulty", std::string{"Medium"})));

	mTileMap = std::make_unique<TileMap>(mPlanetAttributes.mapImagePath, mPlanetAttributes.maxDepth);
	mTileMap->deserialize(root);
	mMapView = std::make_unique<MapView>(*mTileMap);
	mMapView->deserialize(root);
	mMiniMap = std::make_unique<MiniMap>(*mMapView, *mTileMap, mRobotList, mPlanetAttributes.mapImagePath);
	mDetailMap = std::make_unique<DetailMap>(*mMapView, *mTileMap, mPlanetAttributes.tilesetPath);
	mNavControl = std::make_unique<NavControl>(*mMapView);

	mPathSolver = std::make_unique<micropather::MicroPather>(mTileMap.get(), 250, 6, false);
	auto& routeTable = NAS2D::Utility<std::map<class MineFacility*, Route>>::get();
	routeTable.clear();

	readRobots(root->firstChildElement("robots"));
	readStructures(root->firstChildElement("structures"));

	mResearchTracker = readResearch(root->firstChildElement("research"));

	mResourceBreakdownPanel.previousResources() = readResources(*root, "prev_resources");
	readPopulation(root->firstChildElement("population"));
	readTurns(root->firstChildElement("turns"));

	readMoraleChanges(root->firstChildElement("morale_change"));

	updateConnectedness();

	NAS2D::Utility<StructureManager>::get().updateEnergyProduction();
	NAS2D::Utility<StructureManager>::get().updateEnergyConsumed();
	NAS2D::Utility<StructureManager>::get().assignColonistsToResidences(mPopulationPool);

	mRobotPool.update();
	updateResidentialCapacity();
	updateStructuresAvailability();

	updateRoads();
	findMineRoutes();
	updateFood();
	updatePlayerResources();
	updateResearch();

	if (mTurnCount == 0)
	{
		if (NAS2D::Utility<StructureManager>::get().count() == 0)
		{
			mBtnTurns.enabled(false);
			populateStructureMenu();
		}
		else
		{
			/**
			 * There should only ever be one structure if the turn count is 0, the
			 * SEED Lander which at this point should not have been deployed.
			 */
			const auto& list = NAS2D::Utility<StructureManager>::get().getStructures<SeedLander>();
			if (list.size() != 1) { throw std::runtime_error("MapViewState::load(): Turn counter at 0 but more than one structure in list."); }

			SeedLander* seedLander = list[0];
			if (!seedLander) { throw std::runtime_error("MapViewState::load(): Structure in list is not a SeedLander."); }

			seedLander->deploySignal().connect({this, &MapViewState::onDeploySeedLander});

			mStructures.clear();
			mConnections.clear();
			mBtnTurns.enabled(true);
		}
	}
	else
	{
		populateStructureMenu();
	}

	updateCommRangeOverlay();
	mPoliceOverlays.resize(static_cast<std::vector<Tile*>::size_type>(mTileMap->maxDepth()+1));
	updatePoliceOverlay();

	mMapChangedSignal();
}


void MapViewState::readRobots(NAS2D::Xml::XmlElement* element)
{
	mRobotPool.clear();
	mRobotList.clear();
	mRobots.clear();

	for (NAS2D::Xml::XmlElement* robotElement = element->firstChildElement(); robotElement; robotElement = robotElement->nextSiblingElement())
	{
		const auto dictionary = NAS2D::attributesToDictionary(*robotElement);

		const auto type = dictionary.get<int>("type");
		const auto age = dictionary.get<int>("age");
		const auto production_time = dictionary.get<int>("production");
		const auto x = dictionary.get<int>("x", 0);
		const auto y = dictionary.get<int>("y", 0);
		const auto depth = dictionary.get<int>("depth", 0);
		const auto direction = dictionary.get<int>("direction", 0);

		const auto robotType = static_cast<Robot::Type>(type);
		auto& robot = addRobot(robotType);
		if (robotType == Robot::Type::Digger)
		{
			static_cast<Robodigger&>(robot).direction(static_cast<Direction>(direction));
		}

		robot.fuelCellAge(age);

		if (production_time > 0)
		{
			robot.startTask(production_time);
			mRobotPool.insertRobotIntoTable(mRobotList, robot, mTileMap->getTile({{x, y}, depth}));
			mRobotList[&robot]->index(TerrainType::Dozed);
		}

		if (depth > 0)
		{
			mRobotList[&robot]->excavated(true);
		}
	}

	populateRobotMenu();
}


void MapViewState::readStructures(NAS2D::Xml::XmlElement* element)
{
	for (NAS2D::Xml::XmlElement* structureElement = element->firstChildElement(); structureElement != nullptr; structureElement = structureElement->nextSiblingElement())
	{
		const auto dictionary = NAS2D::attributesToDictionary(*structureElement);

		const auto type = dictionary.get<int>("type");
		const auto age = dictionary.get<int>("age");
		const auto state = dictionary.get<int>("state");
		const auto direction = dictionary.get<int>("direction");
		const auto forced_idle = dictionary.get<bool>("forced_idle");
		const auto disabled_reason = dictionary.get<int>("disabled_reason");
		const auto idle_reason = dictionary.get<int>("idle_reason");

		const auto crime_rate = dictionary.get<int>("crime_rate", 0);
		const auto integrity = dictionary.get<int>("integrity", 100);

		const auto production_completed = dictionary.get<int>("production_completed", 0);
		const auto production_type = dictionary.get<int>("production_type", 0);

		const auto pop0 = dictionary.get<int>("pop0");
		const auto pop1 = dictionary.get<int>("pop1");

		const auto mapCoordinate = loadMapCoordinate(dictionary);
		auto& tile = mTileMap->getTile(mapCoordinate);
		tile.index(TerrainType::Dozed);
		tile.excavated(true);

		auto structureId = static_cast<StructureID>(type);
		if (structureId == StructureID::SID_TUBE)
		{
			ConnectorDir connectorDir = static_cast<ConnectorDir>(direction);
			insertTube(connectorDir, mapCoordinate.z, mTileMap->getTile(mapCoordinate));
			continue; // FIXME: ugly
		}

		auto& structure = *StructureCatalogue::get(structureId, &tile);

		if (structureId == StructureID::SID_COLONIST_LANDER)
		{
			static_cast<ColonistLander*>(&structure)->deploySignal().connect({this, &MapViewState::onDeployColonistLander});
		}

		if (structureId == StructureID::SID_CARGO_LANDER)
		{
			static_cast<CargoLander*>(&structure)->deploySignal().connect({this, &MapViewState::onDeployCargoLander});
		}

		if (structureId == StructureID::SID_COMMAND_CENTER)
		{
			ccLocation() = mapCoordinate.xy;
		}

		if (structureId == StructureID::SID_MINE_FACILITY)
		{
			auto* mine = mTileMap->getTile({mapCoordinate.xy, 0}).mine();
			if (mine == nullptr)
			{
				throw std::runtime_error("Mine Facility is located on a Tile with no Mine.");
			}

			auto& mineFacility = *static_cast<MineFacility*>(&structure);
			mineFacility.mine(mine);
			mineFacility.maxDepth(mTileMap->maxDepth());
			mineFacility.extensionComplete().connect({this, &MapViewState::onMineFacilityExtend});

			auto trucks = structureElement->firstChildElement("trucks");
			if (trucks)
			{
				mineFacility.assignedTrucks(NAS2D::attributesToDictionary(*trucks).get<int>("assigned"));
			}

			auto extension = structureElement->firstChildElement("extension");
			if (extension)
			{
				mineFacility.digTimeRemaining(NAS2D::attributesToDictionary(*extension).get<int>("turns_remaining"));
			}
		}

		if (structureId == StructureID::SID_AIR_SHAFT && mapCoordinate.z != 0)
		{
			static_cast<AirShaft*>(&structure)->ug(); // force underground state
		}

		if (structureId == StructureID::SID_SEED_LANDER)
		{
			static_cast<SeedLander*>(&structure)->position(mapCoordinate.xy);
		}

		if (structureId == StructureID::SID_AGRIDOME ||
			structureId == StructureID::SID_COMMAND_CENTER)
		{
			auto& foodProduction = *static_cast<FoodProduction*>(&structure);

			auto foodStorage = structureElement->firstChildElement("food");
			if (foodStorage == nullptr)
			{
				throw std::runtime_error("MapViewState::readStructures(): FoodProduction structure saved without a food level node.");
			}

			foodProduction.foodLevel(NAS2D::attributesToDictionary(*foodStorage).get<int>("level"));
		}

		structure.age(age);
		structure.forced_state_change(static_cast<StructureState>(state), static_cast<DisabledReason>(disabled_reason), static_cast<IdleReason>(idle_reason));
		structure.connectorDirection(static_cast<ConnectorDir>(direction));
		structure.integrity(integrity);

		if (forced_idle != 0) { structure.forceIdle(forced_idle != 0); }

		structure.production() = readResourcesOptional(*structureElement, "production");
		structure.storage() = readResourcesOptional(*structureElement, "storage");

		if (structure.structureClass() == Structure::StructureClass::Residence)
		{
			auto waste = structureElement->firstChildElement("waste");
			if (waste)
			{
				auto& residence = *static_cast<Residence*>(&structure);
				const auto wasteDictionary = NAS2D::attributesToDictionary(*waste);
				residence.wasteAccumulated(wasteDictionary.get<int>("accumulated"));
				residence.wasteOverflow(wasteDictionary.get<int>("overflow"));
			}
		}

		if (structure.structureClass() == Structure::StructureClass::Maintenance)
		{
			auto personnel = structureElement->firstChildElement("personnel");
			if (personnel)
			{
				auto& maintenanceFacility = *static_cast<MaintenanceFacility*>(&structure);
				maintenanceFacility.personnel(NAS2D::attributesToDictionary(*personnel).get<int>("assigned", 0));
				maintenanceFacility.resources(mResourcesCount);
			}
		}

		if (structure.isWarehouse())
		{
			auto& warehouse = *static_cast<Warehouse*>(&structure);
			warehouse.products().deserialize(NAS2D::attributesToDictionary(
				*structureElement->firstChildElement("warehouse_products")
			));
		}

		if (structure.isFactory())
		{
			auto& factory = *static_cast<Factory*>(&structure);
			factory.productType(static_cast<ProductType>(production_type));
			factory.productionTurnsCompleted(production_completed);
			factory.resourcePool(&mResourcesCount);
			factory.productionComplete().connect({this, &MapViewState::onFactoryProductionComplete});
		}

		if (structure.hasCrime())
		{
			structure.crimeRate(crime_rate);
		}

		structure.populationAvailable() = {pop0, pop1};

		NAS2D::Utility<StructureManager>::get().addStructure(structure, tile);
	}
}


void MapViewState::readTurns(NAS2D::Xml::XmlElement* element)
{
	if (element)
	{
		mTurnCount = NAS2D::attributesToDictionary(*element).get<int>("count");

		if (mTurnCount > 0)
		{
			mBtnTurns.enabled(true);
			populateStructureMenu();
		}
	}
}


/**
 * Reads the population tag.
 */
void MapViewState::readPopulation(NAS2D::Xml::XmlElement* element)
{
	if (element)
	{
		mPopulation = {};

		const auto dictionary = NAS2D::attributesToDictionary(*element);

		mMorale = Morale(dictionary.get<int>("morale"), dictionary.get<int>("prev_morale"));

		mTurnNumberOfLanding = dictionary.get<int>("turn_number_of_landing", constants::ColonyShipOrbitTime);

		const auto meanCrimeRate = dictionary.get<int>("mean_crime", 0);

		const auto children = dictionary.get<int>("children");
		const auto students = dictionary.get<int>("students");
		const auto workers = dictionary.get<int>("workers");
		const auto scientists = dictionary.get<int>("scientists");
		const auto retired = dictionary.get<int>("retired");

		mPopulationPanel.crimeRate(meanCrimeRate);

		mPopulation.addPopulation({children, students, workers, scientists, retired});
	}
}


void MapViewState::readMoraleChanges(NAS2D::Xml::XmlElement* moraleChangeElement)
{
	if (!moraleChangeElement) { return; }

	for (auto messageElement = moraleChangeElement->firstChildElement(); messageElement; messageElement = messageElement->nextSiblingElement())
	{
		const auto dictionary = NAS2D::attributesToDictionary(*messageElement);

		const auto message = dictionary.get("message");
		const auto val = dictionary.get<int>("val");

		mMorale.journalMoraleChange({message, val});
	}
}
