// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "MDPlayerState.generated.h"

/**
 * MDPlayerState contains player specific data which most are replicated.
 * OnReps are used to update widgets / UI.
 */
UCLASS()
class MOVEMENTDEMO_API AMDPlayerState : public APlayerState
{
	GENERATED_BODY()

public:

	AMDPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void OnRep_PlayerName() override;

	virtual void SetPlayerName(const FString& S) override;

	void SetGoalIsReached();

	void ResetHasReachedGoal();

	double GetBestCompletedTime() const { return BestCompletedTime; }

	double GetLastCompletedTime() const { return LastCompletedTime; }

	int32 GetWinCount() const {return Wins; }

	void SetWinCount(int32 NewCount);

	bool HasReachedGoal() const { return bHasReachedGoal; }

protected:

	UPROPERTY(ReplicatedUsing = OnRep_BestCompletedTime)
	double BestCompletedTime = -1.0;

	UPROPERTY(ReplicatedUsing = OnRep_LastCompletedTime)
	double LastCompletedTime = -1.0;

	UPROPERTY(ReplicatedUsing = OnRep_Wins)
	int32 Wins = 0;

	UPROPERTY(ReplicatedUsing = OnRep_HasReachedGoal)
	bool bHasReachedGoal = false;

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnRep_BestCompletedTime();

	UFUNCTION()
	void OnRep_LastCompletedTime();

	UFUNCTION()
	void OnRep_Wins();

	UFUNCTION()
	void OnRep_HasReachedGoal();

	void SetBestCompletedTime(double NewTime);

	void SetLastCompletedTime(double NewTime);
};
