#pragma once

#include "../Flare.h"
#include "FlareHUD.h"
#include "FlareMenuPawn.h"
#include "FlareSoundManager.h"
#include "FlareCockpitManager.h"
#include "../Game/FlareGame.h"
#include "../Game/FlareSaveGame.h"
#include "../UI/Components/FlareMainOverlay.h"
#include "FlarePlayerController.generated.h"


class AFlareMenuManager;
class AFlareHUD;


UCLASS(MinimalAPI)
class AFlarePlayerController : public APlayerController
{
	GENERATED_UCLASS_BODY()

public:

	/*----------------------------------------------------
			Gameplay methods
	----------------------------------------------------*/

	virtual void BeginPlay() override;

	virtual void PlayerTick(float DeltaTime) override;
	
	/** Activate or deactivate the exterbal camera */
	void SetExternalCamera(bool NewState);

	/** Fly this ship */
	void FlyShip(AFlareSpacecraft* Ship, bool PossessNow = true);

	/** Stop flying a ship */
	void ExitShip();

	/** The world is ending. Literally. */
	void PrepareForExit();

	/** Set the pause state */
	void SetWorldPause(bool Pause);
	
	/** Get the currently selected fleet */
	UFlareFleet* GetPlayerFleet();

	/** Quick switch to another ship */
	bool SwitchToNextShip(bool Instant = false);

	/** Is the player ship being targeted ? Get one of the attackers too. */
	void GetPlayerShipThreatStatus(bool& IsTargeted, bool& IsFiredUpon, UFlareSimulatedSpacecraft*& Threat) const;

	/** Get a recovery ship */
	void ActivateRecovery();


	/*----------------------------------------------------
		Data management
	----------------------------------------------------*/

	/** Load the company info */
	void SetCompanyDescription(const FFlareCompanyDescription& SaveCompanyData);

	/** Load the player from a save file */
	void Load(const FFlarePlayerSave& SavePlayerData);

	/** Get the ship pawn from the game */
	void OnLoadComplete();

	/** Save the player to a save file */
	void Save(FFlarePlayerSave& SavePlayerData, FFlareCompanyDescription& SaveCompanyData);

	/** Set the player's company */
	void SetCompany(UFlareCompany* NewCompany);
	
	/** Call a sector is activated */
	void OnSectorActivated(UFlareSector* ActiveSector);

	/** Call a sector is deactivated */
	void OnSectorDeactivated();

	/** The battle state has changed, update music, notify, etc */
	void OnBattleStateChanged(EFlareSectorBattleState::Type NewBattleState);

	/** Set the currently flown player ship */
	void SetPlayerShip(UFlareSimulatedSpacecraft* NewPlayerShip);

	/** We just hit this spacecraft with a weapon */
	void SignalHit(AFlareSpacecraft* HitSpacecraft, EFlareDamage::Type DamageType);

	/** Cleanup the PC owned stuff */
	void Clean();


	/*----------------------------------------------------
		Menus
	----------------------------------------------------*/

	/** Show a notification to the user */
	void Notify(FText Text, FText Info, FName Tag, EFlareNotification::Type Type = EFlareNotification::NT_Info, bool Pinned = false, EFlareMenu::Type TargetMenu = EFlareMenu::MENU_None, FFlareMenuParameterData TargetInfo = FFlareMenuParameterData());

	/** Setup the cockpit */
	void SetupCockpit();

	/** Spawn the menu pawn and prepare the UI */
	void SetupMenu();
	
	/** Entering the main menu */
	void OnEnterMenu();

	/** Exiting the main menu */
	void OnExitMenu();

	/** Check wether we are in the menu system */
	bool IsInMenu();

	/** Get the position of the mouse on the screen */
	FVector2D GetMousePosition();

	/** Reset the mouse to the center of the screen */
	void ResetMousePosition();

	/** Signal that we are selecting weapons */
	void SetSelectingWeapon();

