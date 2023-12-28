// Fill out your copyright notice in the Description page of Project Settings.


#include "MovementDemo/MDGameModeBase.h"
#include "MovementDemo/MDGameStateBase.h"
#include "MovementDemo/MDPlayerState.h"
#include "MovementDemo/MDCharacterMovementComponent.h"
#include "EngineUtils.h"
#include "Engine/PlayerStartPIE.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/StructuredLog.h"

AMDGameModeBase::AMDGameModeBase()
{
}

void AMDGameModeBase::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// Start new match when new player joins
	BeginMatchReset();

	// Reset wins when new player joins
	ResetWins();
}

AActor* AMDGameModeBase::ChoosePlayerStart_Implementation(AController* Player)
{
	APlayerStart* FoundPlayerStart = nullptr;
	UClass* PawnClass = GetDefaultPawnClassForController(Player);
	APawn* PawnToFit = PawnClass ? PawnClass->GetDefaultObject<APawn>() : nullptr;
	TArray<APlayerStart*> UnOccupiedStartPoints;
	TArray<APlayerStart*> OccupiedStartPoints;
	UWorld* World = GetWorld();

	// Since we have disable collision between players, default implementation will spawn players top of each other and we need to change the logic
	// We will loops trough all existing players and check that our starting spot has minimum distance to other players
	// This should always work, if we have more starting spots than players
	TArray<FVector> ExistingPlayerLocations;

	for(const APawn* Pawn : TActorRange<APawn>(World, PawnClass))
	{
		ExistingPlayerLocations.Add(Pawn->GetActorLocation());
	}

	//If pawn has capsule minimum distance will be capsule radius + a bit more, otherwise we will use hard coded value
	double MinDistanceSquared = FMath::Square(100.0);

	if(const auto* Capsule = PawnToFit->GetComponentByClass<UCapsuleComponent>())
	{
		MinDistanceSquared = FMath::Square(Capsule->GetScaledCapsuleRadius() + 5.0);
	}

	for(auto* PlayerStart : TActorRange<APlayerStart>(World))
	{
		if (PlayerStart->IsA<APlayerStartPIE>())
		{
			// Always prefer the first "Play from Here" PlayerStart, if we find one while in PIE mode
			FoundPlayerStart = PlayerStart;
			break;
		}

		FVector StartLocation = PlayerStart->GetActorLocation();
		FRotator StartRotation = PlayerStart->GetActorRotation();

		bool bShouldSkipThisPlayerStart = false;
		for(const auto& NoGoLocation : ExistingPlayerLocations)
		{
			// We will use horizontal distance here, might not work for all games, but works for our game
			if (FVector::DistSquaredXY(StartLocation, NoGoLocation) < MinDistanceSquared)
			{
				bShouldSkipThisPlayerStart = true;
				break;
			}
		}

		if(bShouldSkipThisPlayerStart)
		{
			continue;
		}

		// Do some additional collision checks copy pasted from Super
		if (!World->EncroachingBlockingGeometry(PawnToFit, StartLocation, StartRotation))
		{
			UnOccupiedStartPoints.Add(PlayerStart);
		}
		else if (World->FindTeleportSpot(PawnToFit, StartLocation, StartRotation))
		{
			OccupiedStartPoints.Add(PlayerStart);
		}
	}

	if (FoundPlayerStart == nullptr)
	{
		if (UnOccupiedStartPoints.Num() > 0)
		{
			FoundPlayerStart = UnOccupiedStartPoints[FMath::RandRange(0, UnOccupiedStartPoints.Num() - 1)];
		}
		else if (OccupiedStartPoints.Num() > 0)
		{
			FoundPlayerStart = OccupiedStartPoints[FMath::RandRange(0, OccupiedStartPoints.Num() - 1)];
		}
	}
	return FoundPlayerStart;
}

void AMDGameModeBase::RestartPlayerAtPlayerStart(AController* NewPlayer, AActor* StartSpot)
{
	if (NewPlayer == nullptr || NewPlayer->IsPendingKillPending())
	{
		return;
	}

	if (!StartSpot)
	{
		UE_LOG(LogGameMode, Warning, TEXT("RestartPlayerAtPlayerStart: Player start not found"));
		return;
	}

	FRotator SpawnRotation = StartSpot->GetActorRotation();

	UE_LOG(LogGameMode, Verbose, TEXT("RestartPlayerAtPlayerStart %s"), (NewPlayer && NewPlayer->PlayerState) ? *NewPlayer->PlayerState->GetPlayerName() : TEXT("Unknown"));

	if (MustSpectate(Cast<APlayerController>(NewPlayer)))
	{
		UE_LOG(LogGameMode, Verbose, TEXT("RestartPlayerAtPlayerStart: Tried to restart a spectator-only player!"));
		return;
	}

	if (auto* ExistingPawn = NewPlayer->GetPawn())
	{
		// If we have an existing pawn, reset the location and rotation
		ExistingPawn->TeleportTo(StartSpot->GetActorLocation(), StartSpot->GetActorRotation());
	}
	else if (GetDefaultPawnClassForController(NewPlayer) != nullptr)
	{
		// Try to create a pawn to use of the default class for this player
		APawn* NewPawn = SpawnDefaultPawnFor(NewPlayer, StartSpot);
		if (IsValid(NewPawn))
		{
			NewPlayer->SetPawn(NewPawn);
		}
	}

	if (!IsValid(NewPlayer->GetPawn()))
	{
		FailedToRestartPlayer(NewPlayer);
	}
	else
	{
		// Tell the start spot it was used
		InitStartSpot(StartSpot, NewPlayer);

		FinishRestartPlayer(NewPlayer, SpawnRotation);
	}
}

