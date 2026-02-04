// Copyright 2020 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeCategories.h"
#include "AddToProjectConfig.h"
#include "PropertyEditorModule.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class IAssetTypeActions;
class SNotificationItem;
struct FToolMenuSection;

class FGASCompanionEditorModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	EAssetTypeCategories::Type GetAssetTypeCategory() const;

	/**
	* Singleton-like access to this module's interface.  This is just for convenience!
	* Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	*
	* @return Returns singleton instance, loading the module on demand if needed
	*/
	static FGASCompanionEditorModule& Get() {
		return FModuleManager::LoadModuleChecked<FGASCompanionEditorModule>("GASCompanionEditor");
	}

	/**
	* Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	*
	* @return True if the module is loaded and ready to use
	*/
	static bool IsAvailable() {
		return FModuleManager::Get().IsModuleLoaded("GASCompanionEditor");
	}

private:
	/** The notification toast that a check-in is recommended */
	TWeakPtr<SNotificationItem> AssetManagerNotification;

	static constexpr const TCHAR* SettingsContainerName = TEXT("Project");
	static constexpr const TCHAR* SettingsCategoryName = TEXT("GAS Companion");
	static constexpr const TCHAR* SettingsGameplayEffectsSectionName = TEXT("Effects Definitions");
	static constexpr const TCHAR* SettingsGameplayAbilitiesSectionName = TEXT("Abilities Definitions");

	TSharedPtr<FUICommandList> PluginCommands;
	EAssetTypeCategories::Type AssetTypeCategory = EAssetTypeCategories::None;
	FDelegateHandle MapChangedHandle;

	/** All created asset type actions.  Cached here so that we can unregister them during shutdown. */
	TArray<TSharedPtr<IAssetTypeActions>> RegisteredAssetTypeActions;
	/** All registered customization. Cached here so that we can unregister during shutdown */
    TArray<FName> RegisteredClassCustomizations;

	void HandleAssetManagerDeprecationMessage();
	void UpdateAssetManagerClass() const;
	void OnOpenEngineSettingsClicked() const;
	void OnNotificationDismissClicked() const;

	void RegisterMenus();
	void AddComboButtonToMenu(FToolMenuSection& InMenuSection) const;
	static void RegisterComboMenus();
	static void BuildMenuSections(UToolMenu* InMenu);

	static void RegisterCommands();
	static void UnregisterCommands();
	static void InitializeStyles();
	static void ShutdownStyle();

	template <typename TDetailsCustomization>
	void RegisterDetailsCustomization(const FName ClassToCustomize, FPropertyEditorModule& PropertyModule)
	{
		PropertyModule.RegisterCustomClassLayout(ClassToCustomize, FOnGetDetailCustomizationInstance::CreateStatic(&TDetailsCustomization::MakeInstance));
		RegisteredClassCustomizations.Add(ClassToCustomize);
	}

	void UnregisterDetailsCustomization(FPropertyEditorModule& PropertyEditorModule);

	/**
	* Generates menu content for the toolbar combo button drop down menu
	*
	* @return	Menu content widget
	*/
	static TSharedRef<SWidget> GenerateToolbarSettingsMenu(TSharedRef<FUICommandList> InCommandList);
};
