#include "StructureCatalogue.h"
#include "StructureManager.h"

#include <string>
#include <stdexcept>


std::array<StorableResources, StructureTypeID::SID_COUNT> StructureCatalogue::mStructureCostTable;
std::array<StorableResources, StructureTypeID::SID_COUNT> StructureCatalogue::mStructureRecycleValueTable;
std::array<PopulationRequirements, StructureTypeID::SID_COUNT> StructureCatalogue::mPopulationRequirementsTable = {};

/**	Default recycle value. Currently set at 90% but this should probably be
 *	lowered for actual gameplay with modifiers to improve efficiency. */
const float DEFAULT_RECYCLE_VALUE = 0.9f;


/**
 * Gets a new Structure object given a StructureTypeID.
 * 
 * \param	type	A valid StructureTypeID value.
 * 
 * \return	Pointer to a newly constructed Structure or
 *			\c nullptr if structure type unsupported/invalid.
 */
Structure* StructureCatalogue::get(StructureTypeID type)
{
	Structure* structure = nullptr;

	// This seems like a naive approach... I usually see these implemented as the base
	// object type has a static function that is used as an interface to instantiate
	// derived types.
	// TODO: (#843) Pulling component-based structure component in here for now.
	//       This class has friend access to Structure and can therefore call the various
	//       initialization functions. This will be replaced by a generic factory function
	//       that deserializes descriptions from the structure catalog and save files.
	switch (type)
	{
		case StructureTypeID::SID_AGRIDOME:
		{
			structure = new Structure(
				constants::AGRIDOME,
				"structures/agridome.sprite",
				Structure::StructureClass::FoodProduction,
				StructureTypeID::SID_AGRIDOME);
			structure->maxAge(600);
			structure->turnsToBuild(3);
			structure->requiresCHAP(true);
			structure->resourcesIn({ 1, 0, 0, 0 });
			structure->energyRequired(2);

			const int AGRIDOME_CAPACITY = 1000;
			const int AGRIDOME_BASE_PRODUCUCTION = 10;
			FoodProduction* foodProduction = new FoodProduction(structure);
			foodProduction->initialize(AGRIDOME_BASE_PRODUCUCTION, AGRIDOME_CAPACITY);
			NAS2D::Utility<StructureManager>::get().attachComponent(structure, foodProduction);
			break;
		}

		case StructureTypeID::SID_AIR_SHAFT:
			structure = new AirShaft();
			break;

		case StructureTypeID::SID_CARGO_LANDER: // only here for loading games
			structure = new CargoLander(nullptr);
			break;

		case StructureTypeID::SID_CHAP:
			structure = new CHAP();
			break;

		case StructureTypeID::SID_COLONIST_LANDER: // only here for loading games
			structure = new ColonistLander(nullptr);
			break;

		case StructureTypeID::SID_COMMAND_CENTER:
		{
			structure = new Structure(
				constants::COMMAND_CENTER,
				"structures/command_center.sprite",
				Structure::StructureClass::Command,
				StructureTypeID::SID_COMMAND_CENTER);
			structure->maxAge(500);
			structure->turnsToBuild(4);
			structure->requiresCHAP(false);
			structure->selfSustained(true);
			structure->storageCapacity(constants::BASE_STORAGE_CAPACITY);

			FoodProduction* foodProduction = new FoodProduction(structure);
			foodProduction->initialize(0, constants::BASE_STORAGE_CAPACITY);
			NAS2D::Utility<StructureManager>::get().attachComponent(structure, foodProduction);
			break;
		}

		case StructureTypeID::SID_COMMERCIAL:
			structure = new Commercial();
			break;

		case StructureTypeID::SID_COMM_TOWER:
			structure = new CommTower();
			break;

		case StructureTypeID::SID_FUSION_REACTOR:
		{
			structure = new Structure(
				constants::FUSION_REACTOR,
				"structures/fusion_reactor.sprite",
				Structure::StructureClass::EnergyProduction,
				StructureTypeID::SID_FUSION_REACTOR);
			structure->maxAge(1000);
			structure->turnsToBuild(10);
			structure->requiresCHAP(false);
			structure->resourcesIn({ 2, 2, 1, 1 });

			const int FUSION_REACTOR_BASE_PRODUCUCTION = 1000;
			auto power = new PowerStructure(structure);
			power->initialize(FUSION_REACTOR_BASE_PRODUCUCTION, 0.0f);
			NAS2D::Utility<StructureManager>::get().attachComponent(structure, power);
			break;
		}

		case StructureTypeID::SID_HOT_LABORATORY:
			structure = new HotLaboratory();
			break;

		case StructureTypeID::SID_LABORATORY:
			structure = new Laboratory();
			break;

		case StructureTypeID::SID_MEDICAL_CENTER:
			structure = new MedicalCenter();
			break;

		case StructureTypeID::SID_MINE_FACILITY: // only here for loading games
			structure = new MineFacility(nullptr);
			break;

		case StructureTypeID::SID_MINE_SHAFT: // only here for loading games
			structure = new MineShaft();
			break;

		case StructureTypeID::SID_NURSERY:
			structure = new Nursery();
			break;

		case StructureTypeID::SID_PARK:
			structure = new Park();
			break;

		case StructureTypeID::SID_ROAD:
			structure = new Road();
			break;

		case StructureTypeID::SID_SURFACE_POLICE:
			structure = new SurfacePolice();
			break;

		case StructureTypeID::SID_UNDERGROUND_POLICE:
			structure = new UndergroundPolice();
			break;

		case StructureTypeID::SID_RECREATION_CENTER:
			structure = new RecreationCenter();
			break;

		case StructureTypeID::SID_RECYCLING:
			structure = new Recycling();
			break;

		case StructureTypeID::SID_RED_LIGHT_DISTRICT:
			structure = new RedLightDistrict();
			break;

		case StructureTypeID::SID_RESIDENCE:
			structure = new Residence();
			break;

		case StructureTypeID::SID_ROBOT_COMMAND:
			structure = new RobotCommand();
			break;

		case StructureTypeID::SID_SEED_FACTORY:
			structure = new SeedFactory();
			break;

		case StructureTypeID::SID_SEED_LANDER: // only here for loading games
			structure = new SeedLander({0, 0});
			break;

		case StructureTypeID::SID_SEED_POWER:
		{
			structure = new Structure(
				constants::SEED_POWER,
				"structures/seed_1.sprite",
				Structure::StructureClass::EnergyProduction,
				StructureTypeID::SID_SEED_POWER);
			structure->maxAge(150);
			structure->turnsToBuild(5);
			structure->requiresCHAP(false);

			const int SEED_POWER_PRODUCTION = 50;
			auto power = new PowerStructure(structure);
			power->initialize(SEED_POWER_PRODUCTION, 0.0f);
			NAS2D::Utility<StructureManager>::get().attachComponent(structure, power);
			break;
		}

		case StructureTypeID::SID_SEED_SMELTER:
			structure = new SeedSmelter();
			break;

		case StructureTypeID::SID_SMELTER:
			structure = new Smelter();
			break;

		case StructureTypeID::SID_SOLAR_PANEL1:
		{
			structure = new Structure(
				constants::SOLAR_PANEL1,
				"structures/solar_array1.sprite",
				Structure::StructureClass::EnergyProduction,
				StructureTypeID::SID_SOLAR_PANEL1);
			structure->maxAge(1000);
			structure->turnsToBuild(4);
			structure->requiresCHAP(false);

			const int SOLAR_PANEL_BASE_PRODUCUCTION = 50;
			auto power = new PowerStructure(structure);
			power->initialize(0, SOLAR_PANEL_BASE_PRODUCUCTION);
			NAS2D::Utility<StructureManager>::get().attachComponent(structure, power);
			break;
		}

		case StructureTypeID::SID_SOLAR_PLANT:
		{
			structure = new Structure(
				constants::SOLAR_PLANT,
				"structures/solar_plant.sprite",
				Structure::StructureClass::EnergyProduction,
				StructureTypeID::SID_SOLAR_PLANT);
			structure->maxAge(1000);
			structure->turnsToBuild(4);
			structure->requiresCHAP(false);

			const int SOLAR_PLANT_BASE_PRODUCUCTION = 2000;
			auto power = new PowerStructure(structure);
			power->initialize(0, SOLAR_PLANT_BASE_PRODUCUCTION);
			NAS2D::Utility<StructureManager>::get().attachComponent(structure, power);
			break;
		}

		case StructureTypeID::SID_STORAGE_TANKS:
			structure = new StorageTanks();
			break;

		case StructureTypeID::SID_SURFACE_FACTORY:
			structure = new SurfaceFactory();
			break;

		case StructureTypeID::SID_UNDERGROUND_FACTORY:
			structure = new UndergroundFactory();
			break;

		case StructureTypeID::SID_UNIVERSITY:
			structure = new University();
			break;

		case StructureTypeID::SID_WAREHOUSE:
			structure = new Warehouse();
			break;


		case StructureTypeID::SID_TUBE:
			break;

		case StructureTypeID::SID_NONE:
			break;

		case StructureTypeID::SID_COUNT:
			break;
	}

	if (!structure)
	{
		throw std::runtime_error("StructureCatalogue::get(): Unsupported structure type: " + std::to_string(type));
	}

	structure->setPopulationRequirements(StructureCatalogue::populationRequirements(type));

	return structure;
}


