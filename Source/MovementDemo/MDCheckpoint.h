// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MDCheckpoint.generated.h"

class UBoxComponent;

/**
 * Checkpoint for MDCharacters. When character steps on this checkpoint, it will set as a current checkpoint.
 * Can be placed inside levels.
 * See MDCheckpointSubsystem.h for overview of checkpoint system.
 */
UCLASS()
class MOVEMENTDEMO_API AMDCheckpoint : public AActor
{
	GENERATED_BODY()
	
public:	

	AMDCheckpoint();

	virtual void PostInitializeComponents() override;

protected:

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* RootMeshComp;

	UPROPERTY(VisibleAnywhere)
	UBoxComponent* TriggerComp;

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

};
