// Fill out your copyright notice in the Description page of Project Settings.


#include "MovementDemo/MDCharacter.h"
#include "MovementDemo/MDCharacterMovementComponent.h"
#include "MovementDemo/MDCheckpointTrackerComponent.h"
#include "MovementDemo/MDPlayerNameWidget.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"


AMDCharacter::AMDCharacter(const FObjectInitializer& ObjectInitializer) :
Super(ObjectInitializer.SetDefaultSubobjectClass<UMDCharacterMovementComponent>(CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	NetUpdateFrequency = 30.0f;

	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>("SpringArmComp");
	SpringArmComp->SetupAttachment(GetRootComponent());
	SpringArmComp->TargetArmLength = 350.0f;
	SpringArmComp->bUsePawnControlRotation = true;

	CameraComp = CreateDefaultSubobject<UCameraComponent>("CameraComp");
	CameraComp->SetupAttachment(SpringArmComp, USpringArmComponent::SocketName);
	CameraComp->bUsePawnControlRotation = false;
	CameraComp->FieldOfView = 100.0f;

	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	CheckpointTrackerComp = CreateDefaultSubobject<UMDCheckpointTrackerComponent>("CheckpointTrackerComponent");

	NameWidgetComp = CreateDefaultSubobject<UWidgetComponent>("NameWidgetComponent");
	NameWidgetComp->SetupAttachment(GetMesh());
	NameWidgetComp->SetRelativeLocation(FVector(0.0, 0.0, 200.0));
}

void AMDCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	// Make sure we have controller and it's a player (not AI)
	auto* PlayerController = Cast<APlayerController>(GetController());
	if(PlayerController && PlayerController->IsLocalController())
	{
		if (auto* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			ensureAlways(InputMappingContext); // throw error/warning if input is missing
			if(InputMappingContext)
			{
				Subsystem->RemoveMappingContext(InputMappingContext);
				Subsystem->AddMappingContext(InputMappingContext, 0);
			}
		}

		// Hide our own name widget comp
		NameWidgetComp->SetHiddenInGame(true, true);
	}
}

void AMDCharacter::PossessedBy(AController* NewController)
{
	// We are not going to call for ACharacter method, so we have smoother animations on listen server
	APawn::PossessedBy(NewController);

	// We need to update the name on server (OnRep is not called)
	if(auto* PS = GetPlayerState())
	{
		SetNameWidgetText(PS->GetPlayerName());
	}
}

void AMDCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	auto* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);

	// Axes / movement
	if(ensure(IA_MoveForward))
	{
		EnhancedInputComponent->BindAction(IA_MoveForward, ETriggerEvent::Triggered, this, &ThisClass::MoveForward, 1.0f);
	}
	if(ensure(IA_MoveBackward))
	{
		EnhancedInputComponent->BindAction(IA_MoveBackward, ETriggerEvent::Triggered, this, &ThisClass::MoveForward, -1.0f);
	}
	if(ensure(IA_MoveRight))
	{
		EnhancedInputComponent->BindAction(IA_MoveRight, ETriggerEvent::Triggered, this, &ThisClass::MoveRight, 1.0f);
	}
	if(ensure(IA_MoveLeft))
	{
		EnhancedInputComponent->BindAction(IA_MoveLeft, ETriggerEvent::Triggered, this, &ThisClass::MoveRight, -1.0f);
	}
	if(ensure(IA_Turn))
	{
		EnhancedInputComponent->BindAction(IA_Turn, ETriggerEvent::Triggered, this, &ThisClass::Turn);
	}
	if(ensure(IA_LookUp))
	{
		EnhancedInputComponent->BindAction(IA_LookUp, ETriggerEvent::Triggered, this, &ThisClass::LookUp);
	}
	// ----------------------

	// Actions / etc
	if(ensure(IA_Sprint))
	{
		EnhancedInputComponent->BindAction(IA_Sprint, ETriggerEvent::Started, this, &ThisClass::Sprint_Pressed);
		EnhancedInputComponent->BindAction(IA_Sprint, ETriggerEvent::Completed, this, &ThisClass::Sprint_Released);
	}
	if(ensure(IA_Jump))
	{
		EnhancedInputComponent->BindAction(IA_Jump, ETriggerEvent::Started, this, &ThisClass::Jump_Pressed);
		EnhancedInputComponent->BindAction(IA_Jump, ETriggerEvent::Completed, this, &ThisClass::Jump_Released);
	}
	if(ensure(IA_Crouch))
	{
		EnhancedInputComponent->BindAction(IA_Crouch, ETriggerEvent::Started, this, &ThisClass::Crouch_Pressed);
		EnhancedInputComponent->BindAction(IA_Crouch, ETriggerEvent::Completed, this, &ThisClass::Crouch_Released);
	}
	if(ensure(IA_Slide))
	{
		EnhancedInputComponent->BindAction(IA_Slide, ETriggerEvent::Started, this, &ThisClass::Slide_Pressed);
		EnhancedInputComponent->BindAction(IA_Slide, ETriggerEvent::Completed, this, &ThisClass::Slide_Released);
	}
	// ----------------------
}

void AMDCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	Params.Condition = COND_SkipOwner;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, bIsSliding, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, bIsMantling, Params);

	// This updates every Replication
	DOREPLIFETIME_CONDITION(ThisClass, RemoteViewYaw, COND_SkipOwner);
}

void AMDCharacter::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

	if (GetLocalRole() == ROLE_Authority && GetController())
	{
		SetRemoteViewYaw(GetController()->GetControlRotation().Yaw);
	}
}

