#pragma once

#include "IconGrid.h"

#include "../ProductionCost.h"

#include <libOPHD/EnumProductType.h>

#include <libControls/Window.h>
#include <libControls/Button.h>
#include <libControls/CheckBox.h>


class Factory;


/**
 * Implements a Factory Production dialog interface.
 */
class FactoryProduction : public Window
{
public:
	FactoryProduction();

	void factory(Factory* newFactory);
	Factory* factory() { return mFactory; }

	void hide() override;

	void update() override;

protected:
	void onProductSelectionChange(const IconGrid::Item*);
	void onClearSelection();
	void onApply();
	void onOkay();
	void onCancel();
	void onCheckBoxIdleChange();

	void clearProduct();

private:
	Factory* mFactory;

	ProductType mProduct;
	ProductionCost mProductCost;

	IconGrid mProductGrid;

	CheckBox chkIdle;

	Button btnClearSelection;
	Button btnApply;
	Button btnOkay;
	Button btnCancel;
};
