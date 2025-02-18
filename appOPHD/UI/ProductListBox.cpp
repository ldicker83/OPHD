#include "ProductListBox.h"

#include "ProgressBar.h"

#include "../Cache.h"
#include "../Constants/UiConstants.h"
#include "../ProductCatalogue.h"
#include "../ProductPool.h"

#include <NAS2D/Utility.h>
#include <NAS2D/Renderer/Renderer.h>


using namespace NAS2D;


ProductListBox::ProductListBox() :
	ListBoxBase{
		fontCache.load(constants::FONT_PRIMARY, 12),
		fontCache.load(constants::FONT_PRIMARY_BOLD, 12)
	}
{
	itemHeight(30);
}


/**
 * Fills the ProductListBox with Products in a ProductPool object.
 */
void ProductListBox::productPool(const ProductPool& pool)
{
	clear();

	const auto productTypeCount = static_cast<std::size_t>(ProductType::PRODUCT_COUNT);

	for (std::size_t productTypeIndex = 0; productTypeIndex < productTypeCount; ++productTypeIndex)
	{
		const auto productType = static_cast<ProductType>(productTypeIndex);
		const auto productCount = pool.count(productType);

		if (productCount > 0)
		{
			mItems.emplace_back(new ProductListBoxItem{
				ProductCatalogue::get(productType).Name,
				productCount,
				productCount * pool.productStorageRequirement(productType),
				pool.capacity(),
			});
		}
	}

	updateScrollLayout();
}


const ProductListBox::ProductListBoxItem& ProductListBox::getItem(std::size_t index) const
{
	return *static_cast<ProductListBoxItem*>(mItems[index]);
}


void ProductListBox::drawItem(NAS2D::Renderer& renderer, NAS2D::Rectangle<int> drawArea, std::size_t index) const
{
	const auto firstStop = drawArea.size.x / 3;
	const auto secondStop = drawArea.size.x * 2 / 3;
	const auto& item = getItem(index);

	// Draw column breaks
	renderer.drawLine(drawArea.position + NAS2D::Vector{firstStop, 2}, drawArea.position + NAS2D::Vector{firstStop, drawArea.size.y - 2}, constants::PrimaryColor);
	renderer.drawLine(drawArea.position + NAS2D::Vector{secondStop, 2}, drawArea.position + NAS2D::Vector{secondStop, drawArea.size.y - 2}, constants::PrimaryColor);

	// Draw item column contents
	renderer.drawText(mFontBold, item.text, drawArea.position + NAS2D::Vector{5, 15 - mFontBold.height() / 2}, constants::PrimaryColor);
	renderer.drawText(mFont, "Quantity: " + std::to_string(item.productCount), drawArea.position + NAS2D::Vector{firstStop + 5, 15 - mFontBold.height() / 2}, constants::PrimaryColor);
	drawProgressBar(
		item.capacityUsed,
		item.capacityTotal,
		{drawArea.position + NAS2D::Vector{secondStop + 5, 10}, {firstStop - 10, 10}},
		2
	);
}
