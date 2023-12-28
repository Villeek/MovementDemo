// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "MDGameStateBase.generated.h"

/**
 * MDGameStateBase contains shared replicated data that needs to be available for all clients.
 */
UCLASS()
class MOVEMENTDEMO_API AMDGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

public:

	AMDGameStateBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetStartingShotTime(double Time);

	double GetStartingShotServerTime() const { return StartingShotServerTime; }

protected:

	UPROPERTY(ReplicatedUsing = OnRep_StartingShotServerTime)
	double StartingShotServerTime = -1.0;

	UFUNCTION()
	void OnRep_StartingShotServerTime();
};
