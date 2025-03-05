#include "GameViewContainer.h"

#include "../States/GameState.h"

GameViewContainer::GameViewContainer(GameState& gameState) : 
	UIContainer{{&mFileIoDialog}},
	mFileIoDialog{gameState.fileLoadDelegate(), gameState.fileSaveDelegate()}
{
	mGameOptionsDialog.hide();

	mGameOptionsDialog.saveGameDialogButtonSignal().connect({this, &GameViewContainer::showSaveDialog});
	mGameOptionsDialog.loadGameDialogButtonSignal().connect({this, &GameViewContainer::showLoadDialog});

	mFileIoDialog.hide();
	mFileIoDialog.anchored(true);

	const auto rendererCenter = NAS2D::Utility<NAS2D::Renderer>::get().center().to<int>();
	const auto centerPosition = [&rendererCenter](const Control& control) { return (rendererCenter - control.size() / 2); };
	mFileIoDialog.position(NAS2D::Point{centerPosition(mFileIoDialog).x, centerPosition(mFileIoDialog).y});
}


void GameViewContainer::update()
{
	UIContainer::update();
}
