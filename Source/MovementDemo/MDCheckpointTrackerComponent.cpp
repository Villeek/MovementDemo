// Fill out your copyright notice in the Description page of Project Settings.


#include "MovementDemo/MDCheckpointTrackerComponent.h"
#include "MovementDemo/MDCheckpoint.h"
#include "MovementDemo/MDCheckpointSubsystem.h"
#include "MovementDemo/MDCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"


UMDCheckpointTrackerComponent::UMDCheckpointTrackerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UMDCheckpointTrackerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	Params.Condition = COND_OwnerOnly;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, CurrentCheckpoint, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, NextCheckpoint, Params);
}

void UMDCheckpointTrackerComponent::SetCheckpoints(AMDCheckpoint* Current, AMDCheckpoint* Next)
{
	check(GetOwner()->HasAuthority());

	if(CurrentCheckpoint == Current && NextCheckpoint == Next)
	{
		return;
	}

	auto* OldCurrentCheckpoint = CurrentCheckpoint;
	auto* OldNextCheckpoint = NextCheckpoint;

	CurrentCheckpoint = Current;
	NextCheckpoint = Next;

	// If we are listen server, we need to call OnReps manually
	if(GetNetMode() == NM_ListenServer)
	{
		// We need to figure if we are locally controlled, maybe there is cleaner way but this works. We only want to call these if our locally controlled is the owner.
		if(const auto* MDCharacter = Cast<AMDCharacter>(GetOwner()))
		{
			if(MDCharacter->IsLocallyControlled())
			{
				OnRep_CurrentCheckpoint(OldCurrentCheckpoint);
				OnRep_NextCheckpoint(OldNextCheckpoint);
			}
		}
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, CurrentCheckpoint, this);
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, NextCheckpoint, this);
}

void UMDCheckpointTrackerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetNetMode() < NM_Client)
	{
		if (auto* CheckpointSubsystem = GetWorld()->GetSubsystem<UMDCheckpointSubsystem>())
		{
			CheckpointSubsystem->RegisterCheckpointTracker(*this);
		}
	}
}

void UMDCheckpointTrackerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetNetMode() < NM_Client)
	{
		if (auto* CheckpointSubsystem = GetWorld()->GetSubsystem<UMDCheckpointSubsystem>())
		{
			CheckpointSubsystem->UnregisterCheckpointTracker(*this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UMDCheckpointTrackerComponent::OnRep_CurrentCheckpoint(AMDCheckpoint* OldCheckpoint)
{
}

void UMDCheckpointTrackerComponent::OnRep_NextCheckpoint(AMDCheckpoint* OldCheckpoint)
{
}