/**
 * Gets the population required to operate a given Structure.
 * 
 * \param	type	A valid StructureTypeID value.
 */
const PopulationRequirements& StructureCatalogue::populationRequirements(StructureTypeID type)
{
	return mPopulationRequirementsTable[type];
}


/**
 * Gets the cost in resources required to build a given Structure.
 * 
 * \param	type	A valid StructureTypeID value.
 */
const StorableResources& StructureCatalogue::costToBuild(StructureTypeID type)
{
	return mStructureCostTable[type];
}


/**
 * Gets the recycling value of a specified structure type.
 * 
 * \param	type	A valid StructureTypeID value.
 */
const StorableResources StructureCatalogue::recyclingValue(StructureTypeID type)
{
	return mStructureRecycleValueTable[type];
}


/**
 * Initializes StructureCatalogue and builds the requirements tables.
 */
void StructureCatalogue::init()
{
	buildCostTable();
	buildRecycleValueTable();
	buildPopulationRequirementsTable();
}


/**
 * Indicates that the source ResourcePool has enough resources to accommodate
 * the resource requirements of the specificed structure.
 */
bool StructureCatalogue::canBuild(const StorableResources& source, StructureTypeID type)
{
	return StructureCatalogue::costToBuild(type) <= source;
}


/**
 * Fills out the build costs for all structures.
 */
