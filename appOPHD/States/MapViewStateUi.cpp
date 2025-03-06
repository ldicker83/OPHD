// ==================================================================================
// = This file implements the UI and UI Event Handlers used by MapViewState. I separated
// = it into its own file because the MapViewState.cpp file was starting to get a little
// = out of control.
// ==================================================================================

#include "MapViewState.h"
#include "ColonyShip.h"
#include "MapViewStateHelper.h"

#include "MainMenuState.h"

#include "../Constants/Strings.h"
#include "../Constants/UiConstants.h"

#include "../PointerType.h"
#include "../StructureCatalogue.h"
#include "../StructureManager.h"
#include "../Map/TileMap.h"
#include "../Map/MapView.h"

#include "../UI/MessageBox.h"
#include "../UI/NavControl.h"
#include "../UI/CheatMenu.h"

#include <libOPHD/DirectionOffset.h>
#include <libOPHD/EnumDirection.h>

#include <libOPHD/Map/MapCoordinate.h>

#include <NAS2D/Utility.h>
#include <NAS2D/Renderer/Renderer.h>


extern NAS2D::Point<int> MOUSE_COORDS;


namespace
{
	void fillList(IconGrid& grid, const std::vector<IconGrid::Item>& itemList)
	{
		for (const auto& item : itemList)
		{
			grid.addItem(item);
		}
	}
}


/**
 * Sets up the user interface elements
 *
 * \note	The explicit casts to int to truncate floating point values to force
 *			window positions to a whole number.
 */
