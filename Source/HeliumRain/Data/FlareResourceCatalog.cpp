#include "../Flare.h"
#include "FlareResourceCatalog.h"


/*----------------------------------------------------
	Constructor
----------------------------------------------------*/

UFlareResourceCatalog::UFlareResourceCatalog(const class FObjectInitializer& PCIP)
	: Super(PCIP)
{
}


/*----------------------------------------------------
	Data getters
----------------------------------------------------*/

FFlareResourceDescription* UFlareResourceCatalog::Get(FName Identifier) const
{
	auto FindByName = [=](const UFlareResourceCatalogEntry* Candidate)
	{
		return Candidate->Data.Identifier == Identifier;
	};

	UFlareResourceCatalogEntry* const* Entry = Resources.FindByPredicate(FindByName);
	if (Entry && *Entry)
	{
		return &((*Entry)->Data);
	}

	return NULL;
}