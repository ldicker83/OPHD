#pragma once

#include <NAS2D/State.h>

class MapViewState;
class Structure;

class GameState : public NAS2D::State
{
public:
	GameState();
	virtual ~GameState();

	void mapviewstate(MapViewState*);

	virtual void initialize() final;
	virtual State* update() final;

private:
	void onMouseMove(int x, int y, int relX, int relY);

	void fadeComplete();
	void musicComplete();

	void quitEvent();
	void showReportsUi();
	void hideReportsUi();
	void mapChanged();

	void takeMeThere(Structure*);

private:
	NAS2D::State*	mReturnState = this;
};
