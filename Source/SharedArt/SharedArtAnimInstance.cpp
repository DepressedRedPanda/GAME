#include "SharedArtAnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void USharedArtAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);
    const ACharacter* Character = Cast<ACharacter>(TryGetPawnOwner());
    if (!Character)
    {
        Speed = Direction = 0.f;
        bFalling = bCrouched = false;
        return;
    }
    const FVector Velocity = Character->GetVelocity();
    Speed = Velocity.Size2D();
    const FVector LocalVelocity = Character->GetActorRotation().UnrotateVector(Velocity);
    Direction = Speed > KINDA_SMALL_NUMBER ? FMath::RadiansToDegrees(FMath::Atan2(LocalVelocity.Y, LocalVelocity.X)) : 0.f;
    bFalling = Character->GetCharacterMovement()->IsFalling();
    bCrouched = Character->bIsCrouched;
}
