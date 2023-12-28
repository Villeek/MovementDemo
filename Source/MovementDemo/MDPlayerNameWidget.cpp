// Fill out your copyright notice in the Description page of Project Settings.


#include "MovementDemo/MDPlayerNameWidget.h"
#include "Components/TextBlock.h"

void UMDPlayerNameWidget::SetNameText(const FString& NewName)
{
	if(ensure(TextBlock_PlayerName))
	{
		TextBlock_PlayerName->SetText(FText::FromString(NewName));
	}
}