void AMDGameModeBase::SetMatchState(EMDMatchState NewState)
{
	MatchState = NewState;

	switch (MatchState)
	{
		case EMDMatchState::WaitingMatchReset:
		{
			StartCountdownToMatchReset();
			break;
		}
		case EMDMatchState::WaitingMatchStartingShot:
		{
			StartCountdownToStartingShot();
			break;
		}
		case EMDMatchState::OnGoing:
		{
			OnStartingShot();
			break;
		}
		default:
			break;
	}
}

void AMDGameModeBase::TryGiveWinToPlayer(AMDPlayerState* PlayerState)
{
	if (!bHasWinBeenAssignedThisMatch)
	{
		bHasWinBeenAssignedThisMatch = true;
		PlayerState->SetWinCount(PlayerState->GetWinCount() + 1);
	}
}

FString AMDGameModeBase::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal)
{
	check(NewPlayerController);

	// The player needs a PlayerState to register successfully
	if (NewPlayerController->PlayerState == nullptr)
	{
		return FString(TEXT("PlayerState is null"));
	}

	// Register the player with the session
	GameSession->RegisterPlayer(NewPlayerController, UniqueId, UGameplayStatics::HasOption(Options, TEXT("bIsFromInvite")));

	// Find a starting spot
	FString ErrorMessage;
	if (!UpdatePlayerStartSpot(NewPlayerController, Portal, ErrorMessage))
	{
		UE_LOG(LogGameMode, Warning, TEXT("InitNewPlayer: %s"), *ErrorMessage);
	}

	// Set up spectating
	bool bSpectator = FCString::Stricmp(*UGameplayStatics::ParseOption(Options, TEXT("SpectatorOnly")), TEXT("1")) == 0;
	if (bSpectator || MustSpectate(NewPlayerController))
	{
		NewPlayerController->StartSpectatingOnly();
	}

	// Init player's name
	FString InName = FString::Format(TEXT("Player{0}"), { NextPlayerNumber });
	++NextPlayerNumber;

	ChangeName(NewPlayerController, InName, false);

	return ErrorMessage;
}

void AMDGameModeBase::BeginMatchReset()
{
	SetMatchState(EMDMatchState::WaitingMatchReset);
}

void AMDGameModeBase::StartCountdownToMatchReset()
{
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindWeakLambda(this, [this]()
		{
			SetMatchState(EMDMatchState::WaitingMatchStartingShot);
		});
	GetWorldTimerManager().SetTimer(TimerHandle_MatchReset, TimerDelegate, MatchResetDelay, false);
}

void AMDGameModeBase::StartCountdownToStartingShot()
{
	for(auto& Player: GameState->PlayerArray)
	{
		auto* MDPLayerState = CastChecked<AMDPlayerState>(Player);
		MDPLayerState->ResetHasReachedGoal();
		auto* PC = Player->GetPlayerController();
		RestartPlayer(PC);
		RootPlayer(PC);
	}

	auto* GS = GetGameState<AMDGameStateBase>();
	GS->SetStartingShotTime(GetWorld()->TimeSeconds + StartingShotDelay);

	FTimerDelegate TimerDelegate;
	TimerDelegate.BindWeakLambda(this, [this]()
	{
		SetMatchState(EMDMatchState::OnGoing);
	});
	GetWorldTimerManager().SetTimer(TimerHandle_StartingShot, TimerDelegate, StartingShotDelay, false);
}

void AMDGameModeBase::OnStartingShot()
{
	for (auto& Player : GameState->PlayerArray)
	{
		auto* PC = Player->GetPlayerController();
		UnRootPlayer(PC);
	}

	bHasWinBeenAssignedThisMatch = false;
}

void AMDGameModeBase::RootPlayer(APlayerController* PC)
{
	if(auto* Character = PC->GetCharacter())
	{
		auto* MoveComp = Character->GetCharacterMovement();
		MoveComp->SetMovementMode(MOVE_Custom, MDMOVE_Rooted);
		Character->ForceNetUpdate();
	}
}

void AMDGameModeBase::UnRootPlayer(APlayerController* PC)
{
	if (auto* Character = PC->GetCharacter())
	{
		auto* MoveComp = Character->GetCharacterMovement();
		MoveComp->SetMovementMode(MOVE_Walking);
		Character->ForceNetUpdate();
	}
}

void AMDGameModeBase::ResetWins()
{
	for (auto& Player : GameState->PlayerArray)
	{
		auto* MDPLayerState = CastChecked<AMDPlayerState>(Player);
		MDPLayerState->SetWinCount(0);
	}
}