	/** id we select a weapon recently ? */
	bool IsSelectingWeapon() const;

	/** Show a notification explaining if we succeeded in docking */
	void NotifyDockingResult(bool Success, UFlareSimulatedSpacecraft* Target);

	/** Check the battle state of the game before FF */
	bool ConfirmFastForward(FSimpleDelegate OnConfirmed);


	/*----------------------------------------------------
		Objectives
	----------------------------------------------------*/

	/** Start a new objective */
	void StartObjective(FText Name, FFlarePlayerObjectiveData Data);

	/** Finalize the objective */
	void CompleteObjective();

	/** Check if there is an objective yet */
	bool HasObjective() const;

	/** Get the raw objective data */
	const FFlarePlayerObjective* GetCurrentObjective() const;


	/*----------------------------------------------------
		Customization
	----------------------------------------------------*/

	/** Set the color of engine exhausts */
	void SetBasePaintColorIndex(int32 Index);

	/** Set the color of ship paint */
	void SetPaintColorIndex(int32 Index);

	/** Set the color of ship overlays */
	void SetOverlayColorIndex(int32 Index);

	/** Set the color of ship lights */
	void SetLightColorIndex(int32 Index);

	/** Set the pattern index for ship paint */
	void SetPatternIndex(int32 Index);


	/*----------------------------------------------------
		Input
	----------------------------------------------------*/

	virtual void SetupInputComponent() override;

	/** Secondary input that is used whil in external camera */
	void MousePositionInput(FVector2D Val);

	/** Toggle the external view */
	void ToggleCamera();

	/** Toggle the menus */
	void ToggleMenu();

	/** Toggle the overlay */
	void ToggleOverlay();

	/** Send back to menu */
	void BackMenu();

	/** Simulate a turn */
	void Simulate();

	/** Simulate a turn */
	void SimulateConfirmed();

	// Menus
	void ShipMenu();
	void SectorMenu();
	void OrbitMenu();
	void LeaderboardMenu();
	void CompanyMenu();
	void FleetMenu();
	void QuestMenu();
	void MainMenu();
	void SettingsMenu();

	/** Toggle the combat mode */
	void ToggleCombat();

	/** Toggle the pilot mode */
	void TogglePilot();

	/** Toggle the HUD's presence */
	void ToggleHUD();

	/** Quick switch */
	void QuickSwitch();

	/** Move hidden cursor */
	void MouseInputX(float Val);

	/** Move hidden cursor */
	void MouseInputY(float Val);

	/** Joystick lateral movement X */
	void JoystickMoveHorizontalInput(float Val);

	/** Joystick lateral movement Y */
	void JoystickMoveVerticalInput(float Val);
	
	/** Test method 1 */
	void Test1();

	/** Test method 2 */
	void Test2();


	/*----------------------------------------------------
		Wheel menu
	----------------------------------------------------*/

	/** Open the wheel menu */
	void WheelPressed();

	/** Close the wheel menu */
	void WheelReleased();

	/** Align to the ship's speed */
	void AlignToSpeed();

	/** Align to the ship's reverse speed */
	void AlignToReverse();

	/** Brake */
	void Brake();

	/** Inspect the target */
	void InspectTargetSpacecraft();

	/** Fly the target */
	void FlyTargetSpacecraft();

	/** Dock at the target */
	void DockAtTargetSpacecraft();

	/** Set the required combat tactic */
	void SetTacticForCurrentGroup(EFlareCombatTactic::Type Tactic);

	/** Match speed with target spacecraft */
	void MatchSpeedWithTargetSpacecraft();

	/** Find the target spacecraft */
	void LookAtTargetSpacecraft();

	/** Open the upgrade menu */
	void UpgradeShip();

	/** Undock */
	void UndockShip();

	/** trade with the station we're docked to */
	void StartTrading();


	/*----------------------------------------------------
		Test
	----------------------------------------------------*/

