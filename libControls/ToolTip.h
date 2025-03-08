#pragma once

#include "Control.h"

#include <NAS2D/Math/Point.h>
#include <NAS2D/Math/Vector.h>
#include <NAS2D/Resource/Font.h>
#include <NAS2D/Timer.h>

#include <utility>
#include <vector>
#include <optional>
#include <unordered_map>


class ToolTip : public Control
{
public:
	ToolTip();
	~ToolTip() override;

	void add(Control&, const std::string&);

	void update() override;
	void draw() const override;

	void commandSize(int width, int height);
	void commandWidth(int width);
	void commandHeight(int height);

private:
	void onMouseMove(NAS2D::Point<int> position, NAS2D::Vector<int> relative);

	void buildDrawParams(std::pair<Control*, std::string>&, int);

private:
	const NAS2D::Font& mFont;
	NAS2D::Timer mTimer;

	std::pair<Control*, std::string>* mFocusedControl{nullptr};

	std::vector<std::pair<Control*, std::string>> mControls;

	std::unordered_map<Control*, std::string> mWrappedText;

	std::optional<int> mCommandedHeight{std::nullopt};
	std::optional<int> mCommandedWidth{std::nullopt};

	bool mTruncated{false};
};
