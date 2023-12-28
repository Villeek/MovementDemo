// Fill out your copyright notice in the Description page of Project Settings.


#include "MovementDemo/MDCheckpoint.h"
#include "MovementDemo/MDCharacter.h"
#include "MovementDemo/MDCheckpointSubsystem.h"
#include "Components/BoxComponent.h"
#include "Logging/StructuredLog.h"

// Sets default values
AMDCheckpoint::AMDCheckpoint()
{
	PrimaryActorTick.bCanEverTick = false;

	RootMeshComp = CreateDefaultSubobject<UStaticMeshComponent>("RootMeshComp");
	SetRootComponent(RootMeshComp);

	TriggerComp = CreateDefaultSubobject<UBoxComponent>("TriggerComp");
	TriggerComp->SetupAttachment(GetRootComponent());
	TriggerComp->SetCollisionResponseToChannels(ECR_Ignore);
	TriggerComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AMDCheckpoint::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (GetNetMode() < NM_Client)
	{
		if (auto* CheckpointSubsystem = GetWorld()->GetSubsystem<UMDCheckpointSubsystem>())
		{
			CheckpointSubsystem->RegisterCheckpoint(*this);
		}
	}
}

void AMDCheckpoint::BeginPlay()
{
	Super::BeginPlay();

	if(GetNetMode() < NM_Client)
	{
		TriggerComp->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnTriggerOverlap);
	}
}

void AMDCheckpoint::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetNetMode() < NM_Client)
	{
		if (auto* CheckpointSubsystem = GetWorld()->GetSubsystem<UMDCheckpointSubsystem>())
		{
			CheckpointSubsystem->UnregisterCheckpoint(*this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void AMDCheckpoint::OnTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	check(GetNetMode() < NM_Client);

	if(const auto* MDCharacter = Cast<AMDCharacter>(OtherActor))
	{
		if (auto* CheckpointSubsystem = GetWorld()->GetSubsystem<UMDCheckpointSubsystem>())
		{
			if(auto* CheckpointComp = MDCharacter->GetCheckpointTrackerComp())
			{
				CheckpointSubsystem->OnTrackerOverlappedCheckpoint(*CheckpointComp, *this);
			}
		}
	}
}
