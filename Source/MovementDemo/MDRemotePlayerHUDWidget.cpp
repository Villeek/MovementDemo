// Fill out your copyright notice in the Description page of Project Settings.


#include "MovementDemo/MDRemotePlayerHUDWidget.h"
#include "Components/TextBlock.h"
#include "Kismet/KismetTextLibrary.h"

void UMDRemotePlayerHUDWidget::SetPlayerName(const FString& Name)
{
	if(ensure(TextBlock_PlayerName))
	{
		TextBlock_PlayerName->SetText(FText::FromString(Name));
	}
}

void UMDRemotePlayerHUDWidget::SetBestTime(double Time)
{
	if(ensure(TextBlock_BestTime))
	{
		if(Time > 0.0)
		{
			FText AsText = UKismetTextLibrary::Conv_DoubleToText(Time, HalfToEven, false, true, 1, 10, 3, 3);
			TextBlock_BestTime->SetText(AsText);
		}
		else
		{
			TextBlock_BestTime->SetText(FText::GetEmpty());
		}
	}
}

void UMDRemotePlayerHUDWidget::SetLastTime(double Time)
{
	if(ensure(TextBlock_LastTime))
	{
		if(Time > 0.0)
		{
			FText AsText = UKismetTextLibrary::Conv_DoubleToText(Time, HalfToEven, false, true, 1, 10, 3, 3);
			TextBlock_LastTime->SetText(AsText);
		}
		else
		{
			TextBlock_LastTime->SetText(FText::GetEmpty());
		}
	}
}

void UMDRemotePlayerHUDWidget::SetWins(int32 Count)
{
	if(ensure(TextBlock_Wins))
	{
		TextBlock_Wins->SetText(FText::AsNumber(Count));
	}
}
