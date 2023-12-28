// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MDCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UMDCheckpointTrackerComponent;
class UInputMappingContext;
class UInputAction;
class UAnimMontage;
class UWidgetComponent;
struct FInputActionValue;

/**
 * MDCharacter is playable character that has functionality for sliding, wall running and mantling.
 * All movements are fully networked, client predicted and server authoritative.
 * Replication rates, send rates etc are limited to 30hz. See DefaultGame.ini and DefaultEngine.ini.
 */
UCLASS()
class MOVEMENTDEMO_API AMDCharacter : public ACharacter
{
	GENERATED_BODY()

public:

	AMDCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void PawnClientRestart() override;

	virtual void PossessedBy(AController* NewController) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;

	virtual void PreReplicationForReplay(IRepChangedPropertyTracker& ChangedPropertyTracker) override;

	virtual void OnRep_PlayerState() override;

	virtual FRotator GetBaseAimRotation() const override;

	virtual void Jump() override;

	void SetNameWidgetText(const FString& NewName);

	void SetRemoteViewYaw(float NewRemoteViewYaw);

	void SetIsSliding(bool bSlide);

	bool IsSliding() const { return bIsSliding; }

	void SetIsMantling(bool bMantle);

	bool IsMantling() const { return bIsMantling; }

	UFUNCTION(Server, Reliable)
	void ServerDoMantleRPC();

	UMDCheckpointTrackerComponent* GetCheckpointTrackerComp() const { return CheckpointTrackerComp; }

protected:

	UPROPERTY(VisibleAnywhere)
	USpringArmComponent* SpringArmComp;

	UPROPERTY(VisibleAnywhere)
	UCameraComponent* CameraComp;

	UPROPERTY(VisibleAnywhere)
	UMDCheckpointTrackerComponent* CheckpointTrackerComp;

	UPROPERTY(VisibleAnywhere)
	UWidgetComponent* NameWidgetComp;

	UPROPERTY(EditDefaultsOnly, Category = "MovementDemo|Input")
	UInputMappingContext* InputMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "MovementDemo|Mantle")
	UAnimMontage* Montage_Mantle;

	UPROPERTY(Replicated)
	uint32 bIsSliding:1;

	UPROPERTY(ReplicatedUsing = OnRep_IsMantling)
	uint32 bIsMantling:1;

	UPROPERTY(Replicated)
	uint8 RemoteViewYaw;

	/*
	 * These could live in a config (UDataAsset) class, but since we won't have too many for the demo, just put them here.
	 * We use separate for forward/backward, so we can bind them separately and they properly cancel each other. Pressing both at the same time adds to 0.
	 * float Direction simply means 1 == forward, -1 == backwards
	 */ 

	UPROPERTY(EditDefaultsOnly, Category = "MovementDemo|Input")
	UInputAction* IA_MoveForward;

	UPROPERTY(EditDefaultsOnly, Category = "MovementDemo|Input")
	UInputAction* IA_MoveBackward;

	UPROPERTY(EditDefaultsOnly, Category = "MovementDemo|Input")
	UInputAction* IA_MoveRight;

	UPROPERTY(EditDefaultsOnly, Category = "MovementDemo|Input")
	UInputAction* IA_MoveLeft;

	UPROPERTY(EditDefaultsOnly, Category = "MovementDemo|Input")
	UInputAction* IA_Turn;

	UPROPERTY(EditDefaultsOnly, Category = "MovementDemo|Input")
	UInputAction* IA_LookUp;

	UPROPERTY(EditDefaultsOnly, Category = "MovementDemo|Input")
	UInputAction* IA_Sprint;

	UPROPERTY(EditDefaultsOnly, Category = "MovementDemo|Input")
	UInputAction* IA_Jump;

	UPROPERTY(EditDefaultsOnly, Category = "MovementDemo|Input")
	UInputAction* IA_Crouch;

	UPROPERTY(EditDefaultsOnly, Category = "MovementDemo|Input")
	UInputAction* IA_Slide;

	virtual void BeginPlay() override;

	void MoveForward(const FInputActionValue& Value, float Direction);

	void MoveRight(const FInputActionValue& Value, float Direction);

	void Turn(const FInputActionValue& Value);

	void LookUp(const FInputActionValue& Value);

	void Sprint_Pressed();

	void Sprint_Released();

	void Jump_Pressed();

	void Jump_Released();

	void Crouch_Pressed();

	void Crouch_Released();

	void Slide_Pressed();

	void Slide_Released();

	UFUNCTION()
	void OnRep_IsMantling();
};
