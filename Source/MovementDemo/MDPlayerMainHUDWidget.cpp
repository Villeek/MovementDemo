// Fill out your copyright notice in the Description page of Project Settings.


#include "MovementDemo/MDPlayerMainHUDWidget.h"
#include "MovementDemo/MDPlayerState.h"
#include "MovementDemo/MDRemotePlayerHUDWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Kismet/KismetTextLibrary.h"

UMDPlayerMainHUDWidget::UMDPlayerMainHUDWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UMDPlayerMainHUDWidget::SetLocalPlayerName(const FString& Name)
{
	if(ensure(TextBlock_PlayerName))
	{
		TextBlock_PlayerName->SetText(FText::FromString(Name));
	}
}

void UMDPlayerMainHUDWidget::SetCurrentMatchTimeText(double Time)
{
	if(ensure(TextBlock_CurrentMatchTime))
	{
		FText AsText = UKismetTextLibrary::Conv_DoubleToText(Time, HalfToEven, false, true, 1, 10, 3, 3);
		TextBlock_CurrentMatchTime->SetText(AsText);
	}
}

void UMDPlayerMainHUDWidget::SetLocalBestTime(double Time)
{
	if(ensure(TextBlock_BestTime))
	{
		FText AsText = UKismetTextLibrary::Conv_DoubleToText(Time, HalfToEven, false, true, 1, 10, 3, 3);
		TextBlock_BestTime->SetText(AsText);
	}
}

void UMDPlayerMainHUDWidget::SetLocalLastTime(double Time)
{
	if(ensure(TextBlock_LastTime))
	{
		FText AsText = UKismetTextLibrary::Conv_DoubleToText(Time, HalfToEven, false, true, 1, 10, 3, 3);
		TextBlock_LastTime->SetText(AsText);
	}
}

void UMDPlayerMainHUDWidget::SetLocalWins(int32 Count)
{
	if(ensure(TextBlock_Wins))
	{
		TextBlock_Wins->SetText(FText::AsNumber(Count));
	}
}

void UMDPlayerMainHUDWidget::CreateOrRefreshWidgetForRemotePlayer(AMDPlayerState* PlayerState)
{
	if (ensure(VerticalBox_RemotePlayers))
	{
		auto RemoteWidgets = VerticalBox_RemotePlayers->GetAllChildren();

		UMDRemotePlayerHUDWidget* Widget = nullptr;

		// If it exists, grab it
		if (const auto* PtrToPtr = RemoteWidgets.FindByPredicate([PlayerState](const UWidget* W) { return CastChecked<UMDRemotePlayerHUDWidget>(W)->OwningPlayerState.Get() == PlayerState; }))
		{
			Widget = CastChecked<UMDRemotePlayerHUDWidget>(*PtrToPtr);
		}
		// Doesn't exist, need to create new
		else
		{
			if (ensure(RemotePlayerWidgetClass))
			{
				Widget = CreateWidget<UMDRemotePlayerHUDWidget>(GetOwningPlayer(), RemotePlayerWidgetClass);
				Widget->OwningPlayerState = PlayerState;
				check(!RemoteWidgets.Contains(Widget));
				VerticalBox_RemotePlayers->AddChildToVerticalBox(Widget);
			}
		}
		// At this point, we should have a valid widget
		if (ensure(IsValid(Widget)))
		{
			Widget->SetPlayerName(PlayerState->GetPlayerName());
			Widget->SetBestTime(PlayerState->GetBestCompletedTime());
			Widget->SetLastTime(PlayerState->GetLastCompletedTime());
			Widget->SetWins(PlayerState->GetWinCount());
		}
	}
}

void UMDPlayerMainHUDWidget::DestroyWidgetForRemotePlayer(AMDPlayerState* PlayerState)
{
	auto RemoteWidgets = VerticalBox_RemotePlayers->GetAllChildren();

	if (const auto* PtrToPtr = RemoteWidgets.FindByPredicate([PlayerState](const UWidget* W){ return CastChecked<UMDRemotePlayerHUDWidget>(W)->OwningPlayerState.Get() == PlayerState; }))
	{
		auto* Widget = CastChecked<UMDRemotePlayerHUDWidget>(*PtrToPtr);
		VerticalBox_RemotePlayers->RemoveChild(Widget);
		Widget->RemoveFromParent();
	}
}
