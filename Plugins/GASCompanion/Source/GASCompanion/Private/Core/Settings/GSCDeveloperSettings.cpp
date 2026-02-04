// Copyright 2021-2024 Mickael Daniel. All Rights Reserved.

#include "Core/Settings/GSCDeveloperSettings.h"

#include "AttributeSet.h"
#include "GSCLog.h"
#include "Abilities/Attributes/GSCAttributeSet.h"
#include "UObject/Class.h"

UGSCDeveloperSettings::UGSCDeveloperSettings()
{
}

const UGSCDeveloperSettings& UGSCDeveloperSettings::Get()
{
	const UGSCDeveloperSettings* Settings = GetDefault<UGSCDeveloperSettings>();
	check(Settings);
	return *Settings;
}

UGSCDeveloperSettings& UGSCDeveloperSettings::GetMutable()
{
	UGSCDeveloperSettings* Settings = GetMutableDefault<UGSCDeveloperSettings>();
	check(Settings);
	return *Settings;
}

FName UGSCDeveloperSettings::GetCategoryName() const
{
	return PluginCategoryName;
}

#if WITH_EDITOR
FText UGSCDeveloperSettings::GetSectionText() const
{
	return NSLOCTEXT("GASCompanion", "GASCompanionSettings", "GAS Companion Settings");
}

void UGSCDeveloperSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	static const FName HideInDetailsViewPropertyName = GET_MEMBER_NAME_CHECKED(UGSCDeveloperSettings, bHideGSCAttributeSetInDetailsView);
	const FName PropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == HideInDetailsViewPropertyName)
	{
		SetHideInDetailsViewMetaData(UGSCAttributeSet::StaticClass(), bHideGSCAttributeSetInDetailsView);
	}
}

void UGSCDeveloperSettings::SetHideInDetailsViewMetaData(UClass* InClass, const bool bInHideInDetailsView)
{
	check(InClass);
	checkf(InClass->IsChildOf(UAttributeSet::StaticClass()), TEXT("InClass %s is not a UAttributeSet class"), *InClass->GetName());

	static const FString Key = TEXT("HideInDetailsView");
	if (bInHideInDetailsView)
	{
		InClass->SetMetaData(*Key, TEXT("true"));
		GSC_PLOG(Verbose, TEXT("%s metadata has been set for %s class"), *Key, *InClass->GetName())
	}
	else
	{
		InClass->RemoveMetaData(*Key);
		GSC_PLOG(Verbose, TEXT("%s metadata has been removed for %s class"), *Key, *InClass->GetName())
	}
}
#endif
