
#include "../Flare.h"
#include "FlareFleet.h"
#include "FlareCompany.h"
#include "FlareGame.h"
#include "../Player/FlarePlayerController.h"
#include "FlareSimulatedSector.h"


#define LOCTEXT_NAMESPACE "FlareFleet"


/*----------------------------------------------------
	Constructor
----------------------------------------------------*/

UFlareFleet::UFlareFleet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UFlareFleet::Load(const FFlareFleetSave& Data)
{
	FleetCompany = Cast<UFlareCompany>(GetOuter());
	Game = FleetCompany->GetGame();
	FleetData = Data;
	IsShipListLoaded = false;
}

FFlareFleetSave* UFlareFleet::Save()
{
	return &FleetData;
}


/*----------------------------------------------------
	Gameplay
----------------------------------------------------*/

FText UFlareFleet::GetName()
{
	if (GetShips().Num() > 0)
	{
		return FText::FromString(GetShips()[0]->GetImmatriculation().ToString());// TODO Clean with GetFleetName
	}
	else
	{
		return GetFleetName();
	}
}

FText  UFlareFleet::GetFleetName() const
{
	if(Game->GetPC()->GetPlayerFleet() == this)
	{
		return LOCTEXT("PlayerFleetName", "Player Fleet");
	}

	return FleetData.Name;
}

bool UFlareFleet::IsTraveling() const
{
	return CurrentTravel != NULL;
}

bool UFlareFleet::IsTrading() const
{
	return GetTradingShipCount() > 0;
}


bool UFlareFleet::CanTravel()
{
	if (IsTraveling() && !GetCurrentTravel()->CanChangeDestination())
	{
		return false;
	}

	if (GetImmobilizedShipCount() == FleetShips.Num())
	{
		// All ship are immobilized
		return false;
	}

	if(Game->GetPC()->GetPlayerFleet() == this && Game->GetPC()->GetPlayerShip()->GetDamageSystem()->IsStranded())
	{
		// The player ship is stranded
		return false;
	}

	return true;
}

bool UFlareFleet::CanTravel(FText& OutInfo)
{
	if (IsTraveling() && !GetCurrentTravel()->CanChangeDestination())
	{
		OutInfo = LOCTEXT("TravellingFormat", "Can't change destination");
		return false;
	}

	if (GetImmobilizedShipCount() == FleetShips.Num())
	{
		OutInfo = LOCTEXT("Travelling", "Some ships are trading or stranded");
		return false;
	}

	if(Game->GetPC()->GetPlayerFleet() == this && Game->GetPC()->GetPlayerShip()->GetDamageSystem()->IsStranded())
	{
		OutInfo = LOCTEXT("PlayerShipStranded", "The ship you are piloting is stranded");
		return false;
	}

	return true;
}

uint32 UFlareFleet::GetImmobilizedShipCount()
{
	uint32 ImmobilizedShip = 0;

	for (int ShipIndex = 0; ShipIndex < FleetShips.Num(); ShipIndex++)
	{
		if (!FleetShips[ShipIndex]->CanTravel())
		{
			ImmobilizedShip++;
		}
	}
	return ImmobilizedShip;
}

uint32 UFlareFleet::GetTradingShipCount() const
{
	uint32 TradingShip = 0;

	for (int ShipIndex = 0; ShipIndex < FleetShips.Num(); ShipIndex++)
	{
		if (FleetShips[ShipIndex]->IsTrading())
		{
			TradingShip++;
		}
	}
	return TradingShip;
}

uint32 UFlareFleet::GetShipCount() const
{
	return FleetShips.Num();
}

uint32 UFlareFleet::GetMaxShipCount()
{
	return 20;
}

