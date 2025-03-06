#pragma once

#include <string>

struct ColonyShipData
{
	int colonistLanders = 0;
	int cargoLanders = 0;
	int turnsOfManeuveringFuel = 0;
	bool crashed = false;
};

ColonyShipData colonyShipDataFromFile (const std::string&);

enum class Difficulty;
class GameState;

class ColonyShip
{
public:
	ColonyShip();
	ColonyShip(const ColonyShipData&);

	int colonistLanders() const { return mColonistLanders; }
	int cargoLanders() const { return mCargoLanders; }
	int turnsOfManeuveringFuel() const { return mTurnsOfManeuveringFuel; }
	void onDeployColonistLander() { --mColonistLanders; }
	void onDeployCargoLander() { --mCargoLanders; }
	bool crashed() const { return mCrashed; }

private:
	int mColonistLanders = 2;
	int mCargoLanders = 2;
	int mTurnsOfManeuveringFuel = 25;
	bool mCrashed = false;
};
