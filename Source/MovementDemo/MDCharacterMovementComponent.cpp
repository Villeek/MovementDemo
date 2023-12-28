// Fill out your copyright notice in the Description page of Project Settings.


#include "MovementDemo/MDCharacterMovementComponent.h"
#include "MovementDemo/MDCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Logging/StructuredLog.h"

static const FName RootMotionName_Mantle = "Mantle";

UMDCharacterMovementComponent::UMDCharacterMovementComponent()
{
	bOrientRotationToMovement = true;
	RotationRate = FRotator(0.0, 1000.0, 0.0);
	GetNavAgentPropertiesRef().bCanCrouch = true;
	MaxSimulationIterations = 4;
	MaxSimulationTimeStep = 0.05f;
	NetworkSmoothingMode = ENetworkSmoothingMode::Exponential;
	bCanWalkOffLedges = true;
	bCanWalkOffLedgesWhenCrouching = true;
	MaxWalkSpeed = 300.0f;
	MaxSprintSpeed = 1000.0f;
	MaxSlideSpeed = 2000.0f;
	MaxAcceleration = 3000.0f;
	MaxSlideAcceleration = 4200.0f;
	SlidingSlopeAngleToReachMaxAcceleration = 35.0f;

	JumpZVelocity = 450.0f;

	SlideStartBoost = 300.0f;
	SlideEnterRequiredSpeed = 500.0f;
	SlideMinSpeed = 300.0f;
	SlidingFrictionMulti = 0.25f;
	SlideTurnStrength = 1.0f;
	SlidingMaxAngleToDownSlopeBeforeSlowingDown = 70.0f;

	BrakingDecelerationSliding = 1024.0f;

	DesiredDistanceToWallWhenWallRunning = 15.0f;
	DesiredDistanceMaintainSpeed = 3.0f;
	MinSpeedToKeepWallRunning = 300.0f;
	MinDistanceToFloor = 50.0f;
	MaxWallRunSpeed = 800.0f;
	DownwardPullForce = 25.0f;
	MaxDistanceToTraceForWall = 100.0;

	bWantsToSprint = false;
	bWantsToSlide = false;
	bIsSprinting = false;
	bIsSliding = false;
	bWantsToJumpOffWall = false;
	bWasSlidingBeforeFalling = false;

	MinHeightFromFloor_Mantle = 80.0;
	MaxHeightFromFloor_Mantle = 250.0;
	ForwardTraceLength_Mantle = 50.0;
	MaxIterations_Mantle = 30;
}

FNetworkPredictionData_Client* UMDCharacterMovementComponent::GetPredictionData_Client() const
{
	if(!ClientPredictionData)
	{
		auto* MutableThis = const_cast<UMDCharacterMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FMDNetworkPredictionData_Client(*this);
	}
	return ClientPredictionData;
}

void UMDCharacterMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!HasValidData() || ShouldSkipUpdate(DeltaTime))
	{
		return;
	}

	// Can easily add debug logging here if needed
}

void UMDCharacterMovementComponent::PhysicsRotation(float DeltaTime)
{
	if (MovementMode == MOVE_Custom && CustomMovementMode == MDMOVE_Rooted)
	{
		// We are going to hard code our rotation to face where the general flow of the level is going, which will be world -X (for some reason)
		// Real game probably wants to pull this from somewhere else, like from the first checkpoint
		FRotator Rotation = (-FVector::ForwardVector).Rotation();
		MoveUpdatedComponent(FVector::ZeroVector, Rotation,false);
		return;
	}

	Super::PhysicsRotation(DeltaTime);
}

FRotator UMDCharacterMovementComponent::ComputeOrientToMovementRotation(const FRotator& CurrentRotation, float DeltaTime, FRotator& DeltaRotation) const
{
	// Use velocity direction as rotation when sliding
	if(bIsSliding && Velocity.SizeSquared() > UE_KINDA_SMALL_NUMBER)
	{
		return Velocity.GetSafeNormal().Rotation();
	}

	if (Acceleration.SizeSquared() < UE_KINDA_SMALL_NUMBER)
	{
		// AI path following request can orient us in that direction (it's effectively an acceleration)
		if (bHasRequestedVelocity && RequestedVelocity.SizeSquared() > UE_KINDA_SMALL_NUMBER)
		{
			return RequestedVelocity.GetSafeNormal().Rotation();
		}

		// Don't change rotation if there is no acceleration.
		return CurrentRotation;
	}

	// Rotate toward direction of acceleration.
	return Acceleration.GetSafeNormal().Rotation();
}

void UMDCharacterMovementComponent::StartFalling(int32 Iterations, float remainingTime, float timeTick, const FVector& Delta, const FVector& subLoc)
{
	bWasSlidingBeforeFalling = bIsSliding;

	Super::StartFalling(Iterations, remainingTime, timeTick, Delta, subLoc);
}

void UMDCharacterMovementComponent::OnRootMotionSourceBeingApplied(const FRootMotionSource* Source)
{
	if(Source->InstanceName == RootMotionName_Mantle)
	{
		CastChecked<AMDCharacter>(GetCharacterOwner())->SetIsMantling(true);
	}
}