void MapViewState::initUi()
{
	auto& renderer = NAS2D::Utility<NAS2D::Renderer>::get();

	mCheatMenu.cheatCodeEntered().connect({this, &MapViewState::onCheatCodeEntry});
	mCheatMenu.hide();

	mDiggerDirection.directionSelected().connect({this, &MapViewState::onDiggerSelectionDialog});
	mDiggerDirection.hide();

	mTileInspector.position(renderer.center() - NAS2D::Vector{mTileInspector.size().x / 2.0f, 175.0f});
	mTileInspector.hide();

	mStructureInspector.position(renderer.center() - NAS2D::Vector{mStructureInspector.size().x / 2.0f, 175.0f});
	mStructureInspector.hide();

	mRobotInspector.position(renderer.center() - NAS2D::Vector{mRobotInspector.size().x / 2.0f, 175.0f});
	mRobotInspector.hide();

	mFactoryProduction.position(NAS2D::Point{renderer.center().x - mFactoryProduction.size().x / 2.0f, 175.0f});
	mFactoryProduction.hide();

	mFileIoDialog.anchored(true);
	mFileIoDialog.hide();

	const auto populationPanelX = std::min(675, renderer.size().x - mPopulationPanel.size().x);
	mPopulationPanel.position({populationPanelX, constants::ResourceIconSize + 4 + constants::MarginTight});

	mResourceBreakdownPanel.position({0, 22});
	mResourceBreakdownPanel.playerResources(&mResourcesCount);

	mGameOverDialog.returnToMainMenu().connect({this, &MapViewState::onGameOver});
	mGameOverDialog.hide();

	mGameOptionsDialog.saveGameDialogButtonSignal().connect({this, &MapViewState::onOpenSaveGameDialog});
	mGameOptionsDialog.loadGameDialogButtonSignal().connect({this, &MapViewState::onOpenLoadGameDialog});
	mGameOptionsDialog.returnToGameButtonSignal().connect({this, &MapViewState::onReturnToGame});
	mGameOptionsDialog.returnToMainMenuButtonSignal().connect({this, &MapViewState::onGameOver});
	mGameOptionsDialog.hide();

	mAnnouncement.hide();
	mMineOperationsWindow.hide();
	mWarehouseInspector.hide();

	mWindowStack.addWindow(&mTileInspector);
	mWindowStack.addWindow(&mStructureInspector);
	mWindowStack.addWindow(&mFactoryProduction);
	mWindowStack.addWindow(&mDiggerDirection);
	mWindowStack.addWindow(&mAnnouncement);
	mWindowStack.addWindow(&mWarehouseInspector);
	mWindowStack.addWindow(&mMineOperationsWindow);
	mWindowStack.addWindow(&mRobotInspector);
	mWindowStack.addWindow(&mNotificationWindow);
	mWindowStack.addWindow(&mCheatMenu);

	mNotificationArea.notificationClicked().connect({this, &MapViewState::onNotificationClicked});

	mNotificationWindow.takeMeThere().connect({this, &MapViewState::onTakeMeThere});
	mNotificationWindow.hide();

	const auto size = renderer.size().to<int>();
	mBottomUiRect = {{0, size.y - constants::BottomUiHeight}, {size.x, constants::BottomUiHeight}};

	// BUTTONS
	mBtnTurns.image("ui/icons/turns.png");
	mBtnTurns.position(NAS2D::Point{mMiniMapRect.position.x - constants::MainButtonSize - constants::MarginTight, size.y - constants::Margin - constants::MainButtonSize});
	mBtnTurns.size(constants::MainButtonSize);
	mBtnTurns.click().connect({this, &MapViewState::onTurns});
	mBtnTurns.enabled(false);

	mBtnToggleHeightmap.image("ui/icons/height.png");
	mBtnToggleHeightmap.size(constants::MainButtonSize);
	mBtnToggleHeightmap.type(Button::Type::Toggle);
	mBtnToggleHeightmap.click().connect({this, &MapViewState::onToggleHeightmap});

	mBtnToggleConnectedness.image("ui/icons/connection.png");
	mBtnToggleConnectedness.size(constants::MainButtonSize);
	mBtnToggleConnectedness.type(Button::Type::Toggle);
	mBtnToggleConnectedness.click().connect({this, &MapViewState::onToggleConnectedness});

	mBtnToggleCommRangeOverlay.image("ui/icons/comm_overlay.png");
	mBtnToggleCommRangeOverlay.size(constants::MainButtonSize);
	mBtnToggleCommRangeOverlay.type(Button::Type::Toggle);
	mBtnToggleCommRangeOverlay.click().connect({this, &MapViewState::onToggleCommRangeOverlay});

	mBtnToggleRouteOverlay.image("ui/icons/route.png");
	mBtnToggleRouteOverlay.size(constants::MainButtonSize);
	mBtnToggleRouteOverlay.type(Button::Type::Toggle);
	mBtnToggleRouteOverlay.click().connect({this, &MapViewState::onToggleRouteOverlay});

	mBtnTogglePoliceOverlay.image("ui/icons/police.png");
	mBtnTogglePoliceOverlay.size(constants::MainButtonSize);
	mBtnTogglePoliceOverlay.type(Button::Type::Toggle);
	mBtnTogglePoliceOverlay.click().connect({this, &MapViewState::onTogglePoliceOverlay});

	// Menus
	mRobots.position({mBtnTurns.position().x - constants::MarginTight - 52, mBottomUiRect.position.y + constants::Margin});
	mRobots.size({52, constants::BottomUiHeight - constants::Margin * 2});
	mRobots.showTooltip(true);
	mRobots.selectionChanged().connect({this, &MapViewState::onRobotsSelectionChange});

	mConnections.position({mRobots.position().x - constants::MarginTight - 52, mBottomUiRect.position.y + constants::Margin});
	mConnections.size({52, constants::BottomUiHeight - constants::Margin * 2});
	mConnections.selectionChanged().connect({this, &MapViewState::onConnectionsSelectionChange});

	mStructures.position(NAS2D::Point{constants::Margin, mBottomUiRect.position.y + constants::Margin});
	mStructures.size({mConnections.position().x - constants::Margin - constants::MarginTight, constants::BottomUiHeight - constants::Margin * 2});
	mStructures.showTooltip(true);
	mStructures.selectionChanged().connect({this, &MapViewState::onStructuresSelectionChange});

	// Initial Structures
	mStructures.addItem({constants::SeedLander, 0, StructureID::SID_SEED_LANDER});

	// tooltip control sizes
	constexpr auto hudHeight = constants::ResourceIconSize + constants::MarginTight * 2;

	mTooltipCurrentTurns.size({45, hudHeight});
	mTooltipSystemButton.size({hudHeight, hudHeight});

	// Tool Tips
	mToolTip.add(mBtnTurns, constants::ToolTipBtnTurns);
	mToolTip.add(mBtnToggleHeightmap, constants::ToolTipBtnHeightmap);
	mToolTip.add(mBtnToggleConnectedness, constants::ToolTipBtnConnectedness);
	mToolTip.add(mBtnToggleCommRangeOverlay, constants::ToolTipBtnCommRange);
	mToolTip.add(mBtnToggleRouteOverlay, constants::ToolTipBtnRoutes);
	mToolTip.add(mBtnTogglePoliceOverlay, constants::ToolTipBtnPolice);
	mToolTip.add(mTooltipCurrentTurns, constants::ToolTipCurrentTurns);
	mToolTip.add(mTooltipSystemButton, constants::ToolTipSystemMenu);
}


