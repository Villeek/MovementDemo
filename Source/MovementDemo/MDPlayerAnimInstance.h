// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MDPlayerAnimInstance.generated.h"

class AMDCharacter;
class UMDCharacterMovementComponent;

/**
 * Threaded animation updates for MDCharacter.
 * AnimBP Event Graph is empty. Only Anim Graph has nodes.
 * All IK's and procedural animation is implemented inside Control Rig named CR_Player, which can be found in Blueprints/Player/ folder.
 */
UCLASS()
class MOVEMENTDEMO_API UMDPlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:

	UMDPlayerAnimInstance();

	virtual void NativeInitializeAnimation() override;

	// Native update override point. It is usually a good idea to simply gather data in this step and 
	// for the bulk of the work to be done in NativeThreadSafeUpdateAnimation.
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// Native thread safe update override point. Executed on a worker thread just prior to graph update 
	// for linked anim instances, only called when the hosting node(s) are relevant.
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

protected:

	UPROPERTY()
	AMDCharacter* MyPawn;

	UPROPERTY()
	UMDCharacterMovementComponent* MoveComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Booleans")
	uint32 bIsFalling:1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Booleans")
	uint32 bIsMoving:1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Booleans")
	uint32 bIsJumping:1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Booleans")
	uint32 bIsSliding:1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Booleans")
	uint32 bIsMantling:1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Booleans")
	uint32 bIsWallRunning:1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Booleans")
	uint32 bIsCrouching:1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	FVector Velocity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float Speed;

	// Landing looks horrible if sliding + landing. We can use this to control the alpha of the additive animation.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float LandingAdditiveAlpha;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rotation")
	FRotator AimRotation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rotation")
	FRotator PawnRotation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rotation")
	float AimPitch;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rotation")
	float AimYaw;
};
