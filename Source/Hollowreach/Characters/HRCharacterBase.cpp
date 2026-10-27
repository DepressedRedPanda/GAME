#include "Characters/HRCharacterBase.h"
#include "SharedArtLibrary.h"

#include "Components/CapsuleComponent.h"
#include "Components/HRAbilityComponent.h"
#include "Components/HRAttributeComponent.h"
#include "Components/HRCombatComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/HRGreybox.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

AHRCharacterBase::AHRCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

	Attributes = CreateDefaultSubobject<UHRAttributeComponent>(TEXT("Attributes"));
	Combat = CreateDefaultSubobject<UHRCombatComponent>(TEXT("Combat"));
	Abilities = CreateDefaultSubobject<UHRAbilityComponent>(TEXT("Abilities"));

	GetCapsuleComponent()->InitCapsuleSize(38.f, 90.f);

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.f, 720.f, 0.f);
	Movement->JumpZVelocity = 620.f;
	Movement->AirControl = 0.35f;
	Movement->MaxWalkSpeed = BaseWalkSpeed;
	Movement->BrakingDecelerationWalking = 2000.f;
	Movement->GetNavAgentPropertiesRef().bCanCrouch = true;
	bUseControllerRotationYaw = false;

	// Greybox body.
	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(GetCapsuleComponent());

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(VisualRoot);
	BodyMesh->SetStaticMesh(HRGreybox::Cylinder());
	BodyMesh->SetRelativeScale3D(FVector(0.7f, 0.55f, 1.45f));
	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, -15.f));
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetCanEverAffectNavigation(false);

	HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMesh"));
	HeadMesh->SetupAttachment(VisualRoot);
	HeadMesh->SetStaticMesh(HRGreybox::Sphere());
	HeadMesh->SetRelativeScale3D(FVector(0.42f));
	HeadMesh->SetRelativeLocation(FVector(6.f, 0.f, 72.f));
	HeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeadMesh->SetCanEverAffectNavigation(false);

	WeaponPivot = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponPivot"));
	WeaponPivot->SetupAttachment(VisualRoot);
	WeaponPivot->SetRelativeLocation(FVector(10.f, 38.f, 20.f));

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(WeaponPivot);
	WeaponMesh->SetStaticMesh(HRGreybox::Cube());
	WeaponMesh->SetRelativeLocation(FVector(55.f, 0.f, 0.f));
	WeaponMesh->SetRelativeScale3D(FVector(1.1f, 0.06f, 0.12f));
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetCanEverAffectNavigation(false);
}

void AHRCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	Attributes->OnHealthDepleted.AddDynamic(this, &AHRCharacterBase::HandleHealthDepleted);
	Combat->OnStateChanged.AddDynamic(this, &AHRCharacterBase::HandleCombatStateChanged);

	if (USharedArtLibrary::ApplyCharacter(this, SharedAppearanceRole, VisualRoot))
	{
		bUseGreyboxVisuals = false;
		USharedArtLibrary::SetCharacterScale(this, SharedAppearanceRole, VisualRoot->GetRelativeScale3D().X);
	}
	SetGreyboxVisible(bUseGreyboxVisuals);
	RefreshWeaponAppearance(NAME_None);
	if (bUseGreyboxVisuals)
	{
		BodyMaterial = HRGreybox::Tint(BodyMesh, BodyColor);
		HRGreybox::Tint(HeadMesh, BodyColor * 1.15f);
		HRGreybox::Tint(WeaponMesh, FLinearColor(0.75f, 0.75f, 0.8f));
	}
}

void AHRCharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	USharedArtLibrary::UpdateCombat(this, FName(*StaticEnum<EHRCombatState>()->GetNameStringByValue(static_cast<int64>(Combat->GetState()))), Combat->GetComboIndex(), Combat->GetAttackProgress());

	if (GetCharacterMovement()->IsFalling())
	{
		LastFallingSpeed = FMath::Max(LastFallingSpeed, -GetVelocity().Z);
	}

	if (!bIsDead)
	{
		UpdateMovementSpeed();
	}
	if (bUseGreyboxVisuals)
	{
		UpdateGreyboxAnimation(DeltaSeconds);
	}
}

void AHRCharacterBase::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	const float ImpactSpeed = LastFallingSpeed;
	LastFallingSpeed = 0.f;
	if (ImpactSpeed > FallDamageThreshold && !bIsDead)
	{
		FHRDamageInfo Damage;
		Damage.Type = EHRDamageType::Fall;
		Damage.Amount = (ImpactSpeed - FallDamageThreshold) * 0.16f;
		Damage.Source = nullptr;
		Attributes->ApplyDamage(Damage);
	}
}

float AHRCharacterBase::GetMovementMultiplier() const
{
	float Multiplier = Attributes->GetStat(EHRStat::MoveSpeed) * MovementSpeedScale;
	switch (Combat->GetState())
	{
	case EHRCombatState::Attacking: Multiplier *= 0.25f; break;
	case EHRCombatState::Blocking: Multiplier *= 0.5f; break;
	case EHRCombatState::Busy: Multiplier *= 0.4f; break;
	case EHRCombatState::Staggered:
	case EHRCombatState::PostureBroken:
	case EHRCombatState::Downed:
	case EHRCombatState::Dead: Multiplier = 0.f; break;
	default: break;
	}
	return Multiplier;
}

