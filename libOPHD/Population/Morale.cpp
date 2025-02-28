#include "Morale.h"

#include <algorithm>

namespace
{
	constexpr int MinimumMorale = 0;
	constexpr int MaximumMorale = 1000;
	constexpr int DefaultStartingMorale = 600;
}

Morale::Morale() :
	Morale(DefaultStartingMorale, DefaultStartingMorale)
{
}


Morale::Morale(int currentMorale, int previousMorale) :
	mCurrentMorale{currentMorale},
	mPreviousMorale{previousMorale},
	mMoraleAccumulator{0}
{
}


int Morale::currentMorale() const
{
	return std::clamp(mCurrentMorale, MinimumMorale, MaximumMorale);
}


int Morale::previousMorale() const
{
	return std::clamp(mPreviousMorale, MinimumMorale, MaximumMorale);
}


void Morale::journalMoraleChange(MoraleChangeEntry entry)
{
	mMoraleChangeJournal.push_back(std::move(entry));
}


void Morale::journalMoraleChanges(const std::vector<MoraleChangeEntry>& entries)
{
	mMoraleChangeJournal.insert(mMoraleChangeJournal.end(), entries.begin(), entries.end());
}


const std::vector<MoraleChangeEntry>& Morale::moraleChangeJournal() const
{
	return mMoraleChangeJournal;
}


void Morale::closeJournal()
{
	mMoraleChangeJournal.clear();
}


void Morale::commitMoraleChanges()
{
	mPreviousMorale = currentMorale();

	for (const auto& entry : mMoraleChangeJournal)
	{
		mMoraleAccumulator += entry.value;
	}

	mCurrentMorale = std::clamp(mCurrentMorale + mMoraleAccumulator, MinimumMorale, MaximumMorale);
	mMoraleAccumulator = 0;
}
