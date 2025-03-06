#include "GameViewContainer.h"

#include "../States/GameState.h"

GameViewContainer::GameViewContainer(GameState& gameState) :
	UIContainer{{&mFileIoDialog, &mGameOptionsDialog, &mGameOptionsButton}},
	mFileIoDialog{gameState.fileLoadDelegate(), gameState.fileSaveDelegate()},
	mGameOptionsButton{"Menu", {this, &GameViewContainer::onGameOptionsButton}}
{
	mGameOptionsDialog.hide();

	mGameOptionsDialog.saveGameDialogButtonSignal().connect({this, &GameViewContainer::showSaveDialog});
	mGameOptionsDialog.loadGameDialogButtonSignal().connect({this, &GameViewContainer::showLoadDialog});
	mGameOptionsDialog.returnToMainMenuButtonSignal().connect(gameState.onReturnToMainMenuDelegate());

	mFileIoDialog.hide();
	mFileIoDialog.anchored(true);

	const auto rendererCenter = NAS2D::Utility<NAS2D::Renderer>::get().center().to<int>();
	const auto centerPosition = [&rendererCenter](const Control& control) { return (rendererCenter - control.size() / 2); };
	mFileIoDialog.position(NAS2D::Point{centerPosition(mFileIoDialog).x, centerPosition(mFileIoDialog).y});
	mGameOptionsDialog.position(NAS2D::Point{centerPosition(mGameOptionsDialog).x, centerPosition(mGameOptionsDialog).y});

	constexpr auto hudHeight = constants::ResourceIconSize + constants::MarginTight * 2;
	const auto rendererSize = NAS2D::Utility<NAS2D::Renderer>::get().size();
	const auto gameOptionsButtonSize = getDefaultFont().size(mGameOptionsButton.text());

	mGameOptionsButton.size({gameOptionsButtonSize.x + constants::Margin, hudHeight});
	mGameOptionsButton.position({rendererSize.x - mGameOptionsButton.size().x, constants::MarginTight / 2});
}


void GameViewContainer::update()
{
	UIContainer::update();
}