void MapViewState::setupUiPositions(NAS2D::Vector<int> size)
{
	const auto populationPanelX = std::min(675, size.x - mPopulationPanel.size().x);
	mPopulationPanel.position({populationPanelX, constants::ResourceIconSize + 4 + constants::MarginTight});

	// Bottom UI Area
	mBottomUiRect = {{0, size.y - constants::BottomUiHeight}, {size.x, constants::BottomUiHeight}};

	// Menu / System Icon
	mTooltipSystemButton.position({size.x - (constants::ResourceIconSize + constants::MarginTight * 2), 0});
	mTooltipCurrentTurns.position({size.x - 80 , 0});

	mRobotDeploymentSummary.area({{8, size.y - constants::BottomUiHeight - 8 - 100}, {200, 100}});

	// Mini Map
	mMiniMapRect = {{size.x - 300 - constants::Margin, mBottomUiRect.position.y + constants::Margin}, {300, 150}};
	mMiniMap->area(mMiniMapRect);

	const auto navControlEndPoint = NAS2D::Point{size.x, mBottomUiRect.position.y};
	const auto navControlSize = NAS2D::Vector{(32 + constants::MarginTight) * 3, 99};
	mNavControl->area(NAS2D::Rectangle{navControlEndPoint - navControlSize, navControlSize});

	// Notification Area
	auto& renderer = NAS2D::Utility<NAS2D::Renderer>::get();
	mNotificationArea.height(mNavControl->position().y - 22 - constants::Margin);
	mNotificationArea.position({renderer.size().x - mNotificationArea.size().x, 22});

	// Position UI Buttons
	mBtnTurns.position(NAS2D::Point{mMiniMapRect.position.x - constants::MainButtonSize - constants::MarginTight, size.y - constants::Margin - constants::MainButtonSize});
	mBtnToggleHeightmap.position({mBtnTurns.position().x, mMiniMapRect.position.y});
	mBtnToggleConnectedness.position({mBtnTurns.position().x, mMiniMapRect.position.y + constants::MainButtonSize});
	mBtnToggleCommRangeOverlay.position({mBtnTurns.position().x, mMiniMapRect.position.y + (constants::MainButtonSize * 2)});
	mBtnToggleRouteOverlay.position({mBtnTurns.position().x, mMiniMapRect.position.y + (constants::MainButtonSize * 3)});
	mBtnTogglePoliceOverlay.position({mBtnTurns.position().x - constants::MainButtonSize, mMiniMapRect.position.y});

	// UI Panels
	mRobots.position({mBtnTogglePoliceOverlay.position().x - constants::MarginTight - 52, mBottomUiRect.position.y + constants::Margin});
	mConnections.position({mRobots.position().x - constants::MarginTight - 52, mBottomUiRect.position.y + constants::Margin});
	mStructures.position(NAS2D::Point{constants::Margin, mBottomUiRect.position.y + constants::Margin});

	mStructures.size({mConnections.position().x - constants::Margin - constants::MarginTight, constants::BottomUiHeight - constants::Margin * 2});

	// Allow for centering with rounding to integer values
	const auto rendererCenter = NAS2D::Utility<NAS2D::Renderer>::get().center().to<int>();
	const auto centerPosition = [&rendererCenter](const Control& control) { return (rendererCenter - control.size() / 2); };

	// Anchored window positions
	mFileIoDialog.position(NAS2D::Point{centerPosition(mFileIoDialog).x, 50});
	mCheatMenu.position(NAS2D::Point{centerPosition(mCheatMenu).x, centerPosition(mCheatMenu).y});
	mGameOverDialog.position(centerPosition(mGameOverDialog) - NAS2D::Vector{0, 100});
	mAnnouncement.position(centerPosition(mAnnouncement) - NAS2D::Vector{0, 100});
	mGameOptionsDialog.position(centerPosition(mGameOptionsDialog) - NAS2D::Vector{0, 100});

	mDiggerDirection.position(NAS2D::Point{centerPosition(mDiggerDirection).x, size.y / 2 - 125});

	mWarehouseInspector.position(centerPosition(mWarehouseInspector) - NAS2D::Vector{0, 100});
	mMineOperationsWindow.position(centerPosition(mMineOperationsWindow) - NAS2D::Vector{0, 100});

	mNotificationWindow.position(centerPosition(mMineOperationsWindow) - NAS2D::Vector{0, 100});

	/**
	 * \note	We are not setting the tile inspector window's position here because it's something that can be
	 *			moved around by the user. Re-centering it here could annoy the user.
	 */
}


