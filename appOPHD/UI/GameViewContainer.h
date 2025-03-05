#pragma once

#include "FileIo.h"
#include "GameOptionsDialog.h"

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
	FileIo mFileIoDialog;
	GameOptionsDialog mGameOptionsDialog;
};
