
#include "../Flare.h"

#include "FlarePilotHelper.h"
#include "FlareTurret.h"
#include "FlareRCS.h"
#include "FlareTurretPilot.h"
#include "FlareSpacecraft.h"
#include "FlareSpacecraftComponent.h"

#include "../Player/FlarePlayerController.h"
#include "../Game/FlareGame.h"
#include "../Game/AI/FlareCompanyAI.h"

DECLARE_CYCLE_STAT(TEXT("FlareTurretPilot Tick"), STAT_FlareTurretPilot_Tick, STATGROUP_Flare);
DECLARE_CYCLE_STAT(TEXT("FlareTurretPilot Tick Target"), STAT_FlareTurretPilot_Target, STATGROUP_Flare);
DECLARE_CYCLE_STAT(TEXT("FlareTurretPilot Tick Intersect"), STAT_FlareTurretPilot_Intersect, STATGROUP_Flare);
DECLARE_CYCLE_STAT(TEXT("FlareTurretPilot Tick Intersect Gun"), STAT_FlareTurretPilot_Intersect_Gun, STATGROUP_Flare);

DECLARE_CYCLE_STAT(TEXT("FlareTurretPilot GetNearestHostileShip"), STAT_FlareTurretPilot_GetNearestHostileShip, STATGROUP_Flare);


/*----------------------------------------------------
	Constructor
----------------------------------------------------*/

UFlareTurretPilot::UFlareTurretPilot(const class FObjectInitializer& PCIP)
	: Super(PCIP)
{
	TargetSelectionReactionTime = FMath::FRandRange(1.0, 1.5);
	TimeUntilNextTargetSelectionReaction = 0;

	FireReactionTime = FMath::FRandRange(0.1, 0.2);
	TimeUntilFireReaction = 0;
	PilotTargetShip = NULL;
}


/*----------------------------------------------------
	Gameplay events
----------------------------------------------------*/


void UFlareTurretPilot::Initialize(const FFlareTurretPilotSave* Data, UFlareCompany* Company, UFlareTurret* OwnerTurret)
{
	// Main data
	Turret = OwnerTurret;
	PlayerCompany = Company;
	WantFire = false;

	// Setup properties
	if (Data)
	{
		TurretPilotData = *Data;
	}
}

