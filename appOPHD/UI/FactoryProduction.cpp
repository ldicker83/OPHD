#include "FactoryProduction.h"

#include "StringTable.h"

#include "../Constants/UiConstants.h"
#include "../Resources.h"
#include "../ProductCatalogue.h"
#include "../Constants/Strings.h"
#include "../MapObjects/Structures/Factory.h"

#include <NAS2D/Utility.h>
#include <NAS2D/Renderer/Renderer.h>


using namespace NAS2D;


FactoryProduction::FactoryProduction() :
	Window{constants::WindowFactoryProduction},
	mFactory{nullptr},
	mProduct{ProductType::PRODUCT_NONE},
	mProductGrid{"ui/factory.png", 32, constants::MarginTight},
	btnOkay{"Okay", {this, &FactoryProduction::onOkay}},
	btnCancel{"Cancel", {this, &FactoryProduction::onCancel}},
	btnClearSelection{"Clear Selection", {this, &FactoryProduction::onClearSelection}},
	btnApply{"Apply", {this, &FactoryProduction::onApply}},
	chkIdle{"Idle", {this, &FactoryProduction::onCheckBoxIdleChange}}
{
	size({320, 162});

	mProductGrid.size({140, 110});
	mProductGrid.showTooltip(true);
	mProductGrid.hide();
	mProductGrid.selectionChanged().connect({this, &FactoryProduction::onProductSelectionChange});
	add(mProductGrid, {constants::Margin, 25});

	btnOkay.size({40, 20});
	add(btnOkay, {233, 138});

	btnCancel.size({40, 20});
	add(btnCancel, {276, 138});

	btnClearSelection.size({mProductGrid.size().x, 20});
	add(btnClearSelection, {5, 138});

	btnApply.size({40, 20});
	add(btnApply, {mProductGrid.size().x + 12, btnClearSelection.positionY()});

	chkIdle.size({50, 20});
	add(chkIdle, {mProductGrid.size().x + 12, 115});
}


void FactoryProduction::clearProduct()
{
	mProduct = ProductType::PRODUCT_NONE;
	mProductCost = {};
	mProductGrid.clearSelection();
}


void FactoryProduction::hide()
{
	Control::hide();
	mFactory = nullptr;
	clearProduct();
}


void FactoryProduction::onProductSelectionChange(const IconGrid::Item* item)
{
	if (!mFactory) { return; }

	if (!item)
	{
		mProductCost = {};
		return;
	}

	mProduct = static_cast<ProductType>(item->meta);
	mProductCost = productCost(mProduct);
}


void FactoryProduction::onOkay()
{
	mFactory->productType(mProduct);
	hide();
}


void FactoryProduction::onApply()
{
	mFactory->productType(mProduct);
}


void FactoryProduction::onCancel()
{
	hide();
}


void FactoryProduction::onClearSelection()
{
	clearProduct();
	onApply();
}


void FactoryProduction::onCheckBoxIdleChange()
{
	if (!mFactory) { return; }

	mFactory->forceIdle(chkIdle.checked());
}


void FactoryProduction::factory(Factory* newFactory)
{
	mFactory = newFactory;

	if (!mFactory) { return; }

	mProductGrid.clear();
	clearProduct();

	// destroyed factories can't produce anything at all ever.
	if (mFactory->destroyed())
	{
		chkIdle.enabled(false);
		chkIdle.checked(false);
		return;
	}

	chkIdle.checked(mFactory->isIdle());

	const Factory::ProductionTypeList& ptlist = mFactory->productList();

	// The production type list from the factory should never, ever be empty. If it is there is
	// a clear logic error somewhere so fail very loudly and conspicuously here.
	if (ptlist.empty())
	{
		throw std::runtime_error("FactoryProduction::factory(): Factory provided with an empty production type list.");
	}

	for (std::size_t i = 0; i < ptlist.size(); ++i)
	{
		mProductGrid.addItem({ProductCatalogue::get(ptlist[i]).Name, ptlist[i], ptlist[i]});
	}

	if (mFactory->productType() == ProductType::PRODUCT_NONE) { mProductGrid.clearSelection(); }
	else { mProductGrid.selection_meta(static_cast<int>(mFactory->productType())); }

	mProduct = mFactory->productType();
	mProductCost = productCost(mFactory->productType());
}


void FactoryProduction::update()
{
	if (!visible()) { return; }

	Window::update();

	StringTable stringTable(2, 5);
	stringTable.position(mRect.position + NAS2D::Vector{constants::Margin * 2 + mProductGrid.size().x, 25});
	stringTable.setColumnJustification(1, StringTable::Justification::Right);

	stringTable.setColumnText(0,
		{
			"Turns Completed:",
			ResourceNamesRefined[0] + ":",
			ResourceNamesRefined[1] + ":",
			ResourceNamesRefined[2] + ":",
			ResourceNamesRefined[3] + ":",
		});

	const auto totalCost = mProductCost.resourceCost * mProductCost.turnsToBuild;
	stringTable.setColumnText(1,
		{
			std::to_string(mFactory->productionTurnsCompleted()) + " of " + std::to_string(mProductCost.turnsToBuild),
			std::to_string(totalCost.resources[0]),
			std::to_string(totalCost.resources[1]),
			std::to_string(totalCost.resources[2]),
			std::to_string(totalCost.resources[3]),
		});

	stringTable.computeRelativeCellPositions();
	stringTable.draw(Utility<Renderer>::get());
}
