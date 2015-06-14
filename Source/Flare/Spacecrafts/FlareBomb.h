#pragma once

#include "FlareWeapon.h"
#include "FlareBomb.generated.h"

class UFlareBombComponent;

UCLASS(Blueprintable, ClassGroup = (Flare, Ship), meta = (BlueprintSpawnableComponent))
class AFlareBomb : public AActor
{
public:

	GENERATED_UCLASS_BODY()

public:

	/*----------------------------------------------------
		Public methods
	----------------------------------------------------*/

	virtual void PostInitializeComponents() override;

	/** Properties setup */
	void Initialize(UFlareWeapon* Weapon, const FFlareSpacecraftComponentDescription* Description);

	virtual void NotifyHit(class UPrimitiveComponent* MyComp, class AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

	virtual void Drop();

	virtual void Tick(float DeltaSeconds) override;

	virtual float GetParentDistance() const;

protected:

	/*----------------------------------------------------
		Protected data
	----------------------------------------------------*/

	/** Mesh component */
	UPROPERTY()
	UFlareBombComponent*               BombComp;

	/** Damage sound */
	UPROPERTY()
	USoundCue*                               DamageSound;

	UPROPERTY()
	UFlareWeapon*                            ParentWeapon;

	/** Special effects on explosion */
	UPROPERTY()
	UParticleSystem*                         ExplosionEffectTemplate;

	/** Burn mark decal */
	UPROPERTY()
	UMaterialInterface*                      ExplosionEffectMaterial;


	const FFlareSpacecraftComponentDescription*    WeaponDescription;

	bool										Dropped;
	bool										Activated;
	float									DropParentDistance;
};