/**
 * Hides ALL UI elements.
 */
void MapViewState::hideUi()
{
	mBtnTurns.hide();

	mBtnToggleHeightmap.hide();
	mBtnToggleConnectedness.hide();
	mBtnToggleCommRangeOverlay.hide();
	mBtnToggleRouteOverlay.hide();
	mBtnTogglePoliceOverlay.hide();

	mStructures.hide();
	mRobots.hide();
	mConnections.hide();

	mWindowStack.hide();
}


/**
 * Unhides the UI.
 */
void MapViewState::unhideUi()
{
	mBtnTurns.show();

	mBtnToggleHeightmap.show();
	mBtnToggleConnectedness.show();
	mBtnToggleCommRangeOverlay.show();
	mBtnTogglePoliceOverlay.visible(true);
	mBtnToggleRouteOverlay.show();

	mStructures.show();
	mRobots.show();
	mConnections.show();

	mGameOverDialog.enabled(true);
	mGameOptionsDialog.enabled(true);
}


/**
 * Hides all non-essential UI elements.
 */
void MapViewState::resetUi()
{
	clearMode();
	clearSelections();

	mWindowStack.hide();
}


void MapViewState::clearSelections()
{
	mStructures.clearSelection();
	mConnections.clearSelection();
	mRobots.clearSelection();
}


/**
 * Adds selection options to the Structure Menu
 */
void MapViewState::populateStructureMenu()
{
	mStructures.clear();
	mConnections.clear();

	// Above Ground structures only
	if (NAS2D::Utility<StructureManager>::get().count() == 0)
	{
		if (mMapView->isSurface())
		{
			mStructures.addItem({constants::SeedLander, 0, StructureID::SID_SEED_LANDER});
		}
	}
	else if (mMapView->isSurface())
	{
		fillList(mStructures, mStructureTracker.availableSurfaceStructures());
		fillList(mConnections, mStructureTracker.surfaceTubes());

		// Special case code, not thrilled with this
		if (mColonyShip.colonistLanders() > 0) { mStructures.addItem({constants::ColonistLander, 2, StructureID::SID_COLONIST_LANDER}); }
		if (mColonyShip.cargoLanders() > 0) { mStructures.addItem({constants::CargoLander, 1, StructureID::SID_CARGO_LANDER}); }
	}
	else
	{
		fillList(mStructures, mStructureTracker.availableUndergroundStructures());
		fillList(mConnections, mStructureTracker.undergroundTubes());
	}

	updateStructuresAvailability();

	mStructures.sort();
}