void UMDCharacterMovementComponent::OnTeleported()
{
	SetMovementMode(MOVE_Falling);
	Velocity = FVector::ZeroVector;
	bWantsToSlide = false;

	Super::OnTeleported();
}

bool UMDCharacterMovementComponent::CanCrouchInCurrentState() const
{
	if (!CanEverCrouch())
	{
		return false;
	}

	return (IsMovingOnGround()) && UpdatedComponent && !UpdatedComponent->IsSimulatingPhysics();
}

void UMDCharacterMovementComponent::SetWantsToSprint(bool bNewValue)
{
	bWantsToSprint = bNewValue;
}

void UMDCharacterMovementComponent::SetWantsToSlide(bool bNewValue)
{
	bWantsToSlide = bNewValue;
}

float UMDCharacterMovementComponent::GetMaxSpeed() const
{
	switch (MovementMode)
	{
		case MOVE_Walking:
			case MOVE_NavWalking:
			return bIsSliding ? MaxSlideSpeed : IsCrouching() ? MaxWalkSpeedCrouched : bIsSprinting ? MaxSprintSpeed : MaxWalkSpeed;
		case MOVE_Falling:
			/*
			 * We need to return MaxSlideSpeed on the frame we slide off the edge (we are sliding and falling for one frame),
			 * Otherwise our Velocity gets clamped to MaxWalkSpeed(much smaller than slide speed) and we lose all momentum.
			 */
			return bIsSliding ? MaxSlideSpeed : MaxWalkSpeed;
		case MOVE_Swimming:
			return MaxSwimSpeed;
		case MOVE_Flying:
			return MaxFlySpeed;
		case MOVE_Custom:
			return IsWallRunning() ? MaxWallRunSpeed : MaxCustomMovementSpeed;
		case MOVE_None:
		default:
			return 0.f;
	}
}

float UMDCharacterMovementComponent::GetMaxAcceleration() const
{
	return bIsSliding ? MaxSlideAcceleration : Super::GetMaxAcceleration();
}

float UMDCharacterMovementComponent::GetMaxBrakingDeceleration() const
{
	switch (MovementMode)
	{
		case MOVE_Walking:
		case MOVE_NavWalking:
			return bIsSliding ? BrakingDecelerationSliding : BrakingDecelerationWalking;
		case MOVE_Falling:
			return BrakingDecelerationFalling;
		case MOVE_Swimming:
			return BrakingDecelerationSwimming;
		case MOVE_Flying:
			return BrakingDecelerationFlying;
		case MOVE_Custom:
			return 0.f;
		case MOVE_None:
		default:
			return 0.f;
	}
}

void UMDCharacterMovementComponent::CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration)
{
	if(bIsSliding)
	{
		// If server sets MovementMode to None, our velocity will be 0,0,0 and we must abort. Otherwise we will get Nan when calculating new velocity.
		if(Velocity == FVector::ZeroVector)
		{
			return;
		}

		// Steer where the player is looking when sliding. Pressing arrow keys have no effect.
		FRotator ControlRot = GetController()->GetControlRotation();
		ControlRot.Pitch = 0.0;
		ControlRot.Roll = 0.0;
		Acceleration = ControlRot.Vector();

		// Steering while sliding
		const FVector VelRightDir = FVector::CrossProduct(FVector::UpVector, Velocity).GetSafeNormal();
		FVector Steering = Acceleration.GetSafeNormal() * GetMaxAcceleration();
		Steering = Steering.ProjectOnTo(VelRightDir);

		// Lower friction, otherwise we won't slide very far, unless we are sliding on ice
		Friction *= SlidingFrictionMulti;

		// Handle sliding down a slope.
		const auto& Hit = CurrentFloor.HitResult;
		FVector Slope = FVector::ZeroVector;
		if(Hit.IsValidBlockingHit() && !FMath::IsNearlyEqual(Hit.ImpactNormal.Z, 1.0, 1E-06)) // Normal isn't pointing straight up = slope
		{
			// We want to keep the direction, but scale it by slope angle and how much along the slope we are going

			const FVector AcrossSlope = FVector::CrossProduct(FVector::UpVector, Hit.ImpactNormal);
			const FVector DownSlope = AcrossSlope.Cross(Hit.ImpactNormal);

			// Calculate the angle of the slope
			double SlopeSteepness = FVector::DotProduct(DownSlope.GetSafeNormal2D(), DownSlope.GetSafeNormal());
			double MaxSteepness = FMath::Cos(FMath::DegreesToRadians(SlidingSlopeAngleToReachMaxAcceleration));
			// Map value so that we will reach max acceleration at SlidingSlopeAngleToReachMaxAcceleration
			SlopeSteepness = FMath::GetMappedRangeValueClamped(FVector2D(MaxSteepness, 1.0), FVector2D(1.0, 0.0), SlopeSteepness);

			// How much along slope our velocity is, less than 0.0 means we are going uphill
			double VelAlongSlope = FVector::DotProduct(Velocity.GetSafeNormal2D(), DownSlope.GetSafeNormal2D());
			// Stop steering uphill
			Steering *= FMath::Max(0.0, VelAlongSlope);

			// If we are going uphill or almost perpendicular to the slope, slow down
			// This branch looks ugly, but feels much better while playing than using lerp etc
			double MaxDotBeforeSlowingDown = FMath::Cos(FMath::DegreesToRadians(SlidingMaxAngleToDownSlopeBeforeSlowingDown));
			if(VelAlongSlope <= MaxDotBeforeSlowingDown)
			{
				double BrakingMulti = FMath::GetMappedRangeValueClamped(FVector2D(MaxDotBeforeSlowingDown, 0.0), FVector2D(0.0, 1.0), VelAlongSlope);
				Slope = Velocity.GetSafeNormal() * -BrakingDecelerationSliding * BrakingMulti;
			}
			// Otherwise we want to keep the direction but scale size
			else
			{
				Slope = Velocity.GetSafeNormal() * (GetMaxAcceleration() * VelAlongSlope * SlopeSteepness);
			}
		}
		FVector ResistingForce = (Velocity.GetSafeNormal() * -BrakingDecelerationSliding) * Friction * DeltaTime; // Otherwise we slide forever on flat surfaces
		Acceleration = (Steering * SlideTurnStrength + Slope).GetClampedToMaxSize(GetMaxAcceleration());
		Velocity += ResistingForce + (Acceleration * DeltaTime);
		Velocity = Velocity.GetClampedToMaxSize(GetMaxSpeed()); // WARNING! This clamps our Velocity to MaxWalkSpeed when we slide off ledge, see GetMaxSpeed comments.
	}
	else
	{
		Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
	}
}

