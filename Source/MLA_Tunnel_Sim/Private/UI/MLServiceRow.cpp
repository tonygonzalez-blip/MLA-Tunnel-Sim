// Copyright Micrologic Associates. All Rights Reserved.

#include "UI/MLServiceRow.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "MicrologicControllerSubsystem.h"
#include "UI/MLUiText.h"

void UMLServiceRow::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Edit)
	{
		Button_Edit->OnClicked.AddUniqueDynamic(this, &UMLServiceRow::HandleEditClicked);
	}
	if (Button_Send)
	{
		Button_Send->OnClicked.AddUniqueDynamic(this, &UMLServiceRow::HandleSendClicked);
	}
}

UMicrologicControllerSubsystem* UMLServiceRow::GetController() const
{
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UMicrologicControllerSubsystem>() : nullptr;
}

void UMLServiceRow::SetService(const FMLServiceConfig& InConfig)
{
	Config = InConfig;

	if (TextBlock_Number)
	{
		TextBlock_Number->SetText(FText::AsNumber(Config.ServiceNumber));
	}
	if (TextBlock_Description)
	{
		TextBlock_Description->SetText(FText::FromString(Config.Description));
	}
	if (TextBlock_Type)
	{
		const TArray<FString>& Options = MLUi::ServiceTypeOptions();
		const int32 TypeIndex = FMath::Clamp(static_cast<int32>(Config.Type), 0, Options.Num() - 1);
		TextBlock_Type->SetText(FText::FromString(Options[TypeIndex]));
	}
}

void UMLServiceRow::HandleEditClicked()
{
	OnEditRequested.Broadcast(Config.ServiceNumber);
}

void UMLServiceRow::HandleSendClicked()
{
	if (UMicrologicControllerSubsystem* Controller = GetController())
	{
		Controller->ExecuteService(Config.ServiceNumber);
	}
}
