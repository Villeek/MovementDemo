// Fill out your copyright notice in the Description page of Project Settings.


#include "MovementDemo/MDGameStateBase.h"
#include "Logging/StructuredLog.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

AMDGameStateBase::AMDGameStateBase()
{
}

void AMDGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, StartingShotServerTime, Params);
}

void AMDGameStateBase::SetStartingShotTime(double Time)
{
	if(HasAuthority())
	{
		StartingShotServerTime = Time;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, StartingShotServerTime, this);
		ForceNetUpdate();

		// Call OnRep manually on Server
		OnRep_StartingShotServerTime();
	}
}

void AMDGameStateBase::OnRep_StartingShotServerTime()
{

}
