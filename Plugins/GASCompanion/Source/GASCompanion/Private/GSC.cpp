// Copyright 2021 Mickael Daniel. All Rights Reserved.

#include "GSC.h"

#include "AbilitySystemGlobals.h"
#include "GSCAssetManager.h"
#include "GSCLog.h"
#include "Abilities/Attributes/GSCAttributeSet.h"
#include "Core/Settings/GSCDeveloperSettings.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#if WITH_EDITOR
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#endif

#define LOCTEXT_NAMESPACE "FGSCModule"

void FGSCModule::StartupModule()
{
	GSC_LOG(Display, TEXT("Startup GASCompanionModule Module"));

	// Register post engine delegate to handle init Ability System Global data initialization
	FCoreDelegates::OnPostEngineInit.AddStatic(&FGSCModule::OnPostEngineInit);
}

void FGSCModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	GSC_LOG(Display, TEXT("Shutdown GSC module"));

	// Remove delegates
	FCoreDelegates::OnPostEngineInit.RemoveAll(this);
}

FString FGSCModule::GetDocumentationURL(FString InPath)
{
	static FString DocumentationURL = "https://gascompanion.github.io";
	// static FString DocumentationURL = "https://gascompanion-dev.netlify.app";
	if (InPath.IsEmpty())
	{
		return DocumentationURL;
	}

	if (!InPath.StartsWith("/"))
	{
		InPath = "/" + InPath;
	}

	return DocumentationURL + InPath;
}

void FGSCModule::OnPostEngineInit()
{
	// On engine start, set up the HideInDetailsView metadata for GSCAttributeSet class, depending on saved config
	HandleAttributesDeveloperSettings();

	GSC_PLOG(Verbose, TEXT("UGSCDeveloperSettings FName: %s"), *UGSCDeveloperSettings::StaticClass()->GetFName().ToString())
	GSC_PLOG(Verbose, TEXT("UGSCDeveloperSettings SectionName: %s"), *UGSCDeveloperSettings::Get().GetSectionName().ToString())
}

void FGSCModule::HandleAttributesDeveloperSettings()
{
#if WITH_EDITOR
	const bool bHideInDetailsView = UGSCDeveloperSettings::Get().bHideGSCAttributeSetInDetailsView;
	if (bHideInDetailsView)
	{
		// If enabled, ensure on engine start the HideInDetailsView metadata is added. If set to false (the default),
		// leave it as is, metadata on load should be without.
		UGSCDeveloperSettings::SetHideInDetailsViewMetaData(UGSCAttributeSet::StaticClass(), true);
	}
#endif
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGSCModule, GASCompanion)
