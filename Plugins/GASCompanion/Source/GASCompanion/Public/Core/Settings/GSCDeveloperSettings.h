// Copyright 2021-2024 Mickael Daniel. All Rights Reserved.

#pragma once

#include "Engine/DeveloperSettings.h"
#include "GSCDeveloperSettings.generated.h"

class UAttributeSet;

/**
 * Developer Settings for GAS Companion, Attributes and AttributeSets related config.
 *
 * Upon changing any of the below configuration settings, the `Config/DefaultGame.ini` file will be saved to persist
 * this setting.
 */
UCLASS(config=Game, defaultconfig)
class GASCOMPANION_API UGSCDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/**
	 * Set this setting to true to hide GSCAttributeSet attributes from appearing in the Gameplay Attributes dropdown,
	 * namely in Gameplay Effects and K2 nodes like GetFloatAttribute() and their Attribute pin parameter.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Attributes", meta = (DisplayName = "Hide GSCAttributeSet Attributes"))
	bool bHideGSCAttributeSetInDetailsView = false;
	
	/**
	 * True if the GAS Companion module should add combo button and its drop-down menu in the level editor toolbar.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Editor", meta = (ConfigRestartRequired = true))
	bool bEnableEditorToolbarButton = true;
	
	/**
	 * True if the GAS Companion module should add combo button and its drop-down menu in the level editor's status bar at the bottom
	 */
	UPROPERTY(config, EditAnywhere, Category = "Editor", meta = (ConfigRestartRequired = true))
	bool bEnableEditorStatusBarButton = true;

	/** Default constructor */
	UGSCDeveloperSettings();

	static const UGSCDeveloperSettings& Get();
	static UGSCDeveloperSettings& GetMutable();

	/**
	 * The category name for our developer settings
	 *
	 * @see GetCategoryName
	 */
	static constexpr const TCHAR* PluginCategoryName = TEXT("GAS Companion");
	
	//~ Begin UDeveloperSettings interface
	virtual FName GetCategoryName() const override;
#if WITH_EDITOR
	virtual FText GetSectionText() const override;
	//~ End UDeveloperSettings interface
	
	//~ Begin UObject interface
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UObject interface

	/** Adds or remove `HideInDetailsView` class metadata to the passed in UClass. InClass must be a child of UAttributeSet */
	static void SetHideInDetailsViewMetaData(UClass* InClass, bool bInHideInDetailsView);
#endif
};