	bool                                     IsTest1;
	bool                                     IsTest2;


protected:
	
	/*----------------------------------------------------
		Gameplay data
	----------------------------------------------------*/

	// Sound manager
	UPROPERTY()
	UFlareSoundManager*                      SoundManager;

	/** Dust effect template */
	UPROPERTY()
	UParticleSystem*                         DustEffectTemplate;

	/** Dust effect */
	UPROPERTY()
	UParticleSystemComponent*                DustEffect;

	/** Player save structure */
	UPROPERTY()
	FFlarePlayerSave                         PlayerData;

	/** Company data */
	UPROPERTY()
	FFlareCompanyDescription                 CompanyData;

	/** Player pawn used inside menus */
	UPROPERTY()
	AFlareMenuPawn*                          MenuPawn;

	/** Currently flown ship */
	UPROPERTY()
	AFlareSpacecraft*                        ShipPawn;

	/** Currently flown ship */
	UPROPERTY()
	UFlareSimulatedSpacecraft*               PlayerShip;

	/** Player fleet */
	UPROPERTY()
	UFlareFleet*                             PlayerFleet;

	/** Player owned company */
	UPROPERTY()
	UFlareCompany*                           Company;

	/** Menu management */
	UPROPERTY()
	AFlareMenuManager*                       MenuManager;

	/** Cockpit system */
	UPROPERTY()
	AFlareCockpitManager*                    CockpitManager;

	/** Objective */
	UPROPERTY()
	FFlarePlayerObjective                    CurrentObjective;

	// Various gameplay data
	int32                                    QuickSwitchNextOffset;
	float                                    WeaponSwitchTime;
	float                                    TimeSinceWeaponSwitch;
	TEnumAsByte<EFlareSectorBattleState::Type> LastBattleState;
	bool									RecoveryActive;
public:

	/*----------------------------------------------------
		Config
	----------------------------------------------------*/
		
	/** Whether to use the 3D cockpit */
	bool                                     UseCockpit;

	/** Whether to use motion blur */
	bool                                     UseMotionBlur;

	/** Whether to use temporal AA */
	bool                                     UseTemporalAA;

	/** Whether to pause the game in menus */
	bool                                     PauseGameInMenus;
	
	void SetUseCockpit(bool New);

	void SetUseMotionBlur(bool New);

	void SetUseTemporalAA(bool New);

	void SetPauseGameInMenus(bool New);

	void SetMusicVolume(int32 New);

	void SetMasterVolume(int32 New);


public:

	/*----------------------------------------------------
		Getters for game classes
	----------------------------------------------------*/

	inline UFlareCompany* GetCompany() const
	{
		return Company;
	}

	inline const FFlareCompanyDescription* GetCompanyDescription() const
	{
		return &CompanyData;
	}

	inline AFlareMenuPawn* GetMenuPawn() const
	{
		return MenuPawn;
	}

	inline AFlareSpacecraft* GetShipPawn() const
	{
		return ShipPawn;
	}

	inline AFlareGame* GetGame() const
	{
		return Cast<AFlareGame>(GetWorld()->GetAuthGameMode());
	}

	inline AFlareMenuManager* GetMenuManager() const
	{
		return MenuManager;
	}

	inline AFlareHUD* GetNavHUD() const
	{
		return Cast<AFlareHUD>(GetHUD());
	}

	inline AFlareCockpitManager* GetCockpitManager()
	{
		return CockpitManager;
	}

	inline UFlareSoundManager* GetSoundManager()
	{
		return SoundManager;
	}

	inline UFlareTacticManager* GetTacticManager()
	{
		return GetCompany()->GetTacticManager();
	}

	/** Return the last flown ship. Return NULL if no last flown ship, or if it is destroyed */
	UFlareSimulatedSpacecraft* GetPlayerShip();

	FFlarePlayerSave* GetPlayerData()
	{
		return &PlayerData;
	}
};