void StructureCatalogue::buildCostTable()
{
	// RESOURCES: CommonMetals | CommonMinerals | RareMetals | RareMinerals
	mStructureCostTable[StructureTypeID::SID_AGRIDOME] = { 20, 10, 5, 0 };
	mStructureCostTable[StructureTypeID::SID_CHAP] = { 50, 10, 20, 5 };
	mStructureCostTable[StructureTypeID::SID_COMMAND_CENTER] = { 100, 75, 65, 35 };
	mStructureCostTable[StructureTypeID::SID_COMMERCIAL] = { 25, 5, 2, 0 };
	mStructureCostTable[StructureTypeID::SID_COMM_TOWER] = { 30, 10, 5, 5 };
	mStructureCostTable[StructureTypeID::SID_FUSION_REACTOR] = { 75, 25, 50, 30 };
	mStructureCostTable[StructureTypeID::SID_HOT_LABORATORY] = { 45, 10, 15, 5 };
	mStructureCostTable[StructureTypeID::SID_LABORATORY] = { 20, 10, 10, 5 };
	mStructureCostTable[StructureTypeID::SID_MEDICAL_CENTER] = { 25, 5, 2, 0 };
	mStructureCostTable[StructureTypeID::SID_NURSERY] = { 20, 10, 5, 0 };
	mStructureCostTable[StructureTypeID::SID_PARK] = { 25, 5, 2, 0 };
	mStructureCostTable[StructureTypeID::SID_SURFACE_POLICE] = { 25, 5, 2, 0 };
	mStructureCostTable[StructureTypeID::SID_UNDERGROUND_POLICE] = { 25, 5, 2, 0 };
	mStructureCostTable[StructureTypeID::SID_RECREATION_CENTER] = { 25, 5, 2, 0 };
	mStructureCostTable[StructureTypeID::SID_RECYCLING] = { 20, 10, 8, 3 };
	mStructureCostTable[StructureTypeID::SID_RED_LIGHT_DISTRICT] = { 20, 5, 2, 0 };
	mStructureCostTable[StructureTypeID::SID_RESIDENCE] = { 25, 5, 2, 0 };
	mStructureCostTable[StructureTypeID::SID_ROAD] = { 10, 15, 0, 0 };
	mStructureCostTable[StructureTypeID::SID_ROBOT_COMMAND] = { 75, 50, 45, 25 };
	mStructureCostTable[StructureTypeID::SID_SMELTER] = { 30, 20, 10, 5 };
	mStructureCostTable[StructureTypeID::SID_SOLAR_PANEL1] = { 10, 20, 5, 5 };
	mStructureCostTable[StructureTypeID::SID_SOLAR_PLANT] = { 50, 25, 50, 20 };
	mStructureCostTable[StructureTypeID::SID_STORAGE_TANKS] = { 15, 5, 6, 1 };
	mStructureCostTable[StructureTypeID::SID_SURFACE_FACTORY] = { 20, 10, 10, 5 };
	mStructureCostTable[StructureTypeID::SID_UNDERGROUND_FACTORY] = { 20, 10, 10, 5 };
	mStructureCostTable[StructureTypeID::SID_UNIVERSITY] = { 20, 10, 10, 5 };
	mStructureCostTable[StructureTypeID::SID_WAREHOUSE] = { 15, 5, 6, 1 };
}


/**
 * Fills out the recycle value for all structures.
 */