bool UMDCharacterMovementComponent::IsWallRunning() const
{
	return (MovementMode == MOVE_Custom) && (CustomMovementMode == MDMOVE_WallRun) && UpdatedComponent;
}

bool UMDCharacterMovementComponent::CanStartMantle() const
{
	auto* MDCharacter = CastChecked<AMDCharacter>(GetCharacterOwner());
	return IsValid(MDCharacter) && !MDCharacter->IsMantling() && !IsCrouching();
}

FMantleInfo UMDCharacterMovementComponent::TryFindMantleLocation() const
{
	ensure(MaxHeightFromFloor_Mantle > MinHeightFromFloor_Mantle);
	ensure(!MantleTraceObjectTypes.IsEmpty());

	FMantleInfo Info;
	const auto* World = GetWorld();
	auto* MDCharacter = CastChecked<AMDCharacter>(GetCharacterOwner());

	// Offset for Z. CharacterMovementComponent makes capsule hover over the ground a little, so we need to offset the height.
	// This also makes sure we can mantle obstacle that is between Min and Max height.
	// This doesn't really need to be tuned after finding a good number, unless want to make mantle more precise.
	constexpr float ZOffset = 10.0f;

	const FVector ActorLocation = MDCharacter->GetActorLocation();
	const FVector ActorForward = MDCharacter->GetActorForwardVector();
	const auto* Capsule = MDCharacter->GetCapsuleComponent();
	const float Radius = Capsule->GetScaledCapsuleRadius();
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const FVector FeetLocation = ActorLocation - FVector::UpVector * HalfHeight;

	// Split our up movement to smaller deltas
	const FVector DeltaUp = (FVector::UpVector * (MaxHeightFromFloor_Mantle - MinHeightFromFloor_Mantle + ZOffset)) / static_cast<double>(MaxIterations_Mantle);
	// How much we should trace forward looking for free spot to fit our capsule
	const FVector DeltaForward = ActorForward * ForwardTraceLength_Mantle;
	int32 NumHitsForward = 0; // Need to hit something to have something to grab

	// Start at Feet + MinHeight + Capsule half radius, since we want the bottom of the capsule to be at start height.
	// We also need to start a bit lower than start height, otherwise if something is exactly tall as MinHeight, it might not be hit
	FVector CurrentLocation = FeetLocation + FVector::UpVector * (MinHeightFromFloor_Mantle + HalfHeight - ZOffset);
	// Need to scale the capsule just a tiny bit, otherwise we might hit the wall we are standing next to
	constexpr float DownScaleMulti = 0.98f;
	auto Shape = FCollisionShape::MakeCapsule(Radius * DownScaleMulti, HalfHeight);
	FCollisionObjectQueryParams ObjectParams(MantleTraceObjectTypes);
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(MDCharacter);

	for (int32 i = 0; i < MaxIterations_Mantle; ++i)
	{
		CurrentLocation += DeltaUp;

		FHitResult Hit;

		// Trace up
		if(World->SweepSingleByObjectType(Hit, ActorLocation, CurrentLocation, FQuat::Identity, ObjectParams, Shape, CollisionParams))
		{
			Info.bCanMantle = false;
			break;
		}

		// Trace forward
		if (!World->SweepSingleByObjectType(Hit, CurrentLocation, CurrentLocation + DeltaForward, FQuat::Identity, ObjectParams, Shape, CollisionParams))
		{
			// Need at least one hit, otherwise we will vault thin air
			if (NumHitsForward)
			{
				Info.bCanMantle = true;
				Info.StartLocation = ActorLocation;
				Info.EndLocation = CurrentLocation + FVector::UpVector * 1.0f; // Can tweak this Z offset if needed. 1.0f works fine so we hover over the floor a little
				break;
			}
		}
		else
		{
			++NumHitsForward;
		}
	}

	return Info;
}

