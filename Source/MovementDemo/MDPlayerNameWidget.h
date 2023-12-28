// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MDPlayerNameWidget.generated.h"

class UTextBlock;
/**
 * MDPlayerNameWidget is widget that is created on top of every player to show their name inside game world.
 * It is hidden for local player.
 */
UCLASS()
class MOVEMENTDEMO_API UMDPlayerNameWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	void SetNameText(const FString& NewName);

protected:

	UPROPERTY(meta=(BindWidget))
	UTextBlock* TextBlock_PlayerName;
};
