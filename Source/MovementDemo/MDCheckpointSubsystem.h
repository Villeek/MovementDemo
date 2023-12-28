// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MDCheckpointSubsystem.generated.h"

class AMDCheckpoint;
class UMDCheckpointTrackerComponent;

/**
 * Server only WorldSubsystem that handles checkpoint related functionality.
 * 1. All MDCheckpoint actors register to this subsystem before BeginPlay.
 * 2. Checkpoints are sorted in descending order by their GetActorLocation Z value. Need to keep this in mind when designing the level or change SortCheckpoints() method.
 * 3. All MDCheckpointTrackerComponents register to this subsystem before BeginPlay.
 * 4. CheckpointSubsystem loops trough all TrackerComponents on tick to see if they have failed to reach next checkpoint and need to be reset to current checkpoint.
 * 5. If CheckpointTracker overlaps with Checkpoint, set new Current and Next checkpoints. If there is no next checkpoint, assume that the goal is reached.
 */
UCLASS()
class MOVEMENTDEMO_API UMDCheckpointSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:

	UMDCheckpointSubsystem();

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	/** Called once all UWorldSubsystems have been initialized */
	virtual void PostInitialize() override;

	/** Called when world is ready to start gameplay before the game mode transitions to the correct state and call BeginPlay on all actors */
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	virtual void Tick(float DeltaTime) override;

	virtual TStatId GetStatId() const override;

	/*Registers checkpoints. If BeginPlay was already called, will also sort all the checkpoints.*/
	void RegisterCheckpoint(AMDCheckpoint& NewCheckpoint);

	/*Unregisters checkpoint. Call this if destroying checkpoint actor during play.*/
	void UnregisterCheckpoint(AMDCheckpoint& CheckpointToRemove);

	void RegisterCheckpointTracker(UMDCheckpointTrackerComponent& CheckpointTrackerComp);

	void UnregisterCheckpointTracker(UMDCheckpointTrackerComponent& CheckpointTrackerComp);

	void OnTrackerOverlappedCheckpoint(UMDCheckpointTrackerComponent& CheckpointTrackerComp, AMDCheckpoint& Checkpoint);

protected:

	TArray<TWeakObjectPtr<AMDCheckpoint>> CheckpointsArray;

	TArray<TWeakObjectPtr<UMDCheckpointTrackerComponent>> CheckpointTrackerCompArray;

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	void SortCheckpoints();
};
