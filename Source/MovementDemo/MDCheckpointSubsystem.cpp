// Fill out your copyright notice in the Description page of Project Settings.


#include "MovementDemo/MDCheckpointSubsystem.h"
#include "MovementDemo/MDCharacter.h"
#include "MovementDemo/MDCheckpoint.h"
#include "MovementDemo/MDPlayerState.h"
#include "MovementDemo/MDCheckpointTrackerComponent.h"
#include "Logging/StructuredLog.h"

UMDCheckpointSubsystem::UMDCheckpointSubsystem()
{
	
}

bool UMDCheckpointSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}

	const UWorld* World = CastChecked<UWorld>(Outer);
	return World->GetNetMode() < NM_Client; // will be created in standalone, dedicated servers, and listen servers
}

void UMDCheckpointSubsystem::PostInitialize()
{
	Super::PostInitialize();
}

void UMDCheckpointSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	SortCheckpoints();
}

void UMDCheckpointSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	for(auto& Comp : CheckpointTrackerCompArray)
	{
		auto* CurrentCheckpoint = Comp->GetCurrentCheckpoint();
		auto* NextCheckpoint = Comp->GetNextCheckpoint();

		if(IsValid(CurrentCheckpoint) && IsValid(NextCheckpoint))
		{
			auto* CompOwner = Comp->GetOwner();
			auto PlayerZ = CompOwner->GetActorLocation().Z;
			auto NextCheckpointZ = NextCheckpoint->GetActorLocation().Z;

			// If player is below NextCheckpoint, reset player to CurrentCheckpoint
			if(PlayerZ < NextCheckpointZ)
			{
				FVector NewLoc = CurrentCheckpoint->GetActorLocation() + FVector::UpVector * 200.0;
				FRotator NewRot = (NextCheckpoint->GetActorLocation() - CompOwner->GetActorLocation()).GetSafeNormal().Rotation();
				CompOwner->TeleportTo(NewLoc, NewRot, false, true);
			}
		}
	}
}

TStatId UMDCheckpointSubsystem::GetStatId() const
{
	return UObject::GetStatID();
}

void UMDCheckpointSubsystem::RegisterCheckpoint(AMDCheckpoint& NewCheckpoint)
{
	check(!CheckpointsArray.Contains(&NewCheckpoint));
	CheckpointsArray.Emplace(&NewCheckpoint);

	// If BeginPlay has already been called, we need to sort the array again, since the checkpoint was spawned after sorting the array
	if(GetWorld()->HasBegunPlay())
	{
		SortCheckpoints();
	}
}

void UMDCheckpointSubsystem::UnregisterCheckpoint(AMDCheckpoint& CheckpointToRemove)
{
	check(CheckpointsArray.Contains(&CheckpointToRemove));
	CheckpointsArray.RemoveSingle(&CheckpointToRemove);
}

void UMDCheckpointSubsystem::RegisterCheckpointTracker(UMDCheckpointTrackerComponent& CheckpointTrackerComp)
{
	check(!CheckpointTrackerCompArray.Contains(&CheckpointTrackerComp));
	CheckpointTrackerCompArray.Emplace(&CheckpointTrackerComp);
}

void UMDCheckpointSubsystem::UnregisterCheckpointTracker(UMDCheckpointTrackerComponent& CheckpointTrackerComp)
{
	check(CheckpointTrackerCompArray.Contains(&CheckpointTrackerComp));
	CheckpointTrackerCompArray.RemoveSingleSwap(&CheckpointTrackerComp);
}

void UMDCheckpointSubsystem::OnTrackerOverlappedCheckpoint(UMDCheckpointTrackerComponent& CheckpointTrackerComp, AMDCheckpoint& Checkpoint)
{
	check(CheckpointsArray.Contains(&Checkpoint));

	AMDCheckpoint* NewCurrentCheckpoint = &Checkpoint;
	int32 CurIdx = CheckpointsArray.Find(&Checkpoint);
	AMDCheckpoint* NewNextCheckpoint = (CheckpointsArray.Num() - 1) > CurIdx ? CheckpointsArray[CurIdx + 1].Get() : nullptr;

	CheckpointTrackerComp.SetCheckpoints(NewCurrentCheckpoint, NewNextCheckpoint);

	// We reached Goal, the end.
	if(NewNextCheckpoint == nullptr)
	{
		// Check if we can find PlayerState and if so, notify it. Could use Interface here, but since nobody else than MDCharacter would use it, no point.
		if(const auto* MDCharacter = Cast<AMDCharacter>(CheckpointTrackerComp.GetOwner()))
		{
			if(auto* PS = MDCharacter->GetPlayerState<AMDPlayerState>())
			{
				PS->SetGoalIsReached();
			}
		}
	}
}

void UMDCheckpointSubsystem::SortCheckpoints()
{
	// Sort by descending order.
	// First checkpoint is the highest one and the last is the lowest one.
	CheckpointsArray.Sort([](const TWeakObjectPtr<AMDCheckpoint>& A, const TWeakObjectPtr<AMDCheckpoint>& B)
	{
		return A->GetActorLocation().Z > B->GetActorLocation().Z;
	});
}

bool UMDCheckpointSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// We only need this subsystem on Game worlds (PIE included)
	return (WorldType == EWorldType::Game || WorldType == EWorldType::PIE);
}
