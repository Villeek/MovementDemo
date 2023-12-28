// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MDRemotePlayerHUDWidget.generated.h"

class AMDPlayerState;
class UTextBlock;

/**
 * MDRemotePlayerHUDWidget is a widget that is created for each remote player. 
 */
UCLASS()
class MOVEMENTDEMO_API UMDRemotePlayerHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	TWeakObjectPtr<AMDPlayerState> OwningPlayerState;

	void SetPlayerName(const FString& Name);

	void SetBestTime(double Time);

	void SetLastTime(double Time);

	void SetWins(int32 Count);

protected:

	UPROPERTY(meta=(BindWidget))
	UTextBlock* TextBlock_PlayerName;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TextBlock_BestTime;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TextBlock_LastTime;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TextBlock_Wins;
};
