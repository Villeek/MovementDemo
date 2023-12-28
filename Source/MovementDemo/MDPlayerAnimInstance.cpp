// Fill out your copyright notice in the Description page of Project Settings.


#include "MovementDemo/MDPlayerAnimInstance.h"
#include "MovementDemo/MDCharacter.h"
#include "MovementDemo/MDCharacterMovementComponent.h"

UMDPlayerAnimInstance::UMDPlayerAnimInstance()
{
	bUseMultiThreadedAnimationUpdate = true;
}

void UMDPlayerAnimInstance::NativeInitializeAnimation()
{
	if(auto* MDCharacter = Cast<AMDCharacter>(GetOwningActor()))
	{
		MyPawn = MDCharacter;
		MoveComp = CastChecked<UMDCharacterMovementComponent>(MyPawn->GetCharacterMovement());
	}
}

void UMDPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	if(!IsValid(MyPawn))
	{
		NativeInitializeAnimation();
		return;
	}

	// Gather data in game thread
	Velocity = MoveComp->GetLastUpdateVelocity();
	bIsFalling = MoveComp->IsFalling();
	bIsWallRunning = MoveComp->IsWallRunning();
	bIsCrouching = MoveComp->IsCrouching();
	bIsSliding = MyPawn->IsSliding();
	bIsMantling = MyPawn->IsMantling();
	AimRotation = MyPawn->GetBaseAimRotation();
	PawnRotation = MyPawn->GetActorRotation();
}

void UMDPlayerAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	// Process data in worker thread
	Speed = Velocity.Size();
	bIsMoving = Speed >= 5.0;
	bIsJumping = (bIsFalling == true) && Velocity.Z > 100.0;
	LandingAdditiveAlpha = bIsSliding ? 0.0f : 1.0f;
	// Find Aim pitch and yaw relative to where the pawn is facing
	FRotator DeltaRot = AimRotation - PawnRotation;
	DeltaRot.Normalize();
	AimPitch = DeltaRot.Pitch;
	AimYaw = DeltaRot.Yaw;
}