void UMDCharacterMovementComponent::DoMantle(const FMantleInfo& MantleInfo)
{
	auto* MDCharacter = CastChecked<AMDCharacter>(GetCharacterOwner());
	const TSharedPtr<FRootMotionSource_MoveToForce> RootMotion = MakeShared<FRootMotionSource_MoveToForce>();
	RootMotion->StartLocation = MantleInfo.StartLocation;
	RootMotion->TargetLocation = MantleInfo.EndLocation;
	RootMotion->Duration = 0.2f; // Hardcoded for now, no really point exposing this. Could calculate this based on distance in the future.
	RootMotion->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
	RootMotion->AccumulateMode = ERootMotionAccumulateMode::Override;
	RootMotion->Settings.SetFlag(ERootMotionSourceSettingsFlags::UseSensitiveLiftoffCheck);
	RootMotion->FinishVelocityParams.ClampVelocity = 50.0f; // Hardcoded for now, no really point exposing this.
	RootMotion->FinishVelocityParams.SetVelocity = MDCharacter->GetActorForwardVector() * 250.0 + FVector::UpVector * 50.0; // Hardcoded for now, no really point exposing this.
	RootMotion->InstanceName = RootMotionName_Mantle;

	ApplyRootMotionSource(RootMotion);

	if (MDCharacter->IsLocallyControlled() && GetOwnerRole() == ROLE_AutonomousProxy) // We are local but not the server
	{
		FlushServerMoves(); // Removes jerkiness from the end of the movement
		MDCharacter->ServerDoMantleRPC();
	}
}

bool UMDCharacterMovementComponent::CanStartSprinting() const
{
	return true;
}

void UMDCharacterMovementComponent::StartSprinting()
{
	bIsSprinting = true;
}

void UMDCharacterMovementComponent::StopSprinting()
{
	bIsSprinting = false;
}

bool UMDCharacterMovementComponent::CanStartSliding() const
{
	if(bIsSliding || Velocity.SizeSquared() < FMath::Square(SlideEnterRequiredSpeed) || IsFalling() || IsSwimming() || IsWallRunning())
	{
		return false;
	}
	return true;
}

bool UMDCharacterMovementComponent::CanContinueSliding() const
{
	if(Velocity.SizeSquared() < FMath::Square(SlideMinSpeed) || IsFalling())
	{
		return false;
	}

	return true;
}

void UMDCharacterMovementComponent::StartSliding()
{
	bIsSliding = true;
	Velocity += Velocity.GetSafeNormal() * SlideStartBoost;
	if(GetOwnerRole() != ROLE_SimulatedProxy)
	{
		CastChecked<AMDCharacter>(GetCharacterOwner())->SetIsSliding(true);
	}
}

void UMDCharacterMovementComponent::StopSliding()
{
	bIsSliding = false;
	// If local player, require lifting finger from input and pressing again
	if(GetCharacterOwner()->IsLocallyControlled())
	{
		bWantsToSlide = false;
	}
	if (GetOwnerRole() != ROLE_SimulatedProxy)
	{
		CastChecked<AMDCharacter>(GetCharacterOwner())->SetIsSliding(false);
	}
}

bool UMDCharacterMovementComponent::FindWallForWallRunning(FHitResult& OutHit) const
{
	const FVector OwnerForward = GetCharacterOwner()->GetActorForwardVector();
	// If we are not applying any forward acceleration, we don't want to start wall running, since we would just stop and instantly drop off the wall
	if(FVector::DotProduct(OwnerForward, Acceleration) <= 0.0)
	{
		return false;
	}

	const FVector OwnerRight = GetCharacterOwner()->GetActorRightVector();
	FVector Start = GetCharacterOwner()->GetActorLocation();
	FVector EndRight = Start + OwnerRight * MaxDistanceToTraceForWall;
	FVector EndLeft = Start + OwnerRight * -MaxDistanceToTraceForWall;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetCharacterOwner());

	// See that we have min distance from floor
	// If we have walkable floor, no need to trace
	if(CurrentFloor.bWalkableFloor)
	{
		return false;
	}
	// Trace, if no hit == there is no floor under us, trace from feet location
	FHitResult FloorHit;
	if(GetWorld()->LineTraceSingleByChannel(FloorHit, GetActorFeetLocation(), GetActorFeetLocation() + FVector::DownVector * MinDistanceToFloor, ECC_WorldStatic, QueryParams))
	{
		return false;
	}

	// Right
	FHitResult HitRight;
	GetWorld()->LineTraceSingleByChannel(HitRight, Start, EndRight, ECC_WorldStatic, QueryParams);

	// Left
	FHitResult HitLeft;
	GetWorld()->LineTraceSingleByChannel(HitLeft, Start, EndLeft, ECC_WorldStatic, QueryParams);

	// Both hit, take the closer one
	if(HitRight.bBlockingHit && HitLeft.bBlockingHit)
	{
		OutHit = HitRight.Distance < HitLeft.Distance ? HitRight : HitLeft;
		return true;
	}

	if(HitRight.bBlockingHit)
	{
		OutHit = HitRight;
		return true;
	}
	if(HitLeft.bBlockingHit)
	{
		OutHit = HitLeft;
		return true;
	}

	return false;
}

