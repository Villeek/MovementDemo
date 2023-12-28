// Fill out your copyright notice in the Description page of Project Settings.


#include "MovementDemo/MDHUD.h"
#include "MovementDemo/MDPlayerMainHUDWidget.h"
#include "MovementDemo/MDGameStateBase.h"
#include "MovementDemo/MDPlayerState.h"
#include "Logging/StructuredLog.h"

AMDHUD::AMDHUD()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetActorTickInterval(1.0f / 30.0f); // Tick HUD 30 times a second
}

void AMDHUD::CreateWidgetsForLocalPlayer()
{
	// Already created
	if(MainWidget != nullptr)
	{
		return;
	}

	// Sanity check, probably not needed
	if (GetOwningPlayerController()->IsLocalController() && MainHUDWidgetClass)
	{
		MainWidget = CreateWidget<UMDPlayerMainHUDWidget>(GetOwningPlayerController(), MainHUDWidgetClass, FName("PlayerMainHUDWidget"));
		MainWidget->AddToViewport();

		if (auto* PS = GetOwningPlayerController()->GetPlayerState<AMDPlayerState>())
		{
			MainWidget->SetLocalPlayerName(PS->GetPlayerName());
		}

		SetActorTickEnabled(true);

		// Order of replication might not be what we want, so we will try to create widgets here and also on BeginPlay on PlayerState.
		TryCreateWidgetsForRemotePlayers();
	}
}

AMDHUD* AMDHUD::GetLocalHUD(const UWorld* World)
{
	for (auto It = World->GetPlayerControllerIterator(); It; ++It)
	{
		auto* PC = It->Get();
		if (PC->IsLocalController())
		{
			return PC->GetHUD<AMDHUD>();
		}
	}
	return nullptr;
}

void AMDHUD::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Something needs to tick and update time to hud, we might just do it here
	if(ensure(MainWidget))
	{
		auto* GS = GetWorld()->GetGameState<AMDGameStateBase>();
		auto StartTime = GS->GetStartingShotServerTime();
		if(StartTime <= 0.0)
		{
			return;
		}
		auto CurrentTime = GS->GetServerWorldTimeSeconds();

		// if StartTime > CurrentTime, it means we should count down (for launch), otherwise we should just keep counting time up.
		auto TimeToSet = StartTime > CurrentTime ? FMath::Max(0.0, StartTime - CurrentTime) : FMath::Max(0.0, CurrentTime - StartTime);
		MainWidget->SetCurrentMatchTimeText(TimeToSet);
	}
}

void AMDHUD::SetNameForPlayer(AMDPlayerState* PlayerState)
{
	if (!IsValid(MainWidget) || !IsValid(PlayerState))
	{
		return;
	}

	if (IsLocalPlayerState(PlayerState))
	{
		MainWidget->SetLocalPlayerName(PlayerState->GetPlayerName());
	}
	else
	{
		CreateOrRefreshWidgetForRemotePlayer(PlayerState);
	}
}

void AMDHUD::SetBestTimeForPlayer(AMDPlayerState* PlayerState)
{
	if (!IsValid(MainWidget) || !IsValid(PlayerState))
	{
		return;
	}

	if(IsLocalPlayerState(PlayerState))
	{
		MainWidget->SetLocalBestTime(PlayerState->GetBestCompletedTime());
	}
	else
	{
		CreateOrRefreshWidgetForRemotePlayer(PlayerState);
	}
}

void AMDHUD::SetLastTimeForPlayer(AMDPlayerState* PlayerState)
{
	if (!IsValid(MainWidget) || !IsValid(PlayerState))
	{
		return;
	}

	if (IsLocalPlayerState(PlayerState))
	{
		auto LastCompletedTime = PlayerState->GetLastCompletedTime();
		MainWidget->SetLocalLastTime(LastCompletedTime);
		// Stop the clock when reaching goal
		SetActorTickEnabled(false);
		MainWidget->SetCurrentMatchTimeText(LastCompletedTime);
	}
	else
	{
		CreateOrRefreshWidgetForRemotePlayer(PlayerState);
	}
}

void AMDHUD::SetWinsForPlayer(AMDPlayerState* PlayerState)
{
	if (!IsValid(MainWidget) || !IsValid(PlayerState))
	{
		return;
	}

	if (IsLocalPlayerState(PlayerState))
	{
		MainWidget->SetLocalWins(PlayerState->GetWinCount());
	}
	else
	{
		CreateOrRefreshWidgetForRemotePlayer(PlayerState);
	}
}

void AMDHUD::SetPlayerReachedGoal(AMDPlayerState* PlayerState)
{
	if (!IsValid(MainWidget) || !IsValid(PlayerState))
	{
		return;
	}

	if(IsLocalPlayerState(PlayerState))
	{
		if(!PlayerState->HasReachedGoal())
		{
			SetActorTickEnabled(true);
		}
	}
	else // We don't currently use this info on remote players, but we might just update the UI just in case, if we do use it in the future
	{
		CreateOrRefreshWidgetForRemotePlayer(PlayerState);
	}
}

void AMDHUD::CreateOrRefreshWidgetForRemotePlayer(AMDPlayerState* PlayerState)
{
	if (!IsValid(MainWidget) || !IsValid(PlayerState))
	{
		return;
	}

	if(IsLocalPlayerState(PlayerState))
	{
		return;
	}

	MainWidget->CreateOrRefreshWidgetForRemotePlayer(PlayerState);
}

void AMDHUD::RemoveWidgetForRemotePlayer(AMDPlayerState* PlayerState)
{
	if (!IsValid(MainWidget))
	{
		return;
	}

	if (IsLocalPlayerState(PlayerState))
	{
		return;
	}

	MainWidget->DestroyWidgetForRemotePlayer(PlayerState);
}

void AMDHUD::TryCreateWidgetsForRemotePlayers()
{
	auto* GS = GetWorld()->GetGameState();

	// GameState might not be valid at this point
	if (IsValid(GS))
	{
		for (auto& Player : GS->PlayerArray)
		{
			auto* MDPlayerState = Cast<AMDPlayerState>(Player);
			if (IsValid(MDPlayerState) && !IsLocalPlayerState(MDPlayerState))
			{
				CreateOrRefreshWidgetForRemotePlayer(MDPlayerState);
			}
		}
	}
}

void AMDHUD::BeginPlay()
{
	Super::BeginPlay();

	// Create Listen Server player widget here, we don't need to wait for replication
	// Clients will create theirs in AMDPlayerController::OnRep_PlayerState
	if(GetNetMode() < NM_Client && GetNetMode() != NM_DedicatedServer && GetOwningPlayerController()->IsLocalController())
	{
		CreateWidgetsForLocalPlayer();
	}
}

bool AMDHUD::IsLocalPlayerState(AMDPlayerState* PlayerState) const
{
	check(GetOwningPlayerController()->GetPlayerState<AMDPlayerState>() != nullptr);
	return GetOwningPlayerController()->GetPlayerState<AMDPlayerState>() == PlayerState;
}