/**
 * Some UI elements should be treated as 'modal' and block input when they're
 * visible. This is a central way to check for a modal UI element without
 * having to check for multiple elements in a variety of places in the code.
 */
bool MapViewState::modalUiElementDisplayed() const
{
	return mGameOptionsDialog.visible() ||
		mFileIoDialog.visible() ||
		mGameOverDialog.visible();
}


/**
* Updates and draws the UI.
*/
void MapViewState::drawUI()
{
	auto& renderer = NAS2D::Utility<NAS2D::Renderer>::get();

	// Bottom UI
	renderer.drawBoxFilled(mBottomUiRect, NAS2D::Color{39, 39, 39});
	renderer.drawBox(mBottomUiRect, NAS2D::Color{21, 21, 21});
	renderer.drawLine(NAS2D::Point{mBottomUiRect.position.x + 1, mBottomUiRect.position.y}, NAS2D::Point{mBottomUiRect.position.x + mBottomUiRect.size.x - 2, mBottomUiRect.position.y}, NAS2D::Color{56, 56, 56});

	mMiniMap->draw();
	mNavControl->draw();
	mRobotDeploymentSummary.draw();

	if (mResourceInfoBar.isPopulationPanelVisible()) { mPopulationPanel.update(); }
	if (mResourceInfoBar.isResourcePanelVisible()) { mResourceBreakdownPanel.update(); }

	mResourceInfoBar.update();
	drawSystemButton();

	mNotificationArea.update();

	// Buttons
	mBtnTurns.update();
	mBtnToggleHeightmap.update();
	mBtnToggleConnectedness.update();
	mBtnToggleCommRangeOverlay.update();
	mBtnToggleRouteOverlay.update();
	mBtnTogglePoliceOverlay.update();

	// Menus
	mRobots.update();
	mConnections.update();
	mStructures.update();

	// Windows
	mFileIoDialog.update();
	mGameOptionsDialog.update();
	mWindowStack.update();

	if (!modalUiElementDisplayed()) { mToolTip.update(); }
}


void MapViewState::setOverlay(std::vector<Tile*>& tileList, Tile::Overlay overlay)
{
	for (auto tile : tileList)
	{
		tile->overlay(overlay);
	}
}


void MapViewState::clearOverlays()
{
	clearOverlay(mConnectednessOverlay);
	clearOverlay(mCommRangeOverlay);
	clearOverlay(mPoliceOverlays[static_cast<std::size_t>(mMapView->currentDepth())]);
	clearOverlay(mTruckRouteOverlay);
}


void MapViewState::clearOverlay(std::vector<Tile*>& tileList)
{
	setOverlay(tileList, Tile::Overlay::None);
}


void MapViewState::changePoliceOverlayDepth(int oldDepth, int newDepth)
{
	clearOverlay(mPoliceOverlays[static_cast<std::size_t>(oldDepth)]);
	setOverlay(mPoliceOverlays[static_cast<std::size_t>(newDepth)], Tile::Overlay::Police);
}


void MapViewState::onToggleHeightmap()
{
	mMiniMap->heightMapVisible(mBtnToggleHeightmap.isPressed());
}


/**
 * Handles clicks of the Connectedness Overlay button.
 */
void MapViewState::onToggleConnectedness()
{
	clearOverlays();

	if (mBtnToggleConnectedness.isPressed())
	{
		mBtnToggleCommRangeOverlay.toggle(false);
		mBtnToggleRouteOverlay.toggle(false);
		mBtnTogglePoliceOverlay.toggle(false);

		setOverlay(mConnectednessOverlay, Tile::Overlay::Connectedness);
	}
}


