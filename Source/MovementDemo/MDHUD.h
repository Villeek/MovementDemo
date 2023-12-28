// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MDHUD.generated.h"

class UMDPlayerMainHUDWidget;
class AMDPlayerState;

/**
 * MDHUD encapsulates all HUD/Widget/UI functionality.
 * The goal is that we don't need to ever do stuff like GetMainWidget() .. MainWidget->DoStuff, but instead we route all HUD/Widget/UI related stuff trough this class.
 * Because we can't guarantee that stuff replicate in order we want, we must be sure that stuff is valid before we try to update it. That's why we need lot of extra checks for valid etc.
 * TODO: write this again
 */
UCLASS()
class MOVEMENTDEMO_API AMDHUD : public AHUD
{
	GENERATED_BODY()

public:

	AMDHUD();

	// Called from AMDPlayerController::OnRep_PlayerState to ensure we have PlayerState
	void CreateWidgetsForLocalPlayer();

	// Helper to get local HUD
	static AMDHUD* GetLocalHUD(const UWorld* World);

	virtual void Tick(float DeltaSeconds) override;

	void SetNameForPlayer(AMDPlayerState* PlayerState);

	void SetBestTimeForPlayer(AMDPlayerState* PlayerState);

	void SetLastTimeForPlayer(AMDPlayerState* PlayerState);

	void SetWinsForPlayer(AMDPlayerState* PlayerState);

	void SetPlayerReachedGoal(AMDPlayerState* PlayerState);

	void CreateOrRefreshWidgetForRemotePlayer(AMDPlayerState* PlayerState);

	void RemoveWidgetForRemotePlayer(AMDPlayerState* PlayerState);

	void TryCreateWidgetsForRemotePlayers();

protected:

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UMDPlayerMainHUDWidget> MainHUDWidgetClass;

	UPROPERTY(Transient)
	UMDPlayerMainHUDWidget* MainWidget;

	virtual void BeginPlay() override;

	// Helper to check if passed in PlayerState is ours. NOT safe to call if PlayerController's PlayerState isn't replicated yet!
	bool IsLocalPlayerState(AMDPlayerState* PlayerState) const;
};
