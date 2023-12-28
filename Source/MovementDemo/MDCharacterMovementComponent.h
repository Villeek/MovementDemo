// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MDCharacterMovementComponent.generated.h"


//Result of trying to find if the character can mantle (climb up ledge).
struct FMantleInfo
{
	FMantleInfo() {}

	FVector StartLocation = FVector::ZeroVector;

	FVector EndLocation = FVector::ZeroVector;

	bool bCanMantle = false;
};

UENUM(BlueprintType)
enum EMDCustomMovementMode
{
	MDMOVE_None			UMETA(Hidden),
	MDMOVE_WallRun		UMETA(DisplayName = "Wall Run"),
	MDMOVE_Rooted		UMETA(DisplayName = "Rooted"),
};

/**
 * MDCharacterMovementComponent has functionality for sliding, wall running and mantling.
 * This component itself is not replicated, instead it routes replication trough owning character, just like original UCharacterMovementComponent.
 * Sliding is not it's own movement mode. Instead, it uses PhysWalking but calculates velocity and acceleration differently. This way we can use all collision checks etc from PhysWalking.
 * Wall running uses custom movement mode and tries to keep offset to a wall.
 * Mantling is implemented using root motion source.
 * Character can be rooted in place by using Rooted custom movement mode.
 */
UCLASS()
class MOVEMENTDEMO_API UMDCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:

	UMDCharacterMovementComponent();

	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void PhysicsRotation(float DeltaTime) override;

	virtual FRotator ComputeOrientToMovementRotation(const FRotator& CurrentRotation, float DeltaTime, FRotator& DeltaRotation) const override;

	virtual void StartFalling(int32 Iterations, float remainingTime, float timeTick, const FVector& Delta, const FVector& subLoc) override;

	virtual void OnRootMotionSourceBeingApplied(const FRootMotionSource* Source) override;

	virtual void OnTeleported() override;

	virtual bool CanCrouchInCurrentState() const override;

	// Player controlling wants to sprint. Doesn't mean we can or will do it.
	void SetWantsToSprint(bool bNewValue);

	void SetWantsToSlide(bool bNewValue);

	virtual float GetMaxSpeed() const override;

	virtual float GetMaxAcceleration() const override;

	virtual float GetMaxBrakingDeceleration() const override;

	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;

	bool IsWallRunning() const;

	bool CanStartMantle() const;

	FMantleInfo TryFindMantleLocation() const;

	void DoMantle(const FMantleInfo& MantleInfo);

	UPROPERTY(Category = "Character Movement: Walking", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0", ForceUnits = "cm/s"))
	float MaxSprintSpeed;

	UPROPERTY(Category = "Character Movement: Sliding", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0", ForceUnits = "cm/s"))
	float MaxSlideSpeed;

	UPROPERTY(Category = "Character Movement: Sliding", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0", ForceUnits = "cm/s"))
	float MaxSlideAcceleration;

	// We scale acceleration when sliding and this is the angle, when we reach max scale
	UPROPERTY(Category = "Character Movement: Sliding", EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "90.0", UIMin = "0.0", UIMax = "90.0", ForceUnits = "degrees"))
	float SlidingSlopeAngleToReachMaxAcceleration;

	// 90 degrees means we start to slow down when going perpendicular to down slope
	UPROPERTY(Category = "Character Movement: Sliding", EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "90.0", UIMin = "0.0", UIMax = "90.0", ForceUnits = "degrees"))
	float SlidingMaxAngleToDownSlopeBeforeSlowingDown;

	UPROPERTY(Category = "Character Movement: Sliding", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0", ForceUnits = "cm/s"))
	float SlideStartBoost;

	UPROPERTY(Category = "Character Movement: Sliding", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0", ForceUnits = "cm/s"))
	float SlideEnterRequiredSpeed;

	UPROPERTY(Category = "Character Movement: Sliding", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0", ForceUnits = "cm/s"))
	float SlideMinSpeed;

	UPROPERTY(Category = "Character Movement: Sliding", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float SlidingFrictionMulti;

	UPROPERTY(Category = "Character Movement: Sliding", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float BrakingDecelerationSliding;

	UPROPERTY(Category = "Character Movement: Sliding", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "1.0"))
	float SlideTurnStrength;

	UPROPERTY(Category = "Character Movement: Wall Running", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float DesiredDistanceToWallWhenWallRunning;

	UPROPERTY(Category = "Character Movement: Wall Running", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float DesiredDistanceMaintainSpeed;

	UPROPERTY(Category = "Character Movement: Wall Running", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float MinSpeedToKeepWallRunning;

	UPROPERTY(Category = "Character Movement: Wall Running", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float MinDistanceToFloor;

	UPROPERTY(Category = "Character Movement: Wall Running", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float MaxWallRunSpeed;

	UPROPERTY(Category = "Character Movement: Wall Running", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float DownwardPullForce;

	// Should be larger than capsule radius
	UPROPERTY(Category = "Character Movement: Wall Running", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	double MaxDistanceToTraceForWall;

	UPROPERTY(Category = "Character Movement: Mantle", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	double MinHeightFromFloor_Mantle;

	UPROPERTY(Category = "Character Movement: Mantle", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	double MaxHeightFromFloor_Mantle;

	UPROPERTY(Category = "Character Movement: Mantle", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	double ForwardTraceLength_Mantle;

	UPROPERTY(Category = "Character Movement: Mantle", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "10", UIMin = "100"))
	int32 MaxIterations_Mantle;

	UPROPERTY(Category = "Character Movement: Mantle", EditAnywhere, BlueprintReadWrite)
	TArray<TEnumAsByte<EObjectTypeQuery>> MantleTraceObjectTypes;

protected:

	// Stored in FLAG_Custom_0
	uint32 bWantsToSprint:1;

	// Stored in FLAG_Custom_1
	uint32 bWantsToSlide:1;

	// Is actually sprinting and not just wanting to sprint?
	uint32 bIsSprinting:1;

	uint32 bIsSliding:1;

	uint32 bWantsToJumpOffWall:1;

	uint32 bWasSlidingBeforeFalling:1;

	bool CanStartSprinting() const;

	void StartSprinting();

	void StopSprinting();

	bool CanStartSliding() const;

	bool CanContinueSliding() const;

	void StartSliding();

	void StopSliding();

	bool FindWallForWallRunning(FHitResult& OutHit) const;

	virtual void PhysCustom(float DeltaTime, int32 Iterations) override;

	void PhysWallRun(float DeltaTime, int32 Iterations);

	void PhysRooted(float DeltaTime, int32 Iterations);

	void JumpOffTheWall(const FVector& WallJumpVelocity);

	virtual void SetPostLandedPhysics(const FHitResult& Hit) override;

	bool ShouldSlideOnLanded(const FHitResult& Hit) const;

	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;

	virtual void UpdateCharacterStateAfterMovement(float DeltaSeconds) override;

	virtual void UpdateFromCompressedFlags(uint8 Flags) override;

	virtual bool ClientUpdatePositionAfterServerUpdate() override;

	virtual void MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags, const FVector& NewAccel) override;

	friend class FMDSavedMove;
};


// Classes needed for network and prediction
class FMDSavedMove : public FSavedMove_Character
{
	typedef FSavedMove_Character Super;

public:

	// Resets all saved variables.
	virtual void Clear() override;

	// Store input commands in the compressed flags.
	virtual uint8 GetCompressedFlags() const override;

	// Sets up the move before sending it to the server. 
	virtual void SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override;

	// Sets variables on character movement component before making a predictive correction.
	virtual void PrepMoveFor(ACharacter* Character) override;

	uint32 bWantsToSprint:1;

	uint32 bWantsToSlide:1;
};

class FMDNetworkPredictionData_Client : public FNetworkPredictionData_Client_Character
{
	typedef FNetworkPredictionData_Client_Character Super;

public:

	FMDNetworkPredictionData_Client(const UCharacterMovementComponent& ClientMovement);

	virtual FSavedMovePtr AllocateNewMove() override;
};