void MapViewState::onToggleCommRangeOverlay()
{
	clearOverlays();

	if (mBtnToggleCommRangeOverlay.isPressed())
	{
		mBtnToggleConnectedness.toggle(false);
		mBtnToggleRouteOverlay.toggle(false);
		mBtnTogglePoliceOverlay.toggle(false);

		setOverlay(mCommRangeOverlay, Tile::Overlay::Communications);
	}
}

void MapViewState::onTogglePoliceOverlay()
{
	clearOverlays();

	if (mBtnTogglePoliceOverlay.isPressed())
	{
		mBtnToggleCommRangeOverlay.toggle(false);
		mBtnToggleConnectedness.toggle(false);
		mBtnToggleRouteOverlay.toggle(false);

		setOverlay(mPoliceOverlays[static_cast<std::size_t>(mMapView->currentDepth())], Tile::Overlay::Police);
	}
}

void MapViewState::onToggleRouteOverlay()
{
	clearOverlays();

	if (mBtnToggleRouteOverlay.isPressed())
	{
		mBtnToggleConnectedness.toggle(false);
		mBtnToggleCommRangeOverlay.toggle(false);
		mBtnTogglePoliceOverlay.toggle(false);

		setOverlay(mTruckRouteOverlay, Tile::Overlay::TruckingRoutes);
	}
}


void MapViewState::onNotificationClicked(const NotificationArea::Notification& notification)
{
	mNotificationWindow.notification(notification);
	mNotificationWindow.show();
	mWindowStack.bringToFront(&mNotificationWindow);
}


/**
* Currently uses a text comparison function. Not inherently bad but
* should really be turned into a key/value pair table for easier lookups.
*/
void MapViewState::onStructuresSelectionChange(const IconGrid::Item* item)
{
	mConnections.clearSelection();
	mRobots.clearSelection();

	if (!item) { return; }

	// Check availability
	if (!item->available)
	{
		resourceShortageMessage(mResourcesCount, static_cast<StructureID>(item->meta));
		mStructures.clearSelection();
		return;
	}

	setStructureID(static_cast<StructureID>(item->meta), InsertMode::Structure);
}


/**
 * Handler for the Tubes Pallette dialog.
 */
void MapViewState::onConnectionsSelectionChange(const IconGrid::Item* /*item*/)
{
	mRobots.clearSelection();
	mStructures.clearSelection();

	setStructureID(StructureID::SID_TUBE, InsertMode::Tube);
}


/**
 * Handles clicks of the Robot Selection Menu.
 */
void MapViewState::onRobotsSelectionChange(const IconGrid::Item* item)
{
	mConnections.clearSelection();
	mStructures.clearSelection();

	if (!item)
	{
		clearMode();
		return;
	}

	mCurrentRobot = static_cast<Robot::Type>(item->meta);

	mInsertMode = InsertMode::Robot;
	setCursor(PointerType::PlaceTile);
}


void MapViewState::onDiggerSelectionDialog(Direction direction, Tile& tile)
{
	// Before doing anything, if we're going down and the depth is not the surface,
	// the assumption is that we've already checked and determined that there's an air shaft
	// so clear it from the tile, disconnect the tile and run a connectedness search.
	if (tile.depth() > 0 && direction == Direction::Down)
	{
		NAS2D::Utility<StructureManager>::get().removeStructure(*tile.structure());
		tile.deleteMapObject();
		updateConnectedness();
	}

	// Assumes a digger is available.
	Robodigger& robot = mRobotPool.getDigger();
	robot.startTask(tile);
	mRobotPool.insertRobotIntoTable(mRobotList, robot, tile);

	robot.direction(direction);

	const auto directionOffset = directionEnumToOffset(direction);
	if (directionOffset != DirectionCenter)
	{
		mTileMap->getTile({tile.xy() + directionOffset, tile.depth()}).excavated(true);
	}

	if (!mRobotPool.robotAvailable(Robot::Type::Digger))
	{
		mRobots.removeItem(constants::Robodigger);
		clearMode();
	}

	mDiggerDirection.visible(false);
}


/**
 * Click handler for the main menu Save Game button.
 */