void UMDCharacterMovementComponent::PhysCustom(float DeltaTime, int32 Iterations)
{
	Super::PhysCustom(DeltaTime, Iterations);
	switch(CustomMovementMode)
	{
		case MDMOVE_WallRun:
			PhysWallRun(DeltaTime, Iterations);
			break;
		case MDMOVE_Rooted:
			PhysRooted(DeltaTime, Iterations);
			break;
		default:
			break;
	}
}

void UMDCharacterMovementComponent::PhysWallRun(float DeltaTime, int32 Iterations)
{
	// Use the same early returns that the PhysWalking does
	if (DeltaTime < MIN_TICK_TIME)
	{
		return;
	}

	if (!CharacterOwner || (!CharacterOwner->Controller && !bRunPhysicsWithNoController && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() && (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)))
	{
		Acceleration = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
		return;
	}

	if (!UpdatedComponent->IsQueryCollisionEnabled())
	{
		SetMovementMode(MOVE_Walking);
		return;
	}

	// Could add better support for root motion, now we just drop off the wall
	if(HasAnimRootMotion())
	{
		SetMovementMode(MOVE_Falling);
		return;
	}

	const float CapsuleRadius = GetCharacterOwner()->GetCapsuleComponent()->GetScaledCapsuleRadius();

	// Basically do the same as the built in PhysWalking does but tweak it to our WallRun mode
	float RemainingTime = DeltaTime;
	while ((RemainingTime >= MIN_TICK_TIME) && (Iterations < MaxSimulationIterations) && CharacterOwner && (CharacterOwner->Controller || bRunPhysicsWithNoController || HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocity() || (CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy)))
	{
		++Iterations;
		const float TimeTick = GetSimulationTimeStep(RemainingTime, Iterations);
		RemainingTime -= TimeTick;

		// Save current values
		const FVector OldLocation = UpdatedComponent->GetComponentLocation();
		const FVector OldVelocity = Velocity;

		if(IsFalling())
		{
			// Root motion could have put us into Falling.
			// No movement has taken place this movement tick so we pass on full time/past iteration count
			StartNewPhysics(RemainingTime + TimeTick, Iterations - 1);
			return;
		}

		FHitResult WallHitResult;
		if (!FindWallForWallRunning(WallHitResult))
		{
			SetMovementMode(MOVE_Falling); // Set movement mode, otherwise we get stuck in infinite loop
			StartNewPhysics(RemainingTime + TimeTick, Iterations - 1);
			return;
		}

		Acceleration = FVector::VectorPlaneProject(Acceleration, WallHitResult.Normal);
		Acceleration.Z = 0.0f;
		CalcVelocity(DeltaTime, GroundFriction, false, GetMaxBrakingDeceleration());
		Velocity = FVector::VectorPlaneProject(Velocity, WallHitResult.Normal);
		Velocity.Z = -DownwardPullForce;

		if(Velocity.SizeSquared() < FMath::Square(MinSpeedToKeepWallRunning))
		{
			SetMovementMode(MOVE_Falling);
			StartNewPhysics(RemainingTime + TimeTick, Iterations - 1);
			return;
		}

		// Wants to jump off wall
		if (bWantsToJumpOffWall)
		{
			FVector WallJumpVelocity = Velocity.GetSafeNormal() * 1000.0 + WallHitResult.Normal * 500.0 + FVector::UpVector * 325.0; // Could expose this or make a function that calculates this
			JumpOffTheWall(WallJumpVelocity);
			SetMovementMode(MOVE_Falling);
			StartNewPhysics(RemainingTime + TimeTick, Iterations - 1);
		}

		// Compute move parameters
		const FVector MoveDelta = Velocity * TimeTick;
		const FVector Horizontal(Velocity.X, Velocity.Y, 0.0); // Don't rotate the capsule up/down
		const FQuat NewQuat = Horizontal.ToOrientationQuat();
		const bool bZeroDelta = MoveDelta.IsNearlyZero();

		if(bZeroDelta)
		{
			RemainingTime = 0.0f;
		}
		else
		{
			// Keep distance to the wall, we don't want to collide with it
			const double DistanceToWall = FVector::Distance(WallHitResult.ImpactPoint, OldLocation);
			const double ScalarToUse = (CapsuleRadius + DesiredDistanceToWallWhenWallRunning) - DistanceToWall;
			FVector MoveAwayDelta = WallHitResult.Normal * ScalarToUse * DesiredDistanceMaintainSpeed * TimeTick;
			FHitResult MoveAwayHit;
			SafeMoveUpdatedComponent(MoveAwayDelta, NewQuat, true, MoveAwayHit);

			// Try move along the wall
			FHitResult MoveHit;
			SafeMoveUpdatedComponent(MoveDelta, NewQuat, true, MoveHit);

			if(IsFalling())
			{
				const float DesiredDist = MoveDelta.Size();
				if (DesiredDist > UE_KINDA_SMALL_NUMBER)
				{
					const float ActualDist = (UpdatedComponent->GetComponentLocation() - OldLocation).Size2D();
					RemainingTime += TimeTick * (1.f - FMath::Min(1.f, ActualDist / DesiredDist));
				}
				StartNewPhysics(RemainingTime, Iterations);
				return;
			}
			else if(IsSwimming()) // Entered water
			{
				StartSwimming(OldLocation, OldVelocity, TimeTick, RemainingTime, Iterations);
				return;
			}
		}

		// If we didn't move at all this iteration then abort (since future iterations will also be stuck).
		if (UpdatedComponent->GetComponentLocation() == OldLocation)
		{
			RemainingTime = 0.f;
			break;
		}
	}

}

