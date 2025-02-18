#pragma once

#include <libControls/ListBoxBase.h>

#include <NAS2D/Signal/Signal.h>


enum class StructureState;
class Structure;


/**
 * Implements a ListBox control.
 */
class StructureListBox : public ListBoxBase
{
public:
	using SelectionChangedSignal = NAS2D::Signal<Structure*>;

	struct StructureListBoxItem : public ListBoxItem
	{
		StructureListBoxItem(Structure* s, std::string stateDescription = std::string{});

		Structure* structure = nullptr; /**< Pointer to a Structure. */
		std::string structureState; /**< String description of the state of a Structure. */
		StructureState colorIndex; /**< Index to use from the listbox color table. */
	};


	StructureListBox();

	void addItem(Structure* structure, std::string stateDescription = std::string{});
	void setSelected(const Structure*);

	Structure* selectedStructure();

	StructureListBoxItem* last();

protected:
	virtual NAS2D::Color itemBorderColor(std::size_t index) const override;

	void drawItem(NAS2D::Renderer& renderer, NAS2D::Rectangle<int> drawArea, std::size_t index) const override;
};
