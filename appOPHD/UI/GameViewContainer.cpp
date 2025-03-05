#include "GameViewContainer.h"

#include "../States/GameState.h"

GameViewContainer::GameViewContainer(GameState& gameState) : 
	UIContainer{{&mFileIoDialog}},
	mFileIoDialog{gameState.fileLoadDelegate(), gameState.fileSaveDelegate()}
{}


void GameViewContainer::update()
{
	UIContainer::update();
}
