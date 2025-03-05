#pragma once

#include "FileIo.h"
#include "GameOptionsDialog.h"

#include "../Constants/Strings.h"
#include "../Constants/UiConstants.h"

#include <libControls/UIContainer.h>
#include <libControls/Button.h>

class GameState;
class GameViewContainer : public UIContainer
{
public:
	GameViewContainer(GameState& gameState);
	void update() override;
	FileIo& fileIoDialog() { return mFileIoDialog; }
	GameOptionsDialog& gameOptionsDialog() { return mGameOptionsDialog; }

private:
	void onGameOptionsButton() { mGameOptionsDialog.show(); }
	void showSaveDialog() { mGameOptionsDialog.hide(); mFileIoDialog.showSave(constants::SaveGamePath); }
	void showLoadDialog() { mGameOptionsDialog.hide(); mFileIoDialog.showOpen(constants::SaveGamePath); }
	FileIo mFileIoDialog;
	GameOptionsDialog mGameOptionsDialog;
	Button mGameOptionsButton;
};
