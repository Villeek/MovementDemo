// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MDGameModeBase.generated.h"

class AMDPlayerState;

UENUM()
enum class EMDMatchState : uint8
{
	WaitingMatchReset, // waiting for game to reset and start new match
	WaitingMatchStartingShot, // waiting for 3..2..1.. GO!
	OnGoing, // Match is in progress
};

/**
 * MDGameModeBase takes care of flow of the game server side.
 * We start new match and reset wins when new player joins.
 * We then wait for any player to reach the end and restart the match and loop this forever.
 */

UCLASS()
class MOVEMENTDEMO_API AMDGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:

	AMDGameModeBase();

	virtual void PostLogin(APlayerController* NewPlayer) override;

	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	virtual void RestartPlayerAtPlayerStart(AController* NewPlayer, AActor* StartSpot) override;

	EMDMatchState GetMatchState() const { return MatchState; }

	void SetMatchState(EMDMatchState NewState);

	void TryGiveWinToPlayer(AMDPlayerState* PlayerState);

protected:

	FTimerHandle TimerHandle_MatchReset;

	FTimerHandle TimerHandle_StartingShot;

	EMDMatchState MatchState;

	bool bHasWinBeenAssignedThisMatch = false;

	UPROPERTY(EditAnywhere)
	float MatchResetDelay = 5.0f;

	UPROPERTY(EditAnywhere)
	float StartingShotDelay = 3.0f;

	int32 NextPlayerNumber = 1;

	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal = TEXT("")) override;

	void BeginMatchReset();

	void StartCountdownToMatchReset();

	void StartCountdownToStartingShot();

	void OnStartingShot();

	void RootPlayer(APlayerController* PC);

	void UnRootPlayer(APlayerController* PC);

	void ResetWins();
};