void UFlareTurretPilot::TickPilot(float DeltaSeconds)
{
	SCOPE_CYCLE_COUNTER(STAT_FlareTurretPilot_Tick);

	TimeUntilNextTargetSelectionReaction -=DeltaSeconds;
	TimeUntilFireReaction -=DeltaSeconds;
	TimeUntilNextComponentSwitch-=DeltaSeconds;


	ProcessTurretTargetSelection();

	AimAxis = FVector::ZeroVector;

	if (PilotTargetShip)
	{
		if (TimeUntilNextComponentSwitch <= 0)
		{
			//FLOGV("%s Switch because of timeout", *Turret->GetReadableName());
			PilotTargetComponent = NULL;
		}
		else if (PilotTargetComponent)
		{
			if (PilotTargetComponent->GetSpacecraft() != PilotTargetShip)
			{
				//FLOGV("%s Switch because the component %s is not in the target ship", *Turret->GetReadableName(), *PilotTargetComponent->GetReadableName());
				PilotTargetComponent = NULL;
			}
			else if (PilotTargetComponent->GetUsableRatio() <=0)
			{
				//FLOGV("%s Switch because the component %s is destroyed", *Turret->GetReadableName(), *PilotTargetComponent->GetReadableName());
				PilotTargetComponent = NULL;

			}
		}

		if (!PilotTargetComponent)
		{
			PilotTargetComponent = PilotHelper::GetBestTargetComponent(PilotTargetShip);
			TimeUntilNextComponentSwitch = 10;
			//FLOGV("%s Select new target component %s ", *Turret->GetReadableName(), *PilotTargetComponent->GetReadableName());
		}


		if(!PilotTargetComponent)
		{
			PilotTargetShip = NULL;
			return;
		}

		bool DangerousTarget = IsShipDangerous(PilotTargetShip);

		// float PredictionDelay = ReactionTime - DeltaSeconds;
		float PredictionDelay = 0;
		float AmmoVelocity = Turret->GetAmmoVelocity() * 100;
		FVector TurretVelocity = 100 * Turret->GetSpacecraft()->GetLinearVelocity();


		FVector AmmoIntersectionPredictedLocation;

		FVector TurretLocation = Turret->GetTurretBaseLocation();

		float AmmoIntersectionPredictedTime = SpacecraftHelper::GetIntersectionPosition(PilotTargetComponent->GetComponentLocation(), PilotTargetShip->Airframe->GetPhysicsLinearVelocity(), TurretLocation, TurretVelocity, AmmoVelocity, PredictionDelay, &AmmoIntersectionPredictedLocation);
		FVector PredictedFireTargetLocation;
		if (AmmoIntersectionPredictedTime > 0)
		{
			PredictedFireTargetLocation = AmmoIntersectionPredictedLocation - AmmoIntersectionPredictedTime * TurretVelocity;
		}
		else
		{
			PredictedFireTargetLocation = PilotTargetComponent->GetComponentLocation();
		}



		AimAxis = (PredictedFireTargetLocation - TurretLocation).GetUnsafeNormal();
		/*FLOGV("%s Have target AimAxis=%s",*Turret->GetReadableName(),  * AimAxis.ToString());
		FLOGV("%s AmmoIntersectionPredictedTime=%f",*Turret->GetReadableName(),  AmmoIntersectionPredictedTime);
		FLOGV("%s AmmoVelocity=%f",*Turret->GetReadableName(),  AmmoVelocity);*/



		float TargetSize = PilotTargetShip->GetMeshScale() / 100.f + Turret->GetAimRadius() * 2; // Radius in meters
		FVector DeltaLocation = (PilotTargetComponent->GetComponentLocation()-TurretLocation) / 100.f;
		float Distance = DeltaLocation.Size(); // Distance in meters

		//FLOGV("%s Distance=%f",*Turret->GetReadableName(),  Distance);

		// If at range and aligned fire on the target
		// TODO increase tolerance if target is near


		if (AmmoIntersectionPredictedTime > 0 && AmmoIntersectionPredictedTime < 10.f && TimeUntilFireReaction < 0)
		{
			SCOPE_CYCLE_COUNTER(STAT_FlareTurretPilot_Intersect);

			TimeUntilFireReaction = FireReactionTime;
			WantFire = false;

			//FLOG("Near enough");
			FVector FireAxis = Turret->GetFireAxis();


			for (int GunIndex = 0; GunIndex < Turret->GetGunCount(); GunIndex++)
			{
				SCOPE_CYCLE_COUNTER(STAT_FlareTurretPilot_Intersect_Gun);

				FVector MuzzleLocation = Turret->GetMuzzleLocation(GunIndex);

				// Compute target Axis for each gun
				FVector AmmoIntersectionLocation;
				float AmmoIntersectionTime = SpacecraftHelper::GetIntersectionPosition(PilotTargetComponent->GetComponentLocation(), PilotTargetShip->Airframe->GetPhysicsLinearVelocity(), MuzzleLocation, TurretVelocity , AmmoVelocity, 0, &AmmoIntersectionLocation);
				if (AmmoIntersectionTime < 0)
				{
					// No ammo intersection, don't fire
					continue;
				}
				FVector FireTargetAxis = (AmmoIntersectionLocation - MuzzleLocation - AmmoIntersectionPredictedTime * TurretVelocity).GetUnsafeNormal();
				/*FLOGV("Gun %d FireAxis=%s", GunIndex, *FireAxis.ToString());
				FLOGV("Gun %d FireTargetAxis=%s", GunIndex, *FireTargetAxis.ToString());*/

				float AngularPrecisionDot = FVector::DotProduct(FireTargetAxis, FireAxis);
				float AngularPrecision = FMath::Acos(AngularPrecisionDot);
				float AngularSize = FMath::Atan(TargetSize / Distance);

				/*FLOGV("Gun %d Distance=%f", GunIndex, Distance);
				FLOGV("Gun %d TargetSize=%f", GunIndex, TargetSize);
				FLOGV("Gun %d AngularSize=%f", GunIndex, AngularSize);
				FLOGV("Gun %d AngularPrecision=%f", GunIndex, AngularPrecision);*/
				if (AngularPrecision < (DangerousTarget ? AngularSize * 0.25 : AngularSize * 0.2))
				{
					if (!PilotHelper::CheckFriendlyFire(Turret->GetSpacecraft()->GetGame()->GetActiveSector(), PlayerCompany, MuzzleLocation, TurretVelocity, AmmoVelocity, FireTargetAxis, AmmoIntersectionTime, Turret->GetAimRadius()))
					{
						Turret->SetTarget(PilotTargetShip);

						WantFire = true;
						break;
					}
					else
					{
						//FLOG("Friendly fire avoidance");
					}
				}
			}
		}
	}
	else
	{
		WantFire = false;
	}
}

