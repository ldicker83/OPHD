#include "GameOptionsDialog.h"

#include "../Constants/Strings.h"
#include "../ShellOpenPath.h"
#include <array>


GameOptionsDialog::GameOptionsDialog() :
	Window{constants::WindowSystemTitle},
	btnSave{"Save current game", {this, &GameOptionsDialog::onSave}},
	btnLoad{"Load a saved game", {this, &GameOptionsDialog::onLoad}},
	btnHelp{"Help", {this, &GameOptionsDialog::onHelp}},
	btnReturn{"Return to current game", {this, &GameOptionsDialog::onReturn}},
	btnClose{"Return to Main Menu", {this, &GameOptionsDialog::onClose}}
{
	position({0, 0});

	const auto buttons = std::array{&btnSave, &btnLoad, &btnHelp, &btnReturn, &btnClose};
	auto position = mRect.position + NAS2D::Vector{buttonHorizontalMargin, buttonHeight};
	for (auto button : buttons)
	{
		button->size({buttonWidth, buttonHeight});
		add(*button, {position.x, position.y});
		position.y += buttonHeight + buttonVerticalMargin;
	}

	size({buttonWidth + 2 * buttonHorizontalMargin, position.y});

	anchored(true);
}


void GameOptionsDialog::onEnableChange()
{
	btnSave.enabled(enabled());
	btnLoad.enabled(enabled());
	btnHelp.enabled(enabled());
	btnReturn.enabled(enabled());
	btnClose.enabled(enabled());
}


void GameOptionsDialog::update()
{
	if (!visible()) { return; }

	Window::update();
}

void GameOptionsDialog::onSave()
{
	mSignalSaveDialog();
}

void GameOptionsDialog::onLoad()
{
	mSignalLoadDialog();
}

void GameOptionsDialog::onHelp()
{
	shellOpenPath("https://wiki.outpost2.net/doku.php?id=outposthd:how_to_play");
}

void GameOptionsDialog::onReturn()
{
	hide();
}

void GameOptionsDialog::onClose()
{
	mSignalClose();
}