void StructureCatalogue::buildRecycleValueTable()
{
	for (std::size_t i = 0; i < StructureTypeID::SID_COUNT; ++i)
	{
		mStructureRecycleValueTable[static_cast<StructureTypeID>(i)] = recycleValue(static_cast<StructureTypeID>(i), DEFAULT_RECYCLE_VALUE);
	}

	// Set recycling values for landers and automatically built structures.
	// RESOURCES: COMM_MET_ORE, COMM_MIN_ORE, RARE_MET_ORE, RARE_MIN_ORE, COMM_MET, COMM_MIN, RARE_MET, RARE_MIN
	mStructureRecycleValueTable[StructureTypeID::SID_MINE_FACILITY] = { 15, 10, 5, 5 };
	mStructureRecycleValueTable[StructureTypeID::SID_CARGO_LANDER] = { 15, 10, 5, 5 };
	mStructureRecycleValueTable[StructureTypeID::SID_COLONIST_LANDER] = { 15, 10, 5, 5 };
	mStructureRecycleValueTable[StructureTypeID::SID_SEED_LANDER] = { 10, 5, 5, 5 };
	mStructureRecycleValueTable[StructureTypeID::SID_SEED_FACTORY] = { 15, 10, 5, 5 };
	mStructureRecycleValueTable[StructureTypeID::SID_SEED_POWER] = { 15, 10, 5, 5 };
	mStructureRecycleValueTable[StructureTypeID::SID_SEED_SMELTER] = { 15, 10, 5, 5 };

}


/**
 * Fills out the population requirements for all structures.
 */
void StructureCatalogue::buildPopulationRequirementsTable()
{
	// WORKERS, SCIENTISTS
	mPopulationRequirementsTable[StructureTypeID::SID_AGRIDOME] = { 1, 0 };
	mPopulationRequirementsTable[StructureTypeID::SID_CHAP] = { 2, 0 };
	mPopulationRequirementsTable[StructureTypeID::SID_COMMERCIAL] = { 1, 0 };
	mPopulationRequirementsTable[StructureTypeID::SID_FUSION_REACTOR] = { 1, 2 };
	mPopulationRequirementsTable[StructureTypeID::SID_HOT_LABORATORY] = { 1, 5 };
	mPopulationRequirementsTable[StructureTypeID::SID_LABORATORY] = { 1, 5 };
	mPopulationRequirementsTable[StructureTypeID::SID_MEDICAL_CENTER] = { 1, 2 };
	mPopulationRequirementsTable[StructureTypeID::SID_NURSERY] = { 1, 1 };
	mPopulationRequirementsTable[StructureTypeID::SID_PARK] = { 1, 0 };
	mPopulationRequirementsTable[StructureTypeID::SID_SURFACE_POLICE] = { 5, 0 };
	mPopulationRequirementsTable[StructureTypeID::SID_UNDERGROUND_POLICE] = { 5, 0 };
	mPopulationRequirementsTable[StructureTypeID::SID_RECREATION_CENTER] = { 2, 0 };
	mPopulationRequirementsTable[StructureTypeID::SID_RECYCLING] = { 1, 1 };
	mPopulationRequirementsTable[StructureTypeID::SID_RED_LIGHT_DISTRICT] = { 2, 0 };
	mPopulationRequirementsTable[StructureTypeID::SID_ROBOT_COMMAND] = { 4, 0 };
	mPopulationRequirementsTable[StructureTypeID::SID_SEED_FACTORY] = { 2, 0 };
	mPopulationRequirementsTable[StructureTypeID::SID_SEED_SMELTER] = { 2, 0 };
	mPopulationRequirementsTable[StructureTypeID::SID_SMELTER] = { 4, 0 };
	mPopulationRequirementsTable[StructureTypeID::SID_SOLAR_PANEL1] = { 1, 0 };
	mPopulationRequirementsTable[StructureTypeID::SID_SURFACE_FACTORY] = { 4, 0 };
	mPopulationRequirementsTable[StructureTypeID::SID_UNDERGROUND_FACTORY] = { 2, 0 };
	mPopulationRequirementsTable[StructureTypeID::SID_UNIVERSITY] = { 1, 3 };
	mPopulationRequirementsTable[StructureTypeID::SID_WAREHOUSE] = { 1, 0 };
}


/**
 * Calculates the base recycling value of a given structure.
 * 
 * \param	type	A valid StructureTypeID value.
 */
StorableResources StructureCatalogue::recycleValue(StructureTypeID type, float percent)
{
	auto recyclingValue = mStructureCostTable[type];
	for (size_t i = 0; i < recyclingValue.resources.size(); ++i)
	{
		// Truncation of value from float to int cast is intended and desired behavior
		recyclingValue.resources[i] = static_cast<int>(recyclingValue.resources[i] * percent);
	}
	return recyclingValue;
}