FText UFlareFleet::GetStatusInfo() const
{
	if (IsTraveling())
	{
		int64 RemainingDuration = CurrentTravel->GetRemainingTravelDuration();
		return FText::Format(LOCTEXT("TravelTextFormat", "Travelling to {0} ({1} left)"),
			CurrentTravel->GetDestinationSector()->GetSectorName(),
			FText::FromString(*UFlareGameTools::FormatDate(RemainingDuration, 1))); //FString needed here
	}
	else if (IsTrading())
	{
		if(GetTradingShipCount() == GetShipCount())
		{
			return FText::Format(LOCTEXT("FleetTrading", "Trading in {0}"), GetCurrentSector()->GetSectorName());
		}
		else
		{
			return FText::Format(LOCTEXT("FleetPartialTrading", "{0} of {1} ships are trading in {2}"), FText::AsNumber(GetTradingShipCount()), FText::AsNumber(GetShipCount()), GetCurrentSector()->GetSectorName());
		}
	}
	else
	{
		return FText::Format(LOCTEXT("FleetIdle", "Idle in {0}"), GetCurrentSector()->GetSectorName());
	}

	return FText();
}


int32 UFlareFleet::GetFleetCapacity() const
{
	int32 FreeCargoSpace = 0;

	for (int ShipIndex = 0; ShipIndex < FleetShips.Num(); ShipIndex++)
	{
		FreeCargoSpace += FleetShips[ShipIndex]->GetCargoBay()->GetCapacity();
	}
	return FreeCargoSpace;
}

int32 UFlareFleet::GetFleetFreeCargoSpace() const
{
	int32 FreeCargoSpace = 0;

	for (int ShipIndex = 0; ShipIndex < FleetShips.Num(); ShipIndex++)
	{
		FreeCargoSpace += FleetShips[ShipIndex]->GetCargoBay()->GetFreeCargoSpace();
	}
	return FreeCargoSpace;
}

void UFlareFleet::RemoveImmobilizedShips()
{
	TArray<UFlareSimulatedSpacecraft*> ShipToRemove;

	for (int ShipIndex = 0; ShipIndex < FleetShips.Num(); ShipIndex++)
	{
		if (!FleetShips[ShipIndex]->CanTravel())
		{
			ShipToRemove.Add(FleetShips[ShipIndex]);
		}
	}

	for (int ShipIndex = 0; ShipIndex < ShipToRemove.Num(); ShipIndex++)
	{
		RemoveShip(ShipToRemove[ShipIndex]);
	}
}

void UFlareFleet::AddShip(UFlareSimulatedSpacecraft* Ship)
{
	if (IsTraveling())
	{
		FLOGV("Fleet Disband fail: '%s' is travelling", *GetFleetName().ToString());
		return;
	}

	if (GetCurrentSector() != Ship->GetCurrentSector())
	{
		FLOGV("Fleet Merge fail: '%s' is the sector '%s' but '%s' is the sector '%s'",
			  *GetFleetName().ToString(),
			  *GetCurrentSector()->GetSectorName().ToString(),
			  *Ship->GetImmatriculation().ToString(),
			  *Ship->GetCurrentSector()->GetSectorName().ToString());
		return;
	}

	UFlareFleet* OldFleet = Ship->GetCurrentFleet();
	if (OldFleet)
	{
		OldFleet->RemoveShip(Ship);
	}

	FleetData.ShipImmatriculations.Add(Ship->GetImmatriculation());
	FleetShips.AddUnique(Ship);
	Ship->SetCurrentFleet(this);
}

void UFlareFleet::RemoveShip(UFlareSimulatedSpacecraft* Ship, bool destroyed)
{
	if (IsTraveling())
	{
		FLOGV("Fleet RemoveShip fail: '%s' is travelling", *GetFleetName().ToString());
		return;
	}

	FleetData.ShipImmatriculations.Remove(Ship->GetImmatriculation());
	FleetShips.Remove(Ship);
	Ship->SetCurrentFleet(NULL);

	if (!destroyed)
	{
		Ship->GetCompany()->CreateAutomaticFleet(Ship);
	}

	if(FleetShips.Num() == 0)
	{
		Disband();
	}
}