void UMDCharacterMovementComponent::PhysRooted(float DeltaTime, int32 Iterations)
{
	// We want to be rooted to the ground, we don't care about anything else
	if(!HasValidData())
	{
		return;
	}

	constexpr double MaxTraceDistance = 100000.0;
	FHitResult Hit;
	FVector Start = CharacterOwner->GetActorLocation();
	FVector End = Start + FVector::DownVector * MaxTraceDistance;
	float ZOffSetFromGround = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 1.0f;

	if(GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility))
	{
		FVector NewLocation = Hit.Location + FVector::UpVector * ZOffSetFromGround;
		FVector Delta = NewLocation - NewLocation;
		MoveUpdatedComponent(Delta, CharacterOwner->GetActorQuat(), false, nullptr, ETeleportType::ResetPhysics);

		// Update floor
		FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, false);
		AdjustFloorHeight();
		SetBaseFromFloor(CurrentFloor);
	}
	else
	{
		UE_LOGFMT(LogTemp, Error, "UMDCharacterMovementComponent::PhysRooted can't find valid floor location!");
	}
}

void UMDCharacterMovementComponent::JumpOffTheWall(const FVector& WallJumpVelocity)
{
	Launch(WallJumpVelocity);
	bWantsToJumpOffWall = false;
}

void UMDCharacterMovementComponent::SetPostLandedPhysics(const FHitResult& Hit)
{
	Super::SetPostLandedPhysics(Hit);

	// If we were sliding before falling, continue sliding when hitting ground
	bWantsToSlide = ShouldSlideOnLanded(Hit);
	bWasSlidingBeforeFalling = false;
}

bool UMDCharacterMovementComponent::ShouldSlideOnLanded(const FHitResult& Hit) const
{
	return (bWantsToSlide == true) || ((bWasSlidingBeforeFalling == true) && MovementMode == MOVE_Walking && CanStartSliding());
}

void UMDCharacterMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	// Proxies get replicated state.
	if (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)
	{
		auto* MDCharacter = CastChecked<AMDCharacter>(GetCharacterOwner());
		// Check for a change in crouch state. Players toggle crouch by changing bWantsToCrouch.
		// Force crouch when sliding
		const bool bIsCrouching = IsCrouching();
		if (bIsCrouching && !bIsSliding && (!bWantsToCrouch || !CanCrouchInCurrentState()))
		{
			UnCrouch(false);
		}
		else if (!bIsCrouching && (bWantsToCrouch || bIsSliding) && CanCrouchInCurrentState())
		{
			Crouch(false);
		}
		// Check if player wants to sprint.
		if(!bIsSprinting && (bWantsToSprint && CanStartSprinting()))
		{
			StartSprinting();
		}
		// Check if player wants to stop sprinting.
		else if(bIsSprinting && !bWantsToSprint)
		{
			StopSprinting();
		}
		// Check if player wants to slide. 
		if(!bIsSliding && (bWantsToSlide && CanStartSliding()))
		{
			StartSliding();
		}
		// Check if we should start wall running
		const bool bPressedJump = GetCharacterOwner()->bPressedJump;
		FHitResult WallHit;
		if(bPressedJump && !IsWallRunning() && FindWallForWallRunning(WallHit)) // TODO: could optimize saving the result to CurrentWall variable
		{
			SetMovementMode(MOVE_Custom, MDMOVE_WallRun);
		}
		// Check if we are wall running and want to jump off the wall
		else if(bPressedJump && IsWallRunning()) // bPressedJump is already predicted and networked and it's consumed after it's pressed, we need to "save" it here, since it will be consumed
		{
			bWantsToJumpOffWall = true;
		}
		// Check if we should clear Mantle bool
		if(MDCharacter->IsMantling() && !GetRootMotionSource(RootMotionName_Mantle).IsValid())
		{
			MDCharacter->SetIsMantling(false);
		}
	}
}