void AHRCharacterBase::UpdateMovementSpeed()
{
	float Speed = BaseWalkSpeed * GetMovementMultiplier();
	if (WantsToSprint() && Combat->GetState() == EHRCombatState::Idle)
	{
		Speed *= SprintMultiplier;
	}
	GetCharacterMovement()->MaxWalkSpeed = Speed;
	GetCharacterMovement()->MaxWalkSpeedCrouched = Speed * 0.5f;
}

void AHRCharacterBase::UpdateGreyboxAnimation(float DeltaSeconds)
{
	// Procedural "animation" of the greybox blade from the combat state.
	FRotator Target(-35.f, 0.f, 0.f);
	switch (Combat->GetState())
	{
	case EHRCombatState::Attacking:
	{
		const float Progress = Combat->GetAttackProgress();
		const float Swing = FMath::SmoothStep(0.2f, 0.65f, Progress);
		if (Combat->IsHeavyAttack())
		{
			Target = FRotator(FMath::Lerp(75.f, -40.f, Swing), 0.f, 0.f);
		}
		else
		{
			const float Direction = (Combat->GetComboIndex() % 2 == 0) ? 1.f : -1.f;
			Target = FRotator(-5.f, FMath::Lerp(-85.f, 85.f, Swing) * Direction, 0.f);
		}
		break;
	}
	case EHRCombatState::Blocking: Target = FRotator(15.f, -70.f, 80.f); break;
	case EHRCombatState::Staggered: Target = FRotator(40.f, 40.f, 0.f); break;
	case EHRCombatState::PostureBroken: Target = FRotator(-80.f, 30.f, 0.f); break;
	default: break;
	}

	const float Speed = Combat->GetState() == EHRCombatState::Attacking ? 40.f : 12.f;
	WeaponRotation = FMath::RInterpTo(WeaponRotation, Target, DeltaSeconds, Speed);
	WeaponPivot->SetRelativeRotation(WeaponRotation);

	// Slump the body when posture is broken or downed.
	const bool bSlumped = Combat->GetState() == EHRCombatState::PostureBroken || Combat->GetState() == EHRCombatState::Downed;
	const FRotator BodyTarget = bIsDead ? FRotator(0.f, 0.f, 85.f) : (bSlumped ? FRotator(-25.f, 0.f, 0.f) : FRotator::ZeroRotator);
	VisualRoot->SetRelativeRotation(FMath::RInterpTo(VisualRoot->GetRelativeRotation(), BodyTarget, DeltaSeconds, 6.f));
}

void AHRCharacterBase::HandleHealthDepleted(UHRAttributeComponent* InAttributes, AActor* Killer)
{
	Die(Killer);
}

void AHRCharacterBase::HandleCombatStateChanged(EHRCombatState OldState, EHRCombatState NewState)
{
	if (!BodyMaterial)
	{
		return;
	}
	// Flash posture breaks so the deathblow opportunity is readable.
	if (NewState == EHRCombatState::PostureBroken)
	{
		BodyMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, 0.85f, 0.2f));
	}
	else if (OldState == EHRCombatState::PostureBroken)
	{
		BodyMaterial->SetVectorParameterValue(TEXT("Color"), BodyColor);
	}
}

void AHRCharacterBase::Die(AActor* Killer)
{
	if (bIsDead)
	{
		return;
	}
	bIsDead = true;
	Combat->SetDead();
	if (Abilities)
	{
		Abilities->DeactivateAll();
	}
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	K2_OnDeath(Killer);
}

void AHRCharacterBase::ReviveCharacter(float HealthFraction)
{
	bIsDead = false;
	Attributes->RefillAll();
	Attributes->SetHealthFraction(HealthFraction);
	Combat->ResetCombatState();
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	VisualRoot->SetRelativeRotation(FRotator::ZeroRotator);
	ResetVisualColor();
}

void AHRCharacterBase::SetVisualColor(FLinearColor Color)
{
	if (BodyMaterial)
	{
		BodyMaterial->SetVectorParameterValue(TEXT("Color"), Color);
	}
}

void AHRCharacterBase::ResetVisualColor()
{
	SetVisualColor(BodyColor);
}

void AHRCharacterBase::SetVisualScale(float Scale)
{
	VisualRoot->SetRelativeScale3D(FVector(Scale));
	USharedArtLibrary::SetCharacterScale(this, SharedAppearanceRole, Scale);
}

void AHRCharacterBase::SetGreyboxVisible(bool bVisible)
{
	VisualRoot->SetVisibility(bVisible, true);
	if (GetMesh())
	{
		GetMesh()->SetVisibility(!bVisible || GetMesh()->GetSkeletalMeshAsset() != nullptr);
	}
}

void AHRCharacterBase::RefreshWeaponAppearance(FName ItemId)
{
	if (!bUseGreyboxVisuals)
	{
		WeaponMesh->SetVisibility(USharedArtLibrary::ApplyWeapon(this, SharedAppearanceRole, WeaponMesh, ItemId));
	}
}
