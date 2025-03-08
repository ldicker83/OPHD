#pragma once

#include "Planet.h"
#include "MapViewState.h"
#include "MainReportsUiState.h"
#include "ColonyShip.h"

#include "../UI/FileIo.h"

#include <NAS2D/State.h>
#include <NAS2D/Math/Point.h>
#include <NAS2D/Math/Vector.h>
#include <NAS2D/Renderer/Fade.h>

#include <string>
#include <memory>


enum class Difficulty;

class Structure;
class Wrapper;


class GameState : public NAS2D::State
{
public:
	GameState(const std::string& savedGameFilename);
	GameState(const Planet::Attributes& planetAttributes, Difficulty selectedDifficulty);
	~GameState() override;

	State* update() override;

	MainReportsUiState& mainReportsState() { return mMainReportsState; }
	FileIo& fileIoDialog() { return mFileIoDialog; }
	ColonyShip& colonyShip() { return mColonyShip; }

protected:
	void initializeGameState();
	void initializeMapViewState();
	void initialize() override;

	void onMouseMove(NAS2D::Point<int> position, NAS2D::Vector<int> relative);

	void onFadeComplete();
	void onMusicComplete();

	void onQuit();
	void onShowReports();
	void onHideReports();
	void onMapChange();
	void onLoadGame(const std::string&);
	void onSaveGame(const std::string&);

	void onTakeMeThere(const Structure*);

private:
	MainReportsUiState mMainReportsState;
	MapViewState mMapViewState;
	ColonyShip mColonyShip;
	Wrapper* mActiveState = nullptr;
	NAS2D::State* mReturnState = this;
	NAS2D::Fade mFade;
	FileIo mFileIoDialog;
};