/** Remove all ship from the fleet and delete it. Not possible during travel */
void UFlareFleet::Disband()
{
	if (IsTraveling())
	{
		FLOGV("Fleet Disband fail: '%s' is travelling", *GetFleetName().ToString());
		return;
	}

	for (int ShipIndex = 0; ShipIndex < FleetShips.Num(); ShipIndex++)
	{
		FleetShips[ShipIndex]->SetCurrentFleet(NULL);
	}

	if (GetCurrentTradeRoute())
	{
		GetCurrentTradeRoute()->RemoveFleet(this);
	}
	GetCurrentSector()->DisbandFleet(this);
	FleetCompany->RemoveFleet(this);
}

bool UFlareFleet::CanMerge(UFlareFleet* Fleet, FText& OutInfo)
{
	if (GetShipCount() + Fleet->GetShipCount() > GetMaxShipCount())
	{
		OutInfo = LOCTEXT("MergeFleetMaxShips", "Can't add, max ships reached");
		return false;
	}

	if (IsTraveling())
	{
		OutInfo = LOCTEXT("MergeFleetTravel", "Can't add during travel");
		return false;
	}

	if (Fleet->IsTraveling())
	{
		OutInfo = LOCTEXT("MergeOtherFleetTravel", "Can't add travelling ships");
		return false;
	}

	if (GetCurrentSector() != Fleet->GetCurrentSector())
	{
		OutInfo = LOCTEXT("MergeFleetDifferenSector", "Can't add from a different sector");
		return false;
	}

	return true;
}

void UFlareFleet::Merge(UFlareFleet* Fleet)
{
	FText Unused;
	if (!CanMerge(Fleet, Unused))
	{
		FLOGV("Fleet Merge fail: '%s' is not mergeable", *Fleet->GetFleetName().ToString());
		return;
	}

	TArray<UFlareSimulatedSpacecraft*> Ships = Fleet->GetShips();
	Fleet->Disband();
	for (int ShipIndex = 0; ShipIndex < Ships.Num(); ShipIndex++)
	{
		AddShip(Ships[ShipIndex]);
	}
}

void UFlareFleet::SetCurrentSector(UFlareSimulatedSector* Sector)
{
	CurrentSector = Sector;
	if(!Sector->IsTravelSector())
	{
		CurrentTravel = NULL;
	}
	InitShipList();
}

void UFlareFleet::SetCurrentTravel(UFlareTravel* Travel)
{
	CurrentSector = Travel->GetTravelSector();
	CurrentTravel = Travel;
	InitShipList();
	for (int ShipIndex = 0; ShipIndex < FleetShips.Num(); ShipIndex++)
	{
		FleetShips[ShipIndex]->SetSpawnMode(EFlareSpawnMode::Travel);
	}
}

void UFlareFleet::InitShipList()
{
	if (!IsShipListLoaded)
	{
		IsShipListLoaded = true;
		FleetShips.Empty();
		for (int ShipIndex = 0; ShipIndex < FleetData.ShipImmatriculations.Num(); ShipIndex++)
		{
			UFlareSimulatedSpacecraft* Ship = FleetCompany->FindSpacecraft(FleetData.ShipImmatriculations[ShipIndex]);
			if (!Ship)
			{
				FLOGV("WARNING: Fail to find ship with id %s in company %s for fleet %s (%d ships)",
						*FleetData.ShipImmatriculations[ShipIndex].ToString(),
						*FleetCompany->GetCompanyName().ToString(),
						*GetFleetName().ToString(),
						FleetData.ShipImmatriculations.Num());
				continue;
			}
			Ship->SetCurrentFleet(this);
			FleetShips.Add(Ship);
		}
	}
}


/*----------------------------------------------------
	Getters
----------------------------------------------------*/

TArray<UFlareSimulatedSpacecraft*>& UFlareFleet::GetShips()
{
	InitShipList();

	return FleetShips;
}

#undef LOCTEXT_NAMESPACE
