// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MDPlayerController.generated.h"


UCLASS()
class MOVEMENTDEMO_API AMDPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	AMDPlayerController();

	virtual void OnRep_PlayerState() override;

protected:

	virtual void EndPlayingState() override;
};
