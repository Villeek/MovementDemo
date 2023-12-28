// Fill out your copyright notice in the Description page of Project Settings.


#include "MovementDemo/MDPlayerController.h"
#include "MovementDemo/MDHUD.h"
#include "MovementDemo/MDCharacter.h"

AMDPlayerController::AMDPlayerController()
{
	NetUpdateFrequency = 30.0f;
}

void AMDPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if(auto* MDHUD = GetHUD<AMDHUD>())
	{
		MDHUD->CreateWidgetsForLocalPlayer();
	}
}

void AMDPlayerController::EndPlayingState()
{
	Super::EndPlayingState();
	if(auto* MDCharacter = Cast<AMDCharacter>(GetPawn()))
	{
		MDCharacter->SetRemoteViewYaw(0.0f);
	}
}