/*----------------------------------------------------
	Helpers
----------------------------------------------------*/


void UFlareTurretPilot::ProcessTurretTargetSelection()
{

	if (TimeUntilNextTargetSelectionReaction > 0)
	{
		if(PilotTargetShip && !PilotTargetShip->GetParent()->GetDamageSystem()->IsAlive())
		{
			PilotTargetShip = NULL;
		}

		return;
	}
	else
	{
		TimeUntilNextTargetSelectionReaction = TargetSelectionReactionTime;
	}


	AFlareSpacecraft* OldPilotTargetShip = PilotTargetShip;

	EFlarePartSize::Type PreferredShipSize;
	EFlarePartSize::Type SecondaryShipSize;
	if (Turret->GetDescription()->WeaponCharacteristics.DamageType == EFlareShellDamageType::HEAT)
	{
		PreferredShipSize = EFlarePartSize::L;
		SecondaryShipSize = EFlarePartSize::S;
	}
	else
	{
		PreferredShipSize = EFlarePartSize::S;
		SecondaryShipSize = EFlarePartSize::L;
	}

	EFlareCombatTactic::Type Tactic = Turret->GetSpacecraft()->GetParent()->GetCompany()->GetTacticManager()->GetCurrentTacticForShipGroup(EFlareCombatGroup::Capitals);

	PilotTargetShip = GetNearestHostileShip(true, Tactic);

	if (Turret->GetWeaponGroup()->Target)
	{
		SCOPE_CYCLE_COUNTER(STAT_FlareTurretPilot_Target);

		AFlareSpacecraft* TargetCandidate = Turret->GetWeaponGroup()->Target;

		if(!TargetCandidate->GetParent()->GetDamageSystem()->IsAlive())
		{
			Turret->GetWeaponGroup()->Target = NULL;
		}
		else
		{
			FVector TargetAxis = (TargetCandidate->GetActorLocation()- Turret->GetTurretBaseLocation()).GetUnsafeNormal();
			if(Turret->IsReacheableAxis(TargetAxis))
			{
				PilotTargetShip = TargetCandidate;
			}
		}
	}

	if (!PilotTargetShip)
	{
		PilotTargetShip = GetNearestHostileShip(false, Tactic);
	}
}

