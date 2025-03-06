#include "ColonyShip.h"

#include "appOPHD/Constants/Strings.h"
#include "appOPHD/OpenSaveGame.h"

#include <NAS2D/Utility.h>
#include <NAS2D/Filesystem.h>
#include <NAS2D/ParserHelper.h>

namespace
{
	void setLanders(ColonyShipData& colonyShipData, NAS2D::Xml::XmlElement* element)
	{
		if (element)
		{
			const auto dictionary = NAS2D::attributesToDictionary(*element);
			colonyShipData.colonistLanders = dictionary.get<int>("colonist_landers");
			colonyShipData.cargoLanders = dictionary.get<int>("cargo_landers");
		}
	}

	void setManeuveringFuel(ColonyShipData& colonyShipData, NAS2D::Xml::XmlElement* element)
	{
		if (element)
		{
			auto turnCount = NAS2D::attributesToDictionary(*element).get<int>("count");

			colonyShipData.turnsOfManeuveringFuel = turnCount > constants::ColonyShipOrbitTime ? 0 : constants::ColonyShipOrbitTime - turnCount + 1;
		}
	}
}


ColonyShipData colonyShipDataFromFile(const std::string& filePath)
{
	if (!NAS2D::Utility<NAS2D::Filesystem>::get().exists(filePath))
	{
		throw std::runtime_error("File '" + filePath + "' was not found.");
	}
	auto xmlDocument = openSavegame(filePath);
	auto* root = xmlDocument.firstChildElement(constants::SaveGameRootNode);

	ColonyShipData colonyShipData;
	setLanders(colonyShipData, root->firstChildElement("population"));
	setManeuveringFuel(colonyShipData, root->firstChildElement("turns"));

	colonyShipData.crashed = colonyShipData.turnsOfManeuveringFuel == 0;

	return colonyShipData;
}


ColonyShip::ColonyShip()
{}


ColonyShip::ColonyShip(const ColonyShipData& colonyShipData) :
	mColonistLanders{colonyShipData.colonistLanders},
	mCargoLanders{colonyShipData.cargoLanders},
	mTurnsOfManeuveringFuel{colonyShipData.turnsOfManeuveringFuel},
	mCrashed{colonyShipData.crashed}
{}