void MapViewState::onOpenSaveGameDialog()
{
	mGameOptionsDialog.hide();
	mFileIoDialog.showSave(constants::SaveGamePath);
}


/**
 * Click handler for the main menu Load Game button.
 */
void MapViewState::onOpenLoadGameDialog()
{
	mGameOptionsDialog.hide();
	mFileIoDialog.showOpen(constants::SaveGamePath);
}


/**
 * Click handler for the main menu Return to Game button.
 */
void MapViewState::onReturnToGame()
{
	mGameOptionsDialog.hide();
}


/**
 * Click handler for the main menu Return to Main Menu Screen button.
 */
void MapViewState::onGameOver()
{
	mFade.fadeOut(constants::FadeSpeed);
	mQuitSignal();
}


void MapViewState::onTakeMeThere(const MapCoordinate& position)
{
	mMapView->centerOn(position);
	populateStructureMenu();
}


/**
 * Turns button clicked.
 */
void MapViewState::onTurns()
{
	nextTurn();
}

void MapViewState::onCheatCodeEntry(const std::string& cheatCode)
{
	CheatMenu::CheatCode code = CheatMenu::stringToCheatCode(cheatCode);
	switch(code)
	{
		case CheatMenu::CheatCode::Invalid:
			return;
		case CheatMenu::CheatCode::AddResources:
			addRefinedResources({1000, 1000, 1000, 1000});
		break;
		case CheatMenu::CheatCode::AddFood:
		{
			auto foodProducers = NAS2D::Utility<StructureManager>::get().getStructures<FoodProduction>();
			const auto& command = NAS2D::Utility<StructureManager>::get().getStructures<CommandCenter>();
			foodProducers.insert(foodProducers.begin(), command.begin(), command.end());

			for (auto* foodProducer : foodProducers)
			{
				foodProducer->foodLevel(foodProducer->foodCapacity());
			}
		}
		break;
		case CheatMenu::CheatCode::AddChildren:
			mPopulation.addPopulation({10, 0, 0, 0, 0});
		break;
		case CheatMenu::CheatCode::AddStudents:
			mPopulation.addPopulation({0, 10, 0, 0, 0});
		break;
		case CheatMenu::CheatCode::AddWorkers:
			mPopulation.addPopulation({0, 0, 10, 0, 0});
		break;
		case CheatMenu::CheatCode::AddScientists:
			mPopulation.addPopulation({0, 0, 0, 10, 0});
		break;
		case CheatMenu::CheatCode::AddRetired:
			mPopulation.addPopulation({0, 0, 0, 0, 10});
		break;
		case CheatMenu::CheatCode::RemoveChildren:
			mPopulation.removePopulation({10, 0, 0, 0, 0});
		break;
		case CheatMenu::CheatCode::RemoveStudents:
			mPopulation.removePopulation({0, 10, 0, 0, 0});
		break;
		case CheatMenu::CheatCode::RemoveWorkers:
			mPopulation.removePopulation({0, 0, 10, 0, 0});
		break;
		case CheatMenu::CheatCode::RemoveScientists:
			mPopulation.removePopulation({0, 0, 0, 10, 0});
		break;
		case CheatMenu::CheatCode::RemoveRetired:
			mPopulation.removePopulation({0, 0, 0, 0, 10});
		break;
		case CheatMenu::CheatCode::AddRobots:
			mRobotPool.addRobot(Robot::Type::Digger);
			mRobotPool.addRobot(Robot::Type::Dozer);
			mRobotPool.addRobot(Robot::Type::Miner);
		break;

	}
	updatePlayerResources();
	updateStructuresAvailability();
	updateFood();
	updatePopulation();
	updateRobots();
}

/**
 * Update IconGridItems availability
 */
void MapViewState::updateStructuresAvailability()
{
	for (int sid = 1; sid < StructureID::SID_COUNT; ++sid)
	{
		const StructureID id = static_cast<StructureID>(sid);
		mStructures.itemAvailable(StructureName(id), StructureCatalogue::canBuild(mResourcesCount, id));
	}
}
