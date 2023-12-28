// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MDPlayerMainHUDWidget.generated.h"

class UMDRemotePlayerHUDWidget;
class UTextBlock;
class UVerticalBox;
class AMDPlayerState;

/**
 * MDPlayerMainHUDWidget is the main widget for local player. It also contains widgets for remote players, which are created on demand.
 */
UCLASS()
class MOVEMENTDEMO_API UMDPlayerMainHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UMDPlayerMainHUDWidget(const FObjectInitializer& ObjectInitializer);

	void SetLocalPlayerName(const FString& Name);

	void SetCurrentMatchTimeText(double Time);

	void SetLocalBestTime(double Time);

	void SetLocalLastTime(double Time);

	void SetLocalWins(int32 Count);

	UVerticalBox* GetRemotePlayersVerticalBox() const { return VerticalBox_RemotePlayers; }

	void CreateOrRefreshWidgetForRemotePlayer(AMDPlayerState* PlayerState);

	void DestroyWidgetForRemotePlayer(AMDPlayerState* PlayerState);

protected:

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UMDRemotePlayerHUDWidget> RemotePlayerWidgetClass;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TextBlock_PlayerName;

	UPROPERTY(meta=(BindWidget))
	UTextBlock* TextBlock_CurrentMatchTime;

	UPROPERTY(meta=(BindWidget))
	UTextBlock* TextBlock_BestTime;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TextBlock_LastTime;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TextBlock_Wins;

	UPROPERTY(meta= (BindWidget))
	UVerticalBox* VerticalBox_RemotePlayers;
};
