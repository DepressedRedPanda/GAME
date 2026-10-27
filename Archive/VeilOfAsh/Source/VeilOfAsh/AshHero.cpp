#include "AshHero.h"
#include "SharedArtLibrary.h"
#include "AshVitals.h"
#include "AshInventory.h"
#include "AshProgression.h"
#include "AshWorldNode.h"
#include "AshWorldState.h"
#include "AshEnemy.h"
#include "AshStructure.h"
#include "AshCompanion.h"
#include "AshSave.h"
#include "AshGameMode.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

AAshHero::AAshHero()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(true);
    GetCapsuleComponent()->InitCapsuleSize(45.f,88.f);
    GetCharacterMovement()->MaxWalkSpeed = 430.f;
    GetCharacterMovement()->MaxWalkSpeedCrouched = 235.f;
    GetCharacterMovement()->JumpZVelocity = 590.f;
    GetCharacterMovement()->AirControl = .36f;
    GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
    bUseControllerRotationYaw = true;

    Vitals = CreateDefaultSubobject<UAshVitals>(TEXT("Vitals"));
    Inventory = CreateDefaultSubobject<UAshInventory>(TEXT("Inventory"));
    Progression = CreateDefaultSubobject<UAshProgression>(TEXT("Progression"));

    Figure = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DebugFigure"));
    Figure->SetupAttachment(GetCapsuleComponent());
    Figure->SetMobility(EComponentMobility::Movable);
    Figure->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
    Figure->SetRelativeScale3D(FVector(.65,.65,1.45));
    Figure->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    Boom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    Boom->SetupAttachment(GetRootComponent());
    Boom->TargetArmLength = 530.f;
    Boom->SocketOffset = FVector(0,0,110);
    Boom->bUsePawnControlRotation = true;
    Boom->bDoCollisionTest = true;
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    Camera->SetupAttachment(Boom);
    Camera->bUsePawnControlRotation = false;
    Lantern = CreateDefaultSubobject<UPointLightComponent>(TEXT("Lantern"));
    Lantern->SetupAttachment(GetRootComponent());
    Lantern->SetRelativeLocation(FVector(0,0,100));
    Lantern->SetIntensity(3500.f);
    Lantern->SetAttenuationRadius(550.f);
}
void AAshHero::BeginPlay()
{
    Super::BeginPlay();
	USharedArtLibrary::ApplyCharacter(this, TEXT("Player"), Figure);
    if (HasAuthority() && GetActorLocation().Z < 110.f) SetActorLocation(FVector(GetActorLocation().X,GetActorLocation().Y,170));
    if (IsLocallyControlled())
    {
        if (APlayerController* PC = Cast<APlayerController>(Controller)) PC->SetControlRotation(FRotator(-20,GetActorRotation().Yaw,0));
        LastMessage = TEXT("Seek 3 cipher seals and 2 forgotten records. Collect resources, confront the Pale Keeper.");
    }
}
void AAshHero::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const
{
    Super::GetLifetimeReplicatedProps(Out);
    DOREPLIFETIME(AAshHero,Checkpoint);
    DOREPLIFETIME(AAshHero,bGuarding);
    DOREPLIFETIME(AAshHero,bRevealed);
    DOREPLIFETIME(AAshHero,Deaths);
    DOREPLIFETIME(AAshHero,bPermanentDeath);
    DOREPLIFETIME(AAshHero,CompanionMode);
    DOREPLIFETIME(AAshHero,VehicleBoostUntil);
    DOREPLIFETIME(AAshHero,Kills);
    DOREPLIFETIME(AAshHero,Companion);
}
void AAshHero::Tick(float DT)
{
    Super::Tick(DT);
    if (!HasAuthority()) return;
    if (Vitals->bDead && !bDeathHandled) OnDeath();
    const bool bBoost = VehicleBoostUntil > GetWorld()->GetTimeSeconds();
    GetCharacterMovement()->MaxWalkSpeed = bBoost ? 1000.f : (bSprinting ? 650.f : (bRevealed ? 520.f : 430.f));
    if (bRevealed && !Vitals->SpendFocus(3.2f*DT)) bRevealed=false;
    if (bSprinting && GetVelocity().SizeSquared2D() > 2000.f && !Vitals->SpendStamina(7.f*DT)) bSprinting=false;
    // Defensive stance is intentionally costly; it isn't free immunity.
    if (bGuarding && !Vitals->SpendStamina(2.5f*DT)) bGuarding=false;
}
void AAshHero::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    Input->BindAxis("MoveForward",this,&AAshHero::MoveForward);
    Input->BindAxis("MoveRight",this,&AAshHero::MoveRight);
    Input->BindAxis("Turn",this,&AAshHero::Turn);
    Input->BindAxis("LookUp",this,&AAshHero::LookUp);
    Input->BindAction("Jump",IE_Pressed,this,&ACharacter::Jump);
    Input->BindAction("Jump",IE_Released,this,&ACharacter::StopJumping);
    Input->BindAction("Sprint",IE_Pressed,this,&AAshHero::SprintStart);
    Input->BindAction("Sprint",IE_Released,this,&AAshHero::SprintEnd);
    Input->BindAction("Crouch",IE_Pressed,this,&AAshHero::CrouchToggle);
    Input->BindAction("Attack",IE_Pressed,this,&AAshHero::Attack);
    Input->BindAction("Heavy",IE_Pressed,this,&AAshHero::Heavy);
    Input->BindAction("Parry",IE_Pressed,this,&AAshHero::Parry);
    Input->BindAction("Dodge",IE_Pressed,this,&AAshHero::Dodge);
    Input->BindAction("Guard",IE_Pressed,this,&AAshHero::GuardStart);
    Input->BindAction("Guard",IE_Released,this,&AAshHero::GuardEnd);
    Input->BindAction("Spell",IE_Pressed,this,&AAshHero::Spell);
    Input->BindAction("Interact",IE_Pressed,this,&AAshHero::Interact);
    Input->BindAction("Build",IE_Pressed,this,&AAshHero::Build);
    Input->BindAction("CraftRation",IE_Pressed,this,&AAshHero::CraftRation);
    Input->BindAction("CraftTonic",IE_Pressed,this,&AAshHero::CraftTonic);
    Input->BindAction("CraftBlade",IE_Pressed,this,&AAshHero::CraftBlade);
    Input->BindAction("UseRation",IE_Pressed,this,&AAshHero::UseRation);
    Input->BindAction("UseTonic",IE_Pressed,this,&AAshHero::UseTonic);
    Input->BindAction("Might",IE_Pressed,this,&AAshHero::UpgradeMight);
    Input->BindAction("Guile",IE_Pressed,this,&AAshHero::UpgradeGuile);
    Input->BindAction("Insight",IE_Pressed,this,&AAshHero::UpgradeInsight);
    Input->BindAction("Vanguard",IE_Pressed,this,&AAshHero::ChooseVanguard);
    Input->BindAction("Seer",IE_Pressed,this,&AAshHero::ChooseSeer);
    Input->BindAction("Shade",IE_Pressed,this,&AAshHero::ChooseShade);
    Input->BindAction("Companion",IE_Pressed,this,&AAshHero::CompanionOrder);
    Input->BindAction("EndFree",IE_Pressed,this,&AAshHero::EndingOne);
    Input->BindAction("EndChoir",IE_Pressed,this,&AAshHero::EndingTwo);
    Input->BindAction("EndDestroy",IE_Pressed,this,&AAshHero::EndingThree);
    Input->BindAction("Save",IE_Pressed,this,&AAshHero::Save);
    Input->BindAction("Load",IE_Pressed,this,&AAshHero::Load);
    Input->BindAction("Respawn",IE_Pressed,this,&AAshHero::Respawn);
    Input->BindAction("Spectral",IE_Pressed,this,&AAshHero::Spectral);
    Input->BindAction("Mantle",IE_Pressed,this,&AAshHero::Mantle);
}
void AAshHero::MoveForward(float V)
{
    if (Controller && V!=0 && !Vitals->bDead)
        AddMovementInput(FRotationMatrix(FRotator(0,Controller->GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::X),V);
}
void AAshHero::MoveRight(float V)
{
    if (Controller && V!=0 && !Vitals->bDead)
        AddMovementInput(FRotationMatrix(FRotator(0,Controller->GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::Y),V);
}
void AAshHero::Turn(float V) { AddControllerYawInput(V); }
void AAshHero::LookUp(float V) { AddControllerPitchInput(V); }
void AAshHero::SprintStart() { bSprinting=true; ServerAction(15); }
void AAshHero::SprintEnd() { bSprinting=false; ServerAction(16); }
void AAshHero::CrouchToggle() { ServerAction(14); }
void AAshHero::Attack() { ServerAction(0); }
void AAshHero::Heavy() { ServerAction(1); }
void AAshHero::Parry() { ServerAction(2); }
void AAshHero::Dodge() { ServerAction(3); }
void AAshHero::Spell() { ServerAction(4); }
void AAshHero::GuardStart() { ServerAction(5); }
void AAshHero::GuardEnd() { ServerAction(6); }
void AAshHero::CraftRation() { ServerAction(7); }
void AAshHero::CraftTonic() { ServerAction(8); }
void AAshHero::CraftBlade() { ServerAction(9); }
void AAshHero::UseRation() { ServerAction(10); }
void AAshHero::UseTonic() { ServerAction(11); }
void AAshHero::CompanionOrder() { ServerAction(12); }
void AAshHero::Respawn() { ServerAction(13); }
void AAshHero::Spectral() { ServerAction(18); }
void AAshHero::Mantle() { ServerAction(17); }
void AAshHero::Build() { ServerBuild(); }
void AAshHero::UpgradeMight() { ServerSkill(0); }
void AAshHero::UpgradeGuile() { ServerSkill(1); }
void AAshHero::UpgradeInsight() { ServerSkill(2); }
void AAshHero::ChooseVanguard() { ServerClass(0); }
void AAshHero::ChooseSeer() { ServerClass(1); }
void AAshHero::ChooseShade() { ServerClass(2); }
void AAshHero::EndingOne() { ServerEnding(1); }
void AAshHero::EndingTwo() { ServerEnding(2); }
void AAshHero::EndingThree() { ServerEnding(3); }
void AAshHero::Save() { ServerSave(false); }
void AAshHero::Load() { ServerSave(true); }
void AAshHero::Interact() { if (AAshWorldNode* N=FindInteractable()) ServerInteract(N); }

AAshWorldNode* AAshHero::FindInteractable(float Range) const
{
    AAshWorldNode* Best=nullptr; float BestScore=Range;
    for (TActorIterator<AAshWorldNode> It(GetWorld()); It; ++It)
    {
        if (!IsValid(*It)) continue;
        const float D=FVector::Dist(GetActorLocation(),It->GetActorLocation());
        if (D<BestScore) { Best=*It; BestScore=D; }
    }
    return Best;
}
FString AAshHero::NearbyName() const
{
    AAshWorldNode* N=FindInteractable();
    return N? N->Describe() : TEXT("None");
}
void AAshHero::ClientNotice_Implementation(const FString& Text)
{
    LastMessage = Text;
    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,Text);
}
void AAshHero::DeliverHit(AAshEnemy* Enemy,float Damage,bool bHeavy)
{
    if (!HasAuthority() || !Enemy) return;
    const FVector Offset = Enemy->GetActorLocation()-GetActorLocation();
    const FVector Dir=Offset.GetSafeNormal2D();
    const float Range = bHeavy?265.f:230.f;
    if (Offset.Size2D() > Range || FVector::DotProduct(Dir,GetActorForwardVector()) < .25f) return;
    const bool bBackstab=bIsCrouched && FVector::DotProduct(Enemy->GetActorForwardVector(),-Dir) < -.3f;
    const float Final=Damage*(bBackstab?2.f:1.f);
    Enemy->ReceiveStrike(Final,(bHeavy?37.f:25.f)+(bBackstab?28.f:0.f),this);
    if (bBackstab) ClientNotice(TEXT("Unseen strike! Critical damage and posture pressure."));
}
void AAshHero::ServerAction_Implementation(int32 Code)
{
    if (Code<0 || Code>18) return;
    if (Code==15) { bSprinting=true; return; }
    if (Code==16) { bSprinting=false; return; }
    const float Now=GetWorld()->GetTimeSeconds();
    if (Code==13)
    {
        if (!Vitals->bDead || bPermanentDeath) { ClientNotice(TEXT("Respawn unavailable.")); return; }
        Vitals->RestoreAtShrine(); bRevealed=false;
        SetActorLocation(Checkpoint);
        bDeathHandled=false;
        ClientNotice(TEXT("Returned to the last memory shrine."));
        return;
    }
    if (Vitals->bDead) return;
    if (Code==18)
    {
        if (bRevealed) { bRevealed=false; ClientNotice(TEXT("Spectral form ended.")); }
        else if (Vitals->SpendFocus(20.f)) { bRevealed=true; ClientNotice(TEXT("Spectral form: harder to detect; focus drains continuously.")); }
        return;
    }
    if (Code==17)
    {
        if (!Vitals->SpendStamina(14.f) || !GetCharacterMovement()->IsMovingOnGround()) return;
        const FVector Fwd=GetActorForwardVector();
        const FVector Start=GetActorLocation()+FVector(0,0,-35);
        FHitResult Wall;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(AshMantle),false,this);
        if (!GetWorld()->LineTraceSingleByChannel(Wall,Start,Start+Fwd*165.f,ECC_Visibility,Params)) return;
        const FVector Probe=GetActorLocation()+Fwd*190.f+FVector(0,0,225);
        FHitResult Top;
        if (GetWorld()->LineTraceSingleByChannel(Top,Probe,Probe-FVector(0,0,240),ECC_Visibility,Params)
            && Top.ImpactNormal.Z>.72f && Top.ImpactPoint.Z > GetActorLocation().Z-70.f)
        {
            const FVector Destination=Top.ImpactPoint+FVector(0,0,96);
            FHitResult Sweep;
            SetActorLocation(Destination,true,&Sweep);
        }
        return;
    }
    if (Code==14) { if (bIsCrouched) UnCrouch(); else Crouch(); return; }
    if (Code==5) { bGuarding=true; return; }
    if (Code==6) { bGuarding=false; return; }
    if (Code==2) { Vitals->StartParry(Now); return; }
    if (Code==3)
    {
        if (Now-LastDodge<.44f || Vitals->Stamina<21) return;
        LastDodge=Now;
        Vitals->StartDodge(Now);
        LaunchCharacter(GetActorForwardVector()*650.f + FVector(0,0,120),true,true);
        return;
    }
    if (Code==0 || Code==1)
    {
        if (Now-LastAttack < (Code==1?.85f:.32f) || !Vitals->SpendStamina(Code==1?26.f:13.f)) return;
        Combo=(Now-LastAttack<1.35f)?(Combo+1)%3:0;
        LastAttack=Now;
        for (TActorIterator<AAshEnemy> It(GetWorld()); It; ++It)
        {
            const float Base=(Code==1?39.f:19.f+Combo*4.f)+6.f*(Inventory->WeaponTier-1)+4.f*Progression->Might;
            DeliverHit(*It,Base,Code==1);
        }
        return;
    }
    if (Code==4)
    {
        if (Now-LastSpell<1.8f || !Vitals->SpendFocus(25.f)) return;
        LastSpell=Now;
        int32 Count=0;
        for(TActorIterator<AAshEnemy> It(GetWorld());It;++It)
        {
            if (FVector::Dist2D(GetActorLocation(),It->GetActorLocation())<430.f)
            { It->ReceiveStrike(20.f+8.f*Progression->Insight,45.f,this); Count++; }
        }
        ClientNotice(FString::Printf(TEXT("Void pulse struck %d enemies."),Count));
        return;
    }
    if (Code==7 || Code==8 || Code==9)
    {
        const FName Name = Code==7 ? FName(TEXT("Ration")) : (Code==8 ? FName(TEXT("Tonic")) : FName(TEXT("Blade")));
        ClientNotice(Inventory->Craft(Name) ? TEXT("Crafted / upgraded successfully.") : TEXT("Missing materials or weapon already maxed."));
        return;
    }
    if (Code==10)
    {
        if (Inventory->Consume(TEXT("Ration"),1))
        { Vitals->Hunger=FMath::Min(100.f,Vitals->Hunger+44.f); Vitals->Thirst=FMath::Min(100.f,Vitals->Thirst+12.f); ClientNotice(TEXT("Ate a ration.")); }
        else ClientNotice(TEXT("No ration in inventory."));
        return;
    }
    if (Code==11)
    {
        if (Inventory->Consume(TEXT("Tonic"),1))
        { Vitals->Heal(50.f); Vitals->Infection=FMath::Max(0.f,Vitals->Infection-25.f); ClientNotice(TEXT("Used restorative tonic.")); }
        else ClientNotice(TEXT("No tonic in inventory."));
        return;
    }
    if (Code==12)
    {
        CompanionMode=(CompanionMode+1)%3;
        ClientNotice(FString::Printf(TEXT("Companion order %d: 0 Follow, 1 Guard, 2 Hunt (abstract bonus)."),CompanionMode));
    }
}
void AAshHero::ServerInteract_Implementation(AActor* Target)
{
    if (Vitals->bDead || !IsValid(Target) || FVector::Dist(GetActorLocation(),Target->GetActorLocation())>345.f) return;
    if (AAshWorldNode* Node=Cast<AAshWorldNode>(Target)) Node->Interact(this);
}
void AAshHero::ServerBuild_Implementation()
{
    if (Vitals->bDead) return;
    if (Inventory->Count(TEXT("Wood"))<5 || Inventory->Count(TEXT("Ore"))<2)
    { ClientNotice(TEXT("Build shelter: requires 5 Wood and 2 Ore.")); return; }
    Inventory->Consume(TEXT("Wood"),5); Inventory->Consume(TEXT("Ore"),2);
    const FVector Spot = GetActorLocation() + GetActorForwardVector()*420.f + FVector(0,0,50);
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    if (GetWorld()->SpawnActor<AAshStructure>(AAshStructure::StaticClass(),Spot,GetActorRotation(),Params))
    {
        if (AAshWorldState* S=GetWorld()->GetGameState<AAshWorldState>()) S->BuiltCount++;
        SetCheckpoint(GetActorLocation()+FVector(0,0,100));
        ClientNotice(TEXT("Shelter built. Checkpoint updated."));
    }
}
void AAshHero::ServerSkill_Implementation(int32 Branch)
{
    if (Branch<0 || Branch>2 || Vitals->bDead) return;
    if (Progression->BuySkill(Branch))
    {
        if (Branch==0) { Vitals->MaxHealth+=12.f; Vitals->Heal(12.f); }
        ClientNotice(TEXT("Skill improved! Might/Guile/Insight alter combat and progression."));
    }
    else ClientNotice(TEXT("Earn XP by defeating enemies or recovering lore; level up for skill points."));
}
void AAshHero::ServerClass_Implementation(int32 ClassIndex)
{
    if (ClassIndex<0 || ClassIndex>2 || Vitals->bDead) return;
    if (Progression->Level>1) { ClientNotice(TEXT("Class selection locked after level 1.")); return; }
    Progression->ChooseClass(static_cast<EAshClass>(ClassIndex));
    if (ClassIndex==0) Vitals->MaxHealth=120.f;
    else if (ClassIndex==1) { Vitals->MaxHealth=90.f; Vitals->Focus=100.f; }
    else Vitals->MaxHealth=95.f;
    Vitals->Health=Vitals->MaxHealth;
    ClientNotice(ClassIndex==0?TEXT("Vanguard: higher health."):(ClassIndex==1?TEXT("Seer: more focus."):TEXT("Shade: agility and stealth bonuses.")));
}
void AAshHero::ServerEnding_Implementation(int32 Choice)
{
    AAshWorldState* W=GetWorld()->GetGameState<AAshWorldState>();
    if (!W || !W->bBossDefeated) { ClientNotice(TEXT("The Pale Keeper still stands. Endings are locked.")); return; }
    if (Choice<1 || Choice>3 || W->Ending) return;
    if (Choice==1 && Progression->FreefolkReputation<1) { ClientNotice(TEXT("Earn Freefolk trust by speaking with their envoy.")); return; }
    if (Choice==2 && Progression->ChoirReputation<1) { ClientNotice(TEXT("Earn Choir trust by speaking with their envoy.")); return; }
    W->Ending=Choice;
    const FString Text=Choice==1?TEXT("ENDING: The Freefolk reclaim their memories."):
        (Choice==2?TEXT("ENDING: The Choir restores the network at a terrible cost."):
        TEXT("ENDING: The engine dies. The age of gods ends with it."));
    ClientNotice(Text+FString::Printf(TEXT(" Freefolk %d, Choir %d."),Progression->FreefolkReputation,Progression->ChoirReputation));
}
void AAshHero::ServerSave_Implementation(bool bLoad)
{
    // Avoid writing a remote client's campaign to the host's disk.
    if (GetNetMode()!=NM_Standalone) { ClientNotice(TEXT("Save/load in this demo is single-player only.")); return; }
    const FString Slot=TEXT("VeilOfAsh_Save1");
    if (bLoad)
    {
        UAshSave* SaveObj=Cast<UAshSave>(UGameplayStatics::LoadGameFromSlot(Slot,0));
        if (!SaveObj) { ClientNotice(TEXT("No save file found.")); return; }
        Checkpoint=SaveObj->Checkpoint;
        SetActorLocation(SaveObj->Position);
        Inventory->Items=SaveObj->Items; Inventory->WeaponTier=SaveObj->WeaponTier;
        Progression->Level=SaveObj->Level; Progression->XP=SaveObj->XP; Progression->SkillPoints=SaveObj->SkillPoints;
        Progression->Might=SaveObj->Might; Progression->Guile=SaveObj->Guile; Progression->Insight=SaveObj->Insight;
        Progression->FreefolkReputation=SaveObj->FreefolkReputation; Progression->ChoirReputation=SaveObj->ChoirReputation;
        Progression->HeroClass=static_cast<EAshClass>(FMath::Clamp(SaveObj->HeroClass,0,2));
        Vitals->MaxHealth=100.f+Progression->Might*12.f;
        Vitals->Health=FMath::Clamp(SaveObj->Health,1.f,Vitals->MaxHealth);
        Vitals->Hunger=SaveObj->Hunger; Vitals->Thirst=SaveObj->Thirst; Vitals->Infection=SaveObj->Infection;
        Vitals->bDead=false; bDeathHandled=false; Kills=SaveObj->Kills;
        ClientNotice(TEXT("Character restored. World nodes/enemy kills reset on new sessions."));
    }
    else
    {
        UAshSave* SaveObj=Cast<UAshSave>(UGameplayStatics::CreateSaveGameObject(UAshSave::StaticClass()));
        if (!SaveObj) return;
        SaveObj->Position=GetActorLocation(); SaveObj->Checkpoint=Checkpoint;
        SaveObj->Items=Inventory->Items; SaveObj->WeaponTier=Inventory->WeaponTier;
        SaveObj->Level=Progression->Level; SaveObj->XP=Progression->XP; SaveObj->SkillPoints=Progression->SkillPoints;
        SaveObj->Might=Progression->Might; SaveObj->Guile=Progression->Guile; SaveObj->Insight=Progression->Insight;
        SaveObj->FreefolkReputation=Progression->FreefolkReputation; SaveObj->ChoirReputation=Progression->ChoirReputation;
        SaveObj->HeroClass=static_cast<int32>(Progression->HeroClass);
        SaveObj->Health=Vitals->Health; SaveObj->Hunger=Vitals->Hunger; SaveObj->Thirst=Vitals->Thirst;
        SaveObj->Infection=Vitals->Infection; SaveObj->Kills=Kills;
        ClientNotice(UGameplayStatics::SaveGameToSlot(SaveObj,Slot,0)?TEXT("Character saved locally (F9 to load)."):
            TEXT("Saving failed."));
    }
}
void AAshHero::AwardKill(int32 Experience)
{
    if (!HasAuthority()) return;
    Kills++;
    Progression->GainXP(Experience);
    ClientNotice(FString::Printf(TEXT("Enemy defeated. +%d XP. Total kills: %d"),Experience,Kills));
}
void AAshHero::SetCheckpoint(const FVector& Place)
{
    if (HasAuthority()) Checkpoint=Place;
}
void AAshHero::OnDeath()
{
    if (!HasAuthority() || bDeathHandled) return;
    bDeathHandled=true; bGuarding=false; bRevealed=false; Deaths++;
    if (AAshGameMode* Mode=Cast<AAshGameMode>(UGameplayStatics::GetGameMode(GetWorld()))) bPermanentDeath=Mode->bIronSoulPermadeath;
    ClientNotice(bPermanentDeath?TEXT("IRON SOUL: Your run is over."):TEXT("YOU DIED. Press R to resurrect at your checkpoint."));
}