AFlareSpacecraft* UFlareTurretPilot::GetNearestHostileShip(bool ReachableOnly, EFlareCombatTactic::Type Tactic) const
{
	SCOPE_CYCLE_COUNTER(STAT_FlareTurretPilot_GetNearestHostileShip);

	// For now an host ship is a the nearest host ship with the following critera:
	// - Alive
	// - Is dangerous if needed
	// - From another company
	// - Is the nearest

	float SecurityRadius = 0;

	if (Turret->GetDescription()->WeaponCharacteristics.FuzeType == EFlareShellFuzeType::Proximity)
	{
		 SecurityRadius = Turret->GetDescription()->WeaponCharacteristics.AmmoExplosionRadius + Turret->GetSpacecraft()->GetMeshScale() / 100;
	}

	FVector PilotLocation = Turret->GetTurretBaseLocation();
	float MaxDot = 0;
	AFlareSpacecraft* NearestHostileShip = NULL;
	FVector FireAxis = Turret->GetFireAxis();



	struct PilotHelper::TargetPreferences TargetPreferences;
	TargetPreferences.IsLarge = 1;
	TargetPreferences.IsSmall = 1;
	TargetPreferences.IsStation = 1;
	TargetPreferences.IsNotStation = 1;
	TargetPreferences.IsMilitary = 1;
	TargetPreferences.IsNotMilitary = 0.1;
	TargetPreferences.IsDangerous = 1;
	TargetPreferences.IsNotDangerous = 0.01;
	TargetPreferences.IsStranded = 1;
	TargetPreferences.IsNotStranded = 0.5;
	TargetPreferences.IsUncontrollableCivil = 0.0;
	TargetPreferences.IsUncontrollableMilitary = 0.01;
	TargetPreferences.IsNotUncontrollable = 1;
	TargetPreferences.TargetStateWeight = 1;
	TargetPreferences.MaxDistance = 5000000;
	TargetPreferences.DistanceWeight = 0.1;
	TargetPreferences.AttackTarget = NULL;
	TargetPreferences.AttackTargetWeight = 1;
	TargetPreferences.LastTarget = PilotTargetShip;
	TargetPreferences.LastTargetWeight = 10;

	TargetPreferences.PreferredDirection = FireAxis;
	TargetPreferences.MinAlignement = -1;
	TargetPreferences.AlignementWeight = 1.0;
	TargetPreferences.BaseLocation = PilotLocation;



	if (Turret->GetDescription()->WeaponCharacteristics.DamageType == EFlareShellDamageType::HEAT)
	{
		TargetPreferences.IsLarge = 1.0f;
		TargetPreferences.IsSmall = 0.1f;
	}
	else
	{
		TargetPreferences.IsLarge = 0.1f;
		TargetPreferences.IsSmall = 1.0f;
	}

	if (Tactic == EFlareCombatTactic::AttackStations)
	{
		TargetPreferences.IsStation = 10;
	}
	else if (Tactic == EFlareCombatTactic::AttackMilitary)
	{
		TargetPreferences.IsStation = 0.1;
	}
	else if (Tactic == EFlareCombatTactic::AttackCivilians)
	{
		TargetPreferences.IsMilitary = 0.1;
		TargetPreferences.IsNotMilitary = 1.0;
		TargetPreferences.IsNotDangerous = 1.0;
	}
	else if (Tactic == EFlareCombatTactic::ProtectMe)
	{
		// Protect me is only available for player ship
		if (Turret->GetSpacecraft()->GetParent()->GetCompany() == Turret->GetSpacecraft()->GetGame()->GetPC()->GetCompany())
		{
			TargetPreferences.AttackTarget = Turret->GetSpacecraft()->GetGame()->GetPC()->GetShipPawn();
			TargetPreferences.AttackTargetWeight = 1.0;
		}
	}


	while (NearestHostileShip == NULL)
	{
		NearestHostileShip = PilotHelper::GetBestTarget(Turret->GetSpacecraft(), TargetPreferences);

		if(NearestHostileShip == NULL)
		{
			// No target
			return NULL;
		}


		float Distance = (PilotLocation - NearestHostileShip->GetActorLocation()).Size();
		if (Distance < SecurityRadius * 100)
		{
			TargetPreferences.IgnoreList.Add(NearestHostileShip);
			NearestHostileShip = NULL;
			continue;
		}

		FVector TargetAxis = (NearestHostileShip->GetActorLocation()- PilotLocation).GetUnsafeNormal();

		if (ReachableOnly && !Turret->IsReacheableAxis(TargetAxis))
		{
			TargetPreferences.IgnoreList.Add(NearestHostileShip);
			NearestHostileShip = NULL;
			continue;
		}
	}
	return NearestHostileShip;
}

bool UFlareTurretPilot::IsShipDangerous(AFlareSpacecraft* ShipCandidate) const
{
	return ShipCandidate->GetParent()->IsMilitary() && ShipCandidate->GetParent()->GetDamageSystem()->GetSubsystemHealth(EFlareSubsystem::SYS_Weapon) > 0;
}


/*----------------------------------------------------
	Pilot Output
----------------------------------------------------*/

FVector UFlareTurretPilot::GetTargetAimAxis() const
{
	return AimAxis;
}

bool UFlareTurretPilot::IsWantFire() const
{
	return WantFire;
}