void UMDCharacterMovementComponent::UpdateCharacterStateAfterMovement(float DeltaSeconds)
{
	// Proxies get replicated crouch state.
	if (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)
	{
		// Uncrouch if no longer allowed to be crouched
		if (IsCrouching() && !CanCrouchInCurrentState())
		{
			UnCrouch(false);
		}
		// Stop sliding if can't slide anymore
		if(bIsSliding && !CanContinueSliding())
		{
			StopSliding();
		}
	}
}

void UMDCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);
	bWantsToSprint = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
	bWantsToSlide = (Flags & FSavedMove_Character::FLAG_Custom_1) != 0;
}

bool UMDCharacterMovementComponent::ClientUpdatePositionAfterServerUpdate()
{
	// Basically we copy Super and add our input flags, UGLY but has to be done

	if (!HasValidData())
	{
		return false;
	}

	FNetworkPredictionData_Client_Character* ClientData = GetPredictionData_Client_Character();
	check(ClientData);

	if (!ClientData->bUpdatePosition)
	{
		return false;
	}

	ClientData->bUpdatePosition = false;

	// Don't do any network position updates on things running PHYS_RigidBody
	if (CharacterOwner->GetRootComponent() && CharacterOwner->GetRootComponent()->IsSimulatingPhysics())
	{
		return false;
	}

	if (ClientData->SavedMoves.Num() == 0)
	{
		UE_LOG(LogNetPlayerMovement, Verbose, TEXT("ClientUpdatePositionAfterServerUpdate No saved moves to replay"), ClientData->SavedMoves.Num());

		// With no saved moves to resimulate, the move the server updated us with is the last move we've done, no resimulation needed.
		CharacterOwner->bClientResimulateRootMotion = false;
		if (CharacterOwner->bClientResimulateRootMotionSources)
		{
			// With no resimulation, we just update our current root motion to what the server sent us
			UE_LOG(LogRootMotion, VeryVerbose, TEXT("CurrentRootMotion getting updated to ServerUpdate state: %s"), *CharacterOwner->GetName());
			CurrentRootMotion.UpdateStateFrom(CharacterOwner->SavedRootMotion);
			CharacterOwner->bClientResimulateRootMotionSources = false;
		}
		CharacterOwner->SavedRootMotion.Clear();

		return false;
	}

	// Save important values that might get affected by the replay.
	const float SavedAnalogInputModifier = AnalogInputModifier;
	const FRootMotionMovementParams BackupRootMotionParams = RootMotionParams; // For animation root motion
	const FRootMotionSourceGroup BackupRootMotion = CurrentRootMotion;
	const bool bRealPressedJump = CharacterOwner->bPressedJump;
	const float RealJumpMaxHoldTime = CharacterOwner->JumpMaxHoldTime;
	const int32 RealJumpMaxCount = CharacterOwner->JumpMaxCount;
	const bool bRealCrouch = bWantsToCrouch;
	const bool bRealForceMaxAccel = bForceMaxAccel;

	// Our custom stuff
	const bool bRealSprint = bWantsToSprint;
	const bool bRealSlide = bWantsToSlide;
	//

	CharacterOwner->bClientWasFalling = (MovementMode == MOVE_Falling);
	CharacterOwner->bClientUpdating = true;
	bForceNextFloorCheck = true;

	// Replay moves that have not yet been acked.
	UE_LOG(LogNetPlayerMovement, Verbose, TEXT("ClientUpdatePositionAfterServerUpdate Replaying %d Moves, starting at Timestamp %f"), ClientData->SavedMoves.Num(), ClientData->SavedMoves[0]->TimeStamp);
	for (int32 i = 0; i < ClientData->SavedMoves.Num(); i++)
	{
		FSavedMove_Character* const CurrentMove = ClientData->SavedMoves[i].Get();
		checkSlow(CurrentMove != nullptr);

		// Make current SavedMove accessible to any functions that might need it.
		SetCurrentReplayedSavedMove(CurrentMove);

		CurrentMove->PrepMoveFor(CharacterOwner);

		if (ShouldUsePackedMovementRPCs())
		{
			// Make current move data accessible to MoveAutonomous or any other functions that might need it.
			if (FCharacterNetworkMoveData* NewMove = GetNetworkMoveDataContainer().GetNewMoveData())
			{
				SetCurrentNetworkMoveData(NewMove);
				NewMove->ClientFillNetworkMoveData(*CurrentMove, FCharacterNetworkMoveData::ENetworkMoveType::NewMove);
			}
		}

		MoveAutonomous(CurrentMove->TimeStamp, CurrentMove->DeltaTime, CurrentMove->GetCompressedFlags(), CurrentMove->Acceleration);

		CurrentMove->PostUpdate(CharacterOwner, FSavedMove_Character::PostUpdate_Replay);
		SetCurrentNetworkMoveData(nullptr);
		SetCurrentReplayedSavedMove(nullptr);
	}
	const bool bPostReplayPressedJump = CharacterOwner->bPressedJump;

	if (FSavedMove_Character* const PendingMove = ClientData->PendingMove.Get())
	{
		PendingMove->bForceNoCombine = true;
	}

	// Restore saved values.
	AnalogInputModifier = SavedAnalogInputModifier;
	RootMotionParams = BackupRootMotionParams;
	CurrentRootMotion = BackupRootMotion;
	if (CharacterOwner->bClientResimulateRootMotionSources)
	{
		// If we were resimulating root motion sources, it's because we had mismatched state
		// with the server - we just resimulated our SavedMoves and now need to restore
		// CurrentRootMotion with the latest "good state"
		UE_LOG(LogRootMotion, VeryVerbose, TEXT("CurrentRootMotion getting updated after ServerUpdate replays: %s"), *CharacterOwner->GetName());
		CurrentRootMotion.UpdateStateFrom(CharacterOwner->SavedRootMotion);
		CharacterOwner->bClientResimulateRootMotionSources = false;
	}
	CharacterOwner->SavedRootMotion.Clear();
	CharacterOwner->bClientResimulateRootMotion = false;
	CharacterOwner->bClientUpdating = false;
	CharacterOwner->bPressedJump = bRealPressedJump || bPostReplayPressedJump;
	CharacterOwner->JumpMaxHoldTime = RealJumpMaxHoldTime;
	CharacterOwner->JumpMaxCount = RealJumpMaxCount;
	bWantsToCrouch = bRealCrouch;
	bForceMaxAccel = bRealForceMaxAccel;

	// Our custom stuff
	bWantsToSprint = bRealSprint;
	bWantsToSlide = bRealSlide;
	//

	bForceNextFloorCheck = true;

	return (ClientData->SavedMoves.Num() > 0);
}

