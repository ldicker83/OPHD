#pragma once

#include "../UI/MajorEventAnnouncement.h"

#include <libOPHD/Population/Morale.h>

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
	using MoraleChangeEntries = std::vector<MoraleChangeEntry>;
	struct ColonyShipCrashEffects
	{
		MoraleChangeEntries moraleChangeEntries = MoraleChangeEntries{};
		MajorEventAnnouncement::AnnouncementType announcementType = MajorEventAnnouncement::AnnouncementType::ColonyShipCrash;
	};

	ColonyShip();
	ColonyShip(const ColonyShipData&);

	int colonistLanders() const { return mColonistLanders; }
	int cargoLanders() const { return mCargoLanders; }
	int turnsOfManeuveringFuel() const { return mTurnsOfManeuveringFuel; }
	void onDeployColonistLander() { --mColonistLanders; }
	void onDeployCargoLander() { --mCargoLanders; }
	bool crashed() const { return mCrashed; }

private:
	ColonyShipCrashEffects colonyShipCrashEffects(Difficulty);
	MajorEventAnnouncement::AnnouncementType colonyShipCrashAnnouncement(const int, const int);

	struct LanderCrashData
	{
		std::string description = "";
		int numberOfLanders;
		int landerMoraleLossMultiplier;
	};
	MoraleChangeEntry colonyShipCrashMoraleChangeEntry(LanderCrashData, Difficulty);

	int mColonistLanders = 2;
	int mCargoLanders = 2;
	int mTurnsOfManeuveringFuel = 25;
	bool mCrashed = false;
};
