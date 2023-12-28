// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MDCheckpointTrackerComponent.generated.h"

class AMDCheckpoint;

/**
 * Keeps track of current and next checkpoints.
 * See MDCheckpointSubsystem.h for overview of checkpoint system.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MOVEMENTDEMO_API UMDCheckpointTrackerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

	UMDCheckpointTrackerComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetCheckpoints(AMDCheckpoint* Current, AMDCheckpoint* Next);

	AMDCheckpoint* GetCurrentCheckpoint() const { return CurrentCheckpoint; }

	AMDCheckpoint* GetNextCheckpoint() const { return NextCheckpoint; }

protected:

	UPROPERTY(ReplicatedUsing = OnRep_CurrentCheckpoint)
	AMDCheckpoint* CurrentCheckpoint;

	UPROPERTY(ReplicatedUsing = OnRep_NextCheckpoint)
	AMDCheckpoint* NextCheckpoint;

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnRep_CurrentCheckpoint(AMDCheckpoint* OldCheckpoint);

	UFUNCTION()
	void OnRep_NextCheckpoint(AMDCheckpoint* OldCheckpoint);
};
