// Fill out your copyright notice in the Description page of Project Settings.


#include "MovementDemo/MDPlayerState.h"
#include "MovementDemo/MDCharacter.h"
#include "MovementDemo/MDGameModeBase.h"
#include "MovementDemo/MDGameStateBase.h"
#include "MovementDemo/MDHUD.h"
#include "Logging/StructuredLog.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

AMDPlayerState::AMDPlayerState()
{
}

void AMDPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, BestCompletedTime, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, LastCompletedTime, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Wins, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, bHasReachedGoal, Params);
}

void AMDPlayerState::OnRep_PlayerName()
{
	Super::OnRep_PlayerName();

	if (auto* LocalHUD = AMDHUD::GetLocalHUD(GetWorld()))
	{
		LocalHUD->SetNameForPlayer(this);
	}

	// Update name on the widget on top of the player
	if(auto* MDCharacter = GetPawn<AMDCharacter>())
	{
		MDCharacter->SetNameWidgetText(GetPlayerName());
	}
}

void AMDPlayerState::SetPlayerName(const FString& S)
{
	Super::SetPlayerName(S);

	// Call OnRep manually on server
	//OnRep_PlayerName();
}

void AMDPlayerState::SetGoalIsReached()
{
	if(HasAuthority() && !bHasReachedGoal)
	{
		bHasReachedGoal = true;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, bHasReachedGoal, this);

		// Call OnRep manually on server
		OnRep_HasReachedGoal();

		// Check if we have new best time
		auto* World = GetWorld();
		auto* GS = World->GetGameState<AMDGameStateBase>();
		check(IsValid(GS));
		double MatchStartTime = GS->GetStartingShotServerTime();
		double TimeCompleted = World->TimeSince(MatchStartTime);

		if(BestCompletedTime < 0.0 || TimeCompleted < BestCompletedTime)
		{
			SetBestCompletedTime(TimeCompleted);
		}

		// Always set last completed time
		SetLastCompletedTime(TimeCompleted);

		// Start new match
		auto* GM = World->GetAuthGameMode<AMDGameModeBase>();
		check(IsValid(GM));
		// Try award win to player if we were the first one to complete
		GM->TryGiveWinToPlayer(this);

		if(GM->GetMatchState() == EMDMatchState::OnGoing)
		{
			GM->SetMatchState(EMDMatchState::WaitingMatchReset);
		}
	}
}

void AMDPlayerState::ResetHasReachedGoal()
{
	if(HasAuthority() && bHasReachedGoal)
	{
		bHasReachedGoal = false;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, bHasReachedGoal, this);
		ForceNetUpdate();

		// Call OnRep manually on server
		OnRep_HasReachedGoal();
	}
}

void AMDPlayerState::SetWinCount(int32 NewCount)
{
	if(HasAuthority())
	{
		Wins = NewCount;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Wins, this);

		// Call OnRep manually on server
		OnRep_Wins();
	}
}

void AMDPlayerState::BeginPlay()
{
	Super::BeginPlay();
	if(auto* LocalHUD = AMDHUD::GetLocalHUD(GetWorld()))
	{
		// No need to check if local state, widget will check it anyway
		LocalHUD->CreateOrRefreshWidgetForRemotePlayer(this);
	}
}

void AMDPlayerState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (auto* LocalHUD = AMDHUD::GetLocalHUD(GetWorld()))
	{
		// No need to check if local state, widget will check it anyway
		LocalHUD->RemoveWidgetForRemotePlayer(this);
	}
}

void AMDPlayerState::OnRep_BestCompletedTime()
{
	if(auto* LocalHUD = AMDHUD::GetLocalHUD(GetWorld()))
	{
		LocalHUD->SetBestTimeForPlayer(this);
	}
}

void AMDPlayerState::OnRep_LastCompletedTime()
{
	if (auto* LocalHUD = AMDHUD::GetLocalHUD(GetWorld()))
	{
		LocalHUD->SetLastTimeForPlayer(this);
	}
}

void AMDPlayerState::OnRep_Wins()
{
	if (auto* LocalHUD = AMDHUD::GetLocalHUD(GetWorld()))
	{
		LocalHUD->SetWinsForPlayer(this);
	}
}

void AMDPlayerState::OnRep_HasReachedGoal()
{
	if (auto* LocalHUD = AMDHUD::GetLocalHUD(GetWorld()))
	{
		LocalHUD->SetPlayerReachedGoal(this);
	}
}

void AMDPlayerState::SetBestCompletedTime(double NewTime)
{
	if (HasAuthority())
	{
		BestCompletedTime = NewTime;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, BestCompletedTime, this);
		ForceNetUpdate();
		OnRep_BestCompletedTime();
	}
}

void AMDPlayerState::SetLastCompletedTime(double NewTime)
{
	if(HasAuthority())
	{
		LastCompletedTime = NewTime;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, LastCompletedTime, this);
		ForceNetUpdate();
		OnRep_LastCompletedTime();
	}
}