void UMDCharacterMovementComponent::MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags, const FVector& NewAccel)
{
	if (!HasValidData())
	{
		return;
	}

	UpdateFromCompressedFlags(CompressedFlags);
	CharacterOwner->CheckJumpInput(DeltaTime);

	Acceleration = ConstrainInputAcceleration(NewAccel);
	Acceleration = Acceleration.GetClampedToMaxSize(GetMaxAcceleration());
	AnalogInputModifier = ComputeAnalogInputModifier();

	const FVector OldLocation = UpdatedComponent->GetComponentLocation();
	const FQuat OldRotation = UpdatedComponent->GetComponentQuat();

	PerformMovement(DeltaTime);

	// Check if data is valid as PerformMovement can mark character for pending kill
	if (!HasValidData())
	{
		return;
	}

	// If not playing root motion, tick animations after physics. We do this here to keep events, notifies, states and transitions in sync with client updates.
	if (!CharacterOwner->bClientUpdating && !CharacterOwner->IsPlayingRootMotion() && CharacterOwner->GetMesh())
	{
		// We do not TickPose here, so we have smooth listen server animations

		if (CharacterOwner->GetMesh()->ShouldOnlyTickMontages(DeltaTime))
		{
			// If we're not doing a full anim graph update on the server, 
			// trigger events right away, as we could be receiving multiple ServerMoves per frame.
			CharacterOwner->GetMesh()->ConditionallyDispatchQueuedAnimEvents();
		}
	}

	if (CharacterOwner && UpdatedComponent)
	{
		// Smooth local view of remote clients on listen servers
		if (CharacterOwner->GetRemoteRole() == ROLE_AutonomousProxy && IsNetMode(NM_ListenServer))
		{
			SmoothCorrection(OldLocation, OldRotation, UpdatedComponent->GetComponentLocation(), UpdatedComponent->GetComponentQuat());
		}
	}
}


// Classes needed for network and prediction
// -----------------------------------------

//SavedMove
void FMDSavedMove::Clear()
{
	Super::Clear();
	bWantsToSprint = false;
	bWantsToSlide = false;
}

uint8 FMDSavedMove::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();
	if(bWantsToSprint)
	{
		Result |= FLAG_Custom_0;
	}
	if(bWantsToSlide)
	{
		Result |= FLAG_Custom_1;
	}
	return Result;
}

void FMDSavedMove::SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);
	const auto* MovementComponent = CastChecked<UMDCharacterMovementComponent>(Character->GetCharacterMovement());
	bWantsToSprint = MovementComponent->bWantsToSprint;
	bWantsToSlide = MovementComponent->bWantsToSlide;
}

void FMDSavedMove::PrepMoveFor(ACharacter* Character)
{
	Super::PrepMoveFor(Character);
	auto* MovementComponent = CastChecked<UMDCharacterMovementComponent>(Character->GetCharacterMovement());
	MovementComponent->bWantsToSprint = bWantsToSprint;
	MovementComponent->bWantsToSlide = bWantsToSlide;
}

// End SavedMove

// NetworkPredictionData_Client
FMDNetworkPredictionData_Client::FMDNetworkPredictionData_Client(const UCharacterMovementComponent& ClientMovement) : Super(ClientMovement)
{
}

FSavedMovePtr FMDNetworkPredictionData_Client::AllocateNewMove()
{
	return MakeShared<FMDSavedMove>();
}
// End NetworkPredictionData_Client
