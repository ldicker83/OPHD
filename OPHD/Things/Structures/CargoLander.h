#pragma once

#include "Structure.h"

#include "../../Constants.h"
#include "../../Map/Tile.h"

class CargoLander : public Structure
{
public:

	using Signal = NAS2D::Signal<>;

	CargoLander(Tile* t) : Structure(constants::CARGO_LANDER,
		"structures/lander_0.sprite",
		StructureClass::Lander,
		StructureID::SID_CARGO_LANDER),
		mTile(t)
	{
		maxAge(50);
		turnsToBuild(1);
		repairable(false);
		requiresCHAP(false);
		selfSustained(true);

		enable();
	}

	Signal::Source& deploySignal() { return mDeploy; }

protected:
	void think() override
	{
		if (age() == turnsToBuild())
		{
			mDeploy();
			mTile->index(TerrainType::Dozed);
		}
	}

private:
	CargoLander() = delete;
	CargoLander(const CargoLander&) = delete;
	CargoLander& operator=(const CargoLander&) = delete;

private:
	Signal mDeploy;
	Tile* mTile = nullptr;
};