void AMDCharacter::PreReplicationForReplay(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	Super::PreReplicationForReplay(ChangedPropertyTracker);
	
	if (const UWorld* World = GetWorld())
	{
		// On client replays, our view yaw will be set to 0 as by default we do not replicate
		// yaw for owners, just for simulated. So instead push our rotation into the sampler
		if (World->IsRecordingClientReplay() && Controller != nullptr && GetLocalRole() == ROLE_AutonomousProxy && GetNetMode() == NM_Client)
		{
			SetRemoteViewYaw(Controller->GetControlRotation().Yaw);
		}
	}
}

void AMDCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	SetNameWidgetText(GetPlayerState()->GetPlayerName());
}

FRotator AMDCharacter::GetBaseAimRotation() const
{
	// If we have a controller, by default we aim at the player's 'eyes' direction
	FVector POVLoc;
	FRotator POVRot;
	if (Controller != nullptr && !InFreeCam())
	{
		Controller->GetPlayerViewPoint(POVLoc, POVRot);
		return POVRot;
	}

	// If we have no controller, we simply use our rotation
	POVRot = GetActorRotation();

	// If we are simulated, use replicated values
	if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		POVRot.Pitch = FRotator::DecompressAxisFromByte(RemoteViewPitch);
		POVRot.Yaw = FRotator::DecompressAxisFromByte(RemoteViewYaw);
	}

	return POVRot;
}

void AMDCharacter::Jump()
{
	// We need to keep this logic here, otherwise it gets really messy since default character movement doesn't want to jump if it's not on ground.
	// But we want to be able to mantle when not on ground.
	auto* MoveComp = CastChecked<UMDCharacterMovementComponent>(GetCharacterMovement());
	FMantleInfo MantleInfo;

	if (MoveComp->CanStartMantle())
	{
		MantleInfo = MoveComp->TryFindMantleLocation();
		if (MantleInfo.bCanMantle)
		{
			MoveComp->DoMantle(MantleInfo);
		}
	}

	if(!MantleInfo.bCanMantle)
	{
		Super::Jump();
	}
}

void AMDCharacter::SetNameWidgetText(const FString& NewName)
{
	// Need to make sure it's in proper state.
	NameWidgetComp->InitWidget();
	if (auto* Widget = Cast<UMDPlayerNameWidget>(NameWidgetComp->GetUserWidgetObject()))
	{
		Widget->SetNameText(NewName);
	}
}

void AMDCharacter::SetRemoteViewYaw(float NewRemoteViewYaw)
{
	// Compress yaw to 1 byte
	RemoteViewYaw = FRotator::CompressAxisToByte(NewRemoteViewYaw);
}

void AMDCharacter::SetIsSliding(bool bSlide)
{
	// Simulated should never call this...
	check(GetLocalRole() != ROLE_SimulatedProxy);

	bIsSliding = bSlide;
	if(HasAuthority())
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, bIsSliding, this);
	}
}

void AMDCharacter::SetIsMantling(bool bMantle)
{
	// Simulated should never call this...
	check(GetLocalRole() != ROLE_SimulatedProxy);
	bIsMantling = bMantle;
	if(HasAuthority())
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, bIsMantling, this);
	}
	// Call OnRep manually on local player and server, simulated proxies call it from replication
	OnRep_IsMantling();
}

void AMDCharacter::ServerDoMantleRPC_Implementation()
{
	// Server does Mantle calculations on server side, not trusting client
	auto* MoveComp = CastChecked<UMDCharacterMovementComponent>(GetCharacterMovement());
	const auto Info = MoveComp->TryFindMantleLocation();
	if (Info.bCanMantle)
	{
		MoveComp->DoMantle(Info);
	}
}

void AMDCharacter::BeginPlay()
{
	Super::BeginPlay();

}

void AMDCharacter::MoveForward(const FInputActionValue& Value, float Direction)
{
	if(Controller)
	{
		const float Scalar = Value.GetMagnitude() * Direction;
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(ForwardDirection, Scalar);
	}
}

void AMDCharacter::MoveRight(const FInputActionValue& Value, float Direction)
{
	if(Controller)
	{
		const float Scalar = Value.GetMagnitude() * Direction;
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(RightDirection, Scalar);
	}
}

void AMDCharacter::Turn(const FInputActionValue& Value)
{
	AddControllerYawInput(Value.GetMagnitude());
}

void AMDCharacter::LookUp(const FInputActionValue& Value)
{
	AddControllerPitchInput(Value.GetMagnitude() * -1.0f); // Invert it (can make this a setting later)
}

void AMDCharacter::Sprint_Pressed()
{
	CastChecked<UMDCharacterMovementComponent>(GetCharacterMovement())->SetWantsToSprint(true);
}

void AMDCharacter::Sprint_Released()
{
	CastChecked<UMDCharacterMovementComponent>(GetCharacterMovement())->SetWantsToSprint(false);
}

void AMDCharacter::Jump_Pressed()
{
	Jump();
}

void AMDCharacter::Jump_Released()
{
	StopJumping();
}

void AMDCharacter::Crouch_Pressed()
{
	Crouch();
}

void AMDCharacter::Crouch_Released()
{
	UnCrouch();
}

void AMDCharacter::Slide_Pressed()
{
	CastChecked<UMDCharacterMovementComponent>(GetCharacterMovement())->SetWantsToSlide(true);
}

void AMDCharacter::Slide_Released()
{
	CastChecked<UMDCharacterMovementComponent>(GetCharacterMovement())->SetWantsToSlide(false);
}

void AMDCharacter::OnRep_IsMantling()
{
	if(bIsMantling && Montage_Mantle)
	{
		PlayAnimMontage(Montage_Mantle);
	}
}