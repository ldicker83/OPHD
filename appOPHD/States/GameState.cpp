#include "GameState.h"

#include "MainMenuState.h"
#include "MapViewState.h"
#include "MainReportsUiState.h"
#include "Wrapper.h"
#include "../StructureManager.h"
#include "../UI/MessageBox.h"
#include "../Constants/Strings.h"

#include <NAS2D/Filesystem.h>
#include <NAS2D/Utility.h>
#include <NAS2D/EventHandler.h>
#include <NAS2D/Mixer/Mixer.h>
#include <NAS2D/Renderer/Renderer.h>


NAS2D::Point<int> MOUSE_COORDS; /**< Mouse Coordinates. Used by other states/wrapers. */


GameState::GameState(const std::string& savedGameFilename) :
	mMainReportsState{std::make_unique<MainReportsUiState>()},
	mMapViewState{std::make_unique<MapViewState>(*this, savedGameFilename)},
	mGameViewContainer{*this}
{
	initializeGameState();
}


GameState::GameState(const Planet::Attributes& planetAttributes, Difficulty selectedDifficulty) :
	mMainReportsState{std::make_unique<MainReportsUiState>()},
	mMapViewState{std::make_unique<MapViewState>(*this, planetAttributes, selectedDifficulty)},
	mGameViewContainer{*this}
{
	initializeGameState();
}


GameState::~GameState()
{
	NAS2D::Utility<StructureManager>::get().dropAllStructures();

	auto& eventHandler = NAS2D::Utility<NAS2D::EventHandler>::get();
	eventHandler.mouseMotion().disconnect({this, &GameState::onMouseMove});

	NAS2D::Utility<NAS2D::Mixer>::get().musicCompleteSignalSource().disconnect({this, &GameState::onMusicComplete});
	NAS2D::Utility<NAS2D::Mixer>::get().stopAllAudio();
}


GameState::ReturnToMainMenuDelegate GameState::onReturnToMainMenuDelegate()
{
	return {mMapViewState.get(), &MapViewState::onGameOver};
}


void GameState::initializeGameState()
{
	mMainReportsState->initialize();
	mMainReportsState->hideReports().connect({this, &GameState::onHideReports});

	for (auto takeMeThere : mMainReportsState->takeMeThere())
	{
		takeMeThere->connect({this, &GameState::onTakeMeThere});
	}

	mMapViewState->initialize();
	initializeMapViewState();

	auto& eventHandler = NAS2D::Utility<NAS2D::EventHandler>::get();
	eventHandler.mouseMotion().connect({this, &GameState::onMouseMove});

	NAS2D::Utility<NAS2D::Mixer>::get().musicCompleteSignalSource().connect({this, &GameState::onMusicComplete});
}


void GameState::initializeMapViewState()
{
	mActiveState = mMapViewState.get();
	mMapViewState->activate();

	mMapViewState->quit().connect({this, &GameState::onQuit});
	mMapViewState->showReportsUi().connect({this, &GameState::onShowReports});
	mMapViewState->mapChanged().connect({this, &GameState::onMapChange});
}


void GameState::initialize()
{
	mFade.fadeIn(constants::FadeSpeed);
}


void GameState::onMouseMove(NAS2D::Point<int> position, NAS2D::Vector<int> /*relative*/)
{
	MOUSE_COORDS = position;
}


/**
 * Music Complete event handler.
 *
 * Called by NAS2D::Mixer upon completion of a music track. This function
 * changes the background music track to a different track in the lineup.
 */
void GameState::onMusicComplete()
{
	/// \todo	Make me work... once there's some music to listen to. 0.0
}


/**
 * Event handler that responds to a quit event raised by the MapViewState.
 *
 * This event is raised on game overs and when the user chooses the "Return
 * to Main Menu" from the system options window.
 */
void GameState::onQuit()
{
	mMapViewState->deactivate();
	mMainReportsState->deactivate();
}


/**
 * Event handler that responds to a show reports event raised by the MapViewState.
 *
 * This event is raised whenever a user double-clicks on a factory in the MapViewState.
 */
void GameState::onShowReports()
{
	if (mMainReportsState)
	{
		mActiveState->deactivate();
		mActiveState = mMainReportsState.get();
		mActiveState->activate();
	}
}


/**
 * Event handler that responds to a hide report event raised by the MainReportsUiState.
 *
 * This event is raised by the MainReportsUiState whenever the user clicks the Exit
 * UI panel or if the Escape key is pressed.
 */
void GameState::onHideReports()
{
	if (mMapViewState)
	{
		mActiveState->deactivate();
		mActiveState = mMapViewState.get();
		mActiveState->activate();
	}
}


void GameState::onMapChange()
{
	mMainReportsState->clearLists();
}


void GameState::onLoadGame(const std::string& saveGameName)
{
	auto& filesystem = NAS2D::Utility<NAS2D::Filesystem>::get();
	auto saveGamePath = constants::SaveGamePath + saveGameName + ".xml";
	try
	{
		if (!filesystem.exists(saveGamePath))
		{
			throw std::runtime_error("Save game file does not exist: " + saveGamePath);
		}
		auto newMapViewState = std::make_unique<MapViewState>(*this, saveGamePath);
		newMapViewState->initialize();
		mNewMapViewState = std::move(newMapViewState);
	}
	catch (const std::exception& e)
	{
		doNonFatalErrorMessage("Load Failed", e.what());
		return;
	}
}


void GameState::onSaveGame(const std::string& saveGameName)
{
	mMapViewState->save(constants::SaveGamePath + saveGameName + ".xml");
}


/**
 * Event handler that responds to a 'take me there' event raised by the MainReportsUiState.
 *
 * This event is raised by the MainReportsUiState whenever a "Take Me There" button in any
 * of the report UI panels is clicked.
 */
void GameState::onTakeMeThere(const Structure* structure)
{
	onHideReports();
	mMapViewState->focusOnStructure(structure);
}


NAS2D::State* GameState::update()
{
	if (mActiveState)
	{
		mActiveState->update();
	}

	mFade.update();
	mFade.draw(NAS2D::Utility<NAS2D::Renderer>::get());

	if (mMapViewState && mMapViewState->hasGameEnded())
	{
		mReturnState = new MainMenuState();
	}

	if (mNewMapViewState)
	{
		mMapViewState->quit().disconnect({this, &GameState::onQuit});
		mMapViewState = std::move(mNewMapViewState);
		initializeMapViewState();
	}

	return mReturnState;
}
