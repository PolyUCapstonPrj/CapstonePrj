// Copyright 2021 Mickael Daniel. All Rights Reserved.

#include "GASCompanionEditorModule.h"

#include "AssetToolsModule.h"
#include "Core/Common/GSCAttributesWizardCommands.h"
#include "Core/Logging/GASCompanionEditorLog.h"
#include "Core/Settings/GSCDeveloperSettings.h"
#include "CreationMenu/GSCGameplayAbilityCreationMenu.h"
#include "CreationMenu/GSCGameplayEffectCreationMenu.h"
#include "CreationMenu/GSCGameplayEffectTemplateDetails.h"
#include "Engine/AssetManager.h"
#include "Framework/Notifications/NotificationManager.h"
#include "GSC.h"
#include "HAL/PlatformApplicationMisc.h"
#include "ISettingsModule.h"
#include "Interfaces/IMainFrameModule.h"
#include "LevelEditor.h"
#include "SourceCodeNavigation.h"
#include "ToolMenus.h"
#include "UI/Widgets/SGSCAttributeListReferenceViewer.h"
#include "UI/Widgets/SGSCNewAttributeSetClassDialog.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "GASCompanionEditor"

namespace UE::GASCompanionEditor::Private
{

	static TSharedPtr<SWindow> ReferenceViewerAttributesWindow;
	
	/**
	* Opens a dialog to add code files or blueprints to the current project.
	*
	* @param Config	Configuration options for the dialog
	*/
	static void OpenAddToProjectDialog(const FAddToProjectConfig& Config)
	{
		// If we've been given a class then we only show the second page of the dialog, so we can make the window smaller as that page doesn't have as much content
		const FVector2D WindowSize = FVector2D(940, 720);

		FText WindowTitle = Config._WindowTitle;
		if (WindowTitle.IsEmpty())
		{
			WindowTitle = LOCTEXT("AddCodeWindowHeader_Native", "Add AttributeSet C++ Class");
		}

		TSharedRef<SWindow> AddCodeWindow =
			SNew(SWindow)
			.Title(WindowTitle)
			.ClientSize(WindowSize)
			.SizingRule(ESizingRule::UserSized)
			.MinWidth(WindowSize.X)
			.MinHeight(WindowSize.Y)
			.SupportsMinimize(false)
			.SupportsMaximize(true);

		const TSharedRef<SGSCNewAttributeSetClassDialog> NewClassDialog =
			SNew(SGSCNewAttributeSetClassDialog)
			.ParentWindow(AddCodeWindow)
			.InitialPath(Config._InitialPath)
			.OnAddedToProject(Config._OnAddedToProject)
			.DefaultClassPrefix(Config._DefaultClassPrefix)
			.DefaultClassName(Config._DefaultClassName);

		AddCodeWindow->SetContent(NewClassDialog);

		TSharedPtr<SWindow> ParentWindow = Config._ParentWindow;
		if (!ParentWindow.IsValid())
		{
			static const FName MainFrameModuleName = "MainFrame";
			IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(MainFrameModuleName);
			ParentWindow = MainFrameModule.GetParentWindow();
		}

		if (Config._bModal)
		{
			FSlateApplication::Get().AddModalWindow(AddCodeWindow, ParentWindow);
		}
		else if (ParentWindow.IsValid())
		{
			FSlateApplication::Get().AddWindowAsNativeChild(AddCodeWindow, ParentWindow.ToSharedRef());
		}
		else
		{
			FSlateApplication::Get().AddWindow(AddCodeWindow);
		}
	}

	static void OpenSettings(const FName& ContainerName, const FName& CategoryName, const FName& SectionName)
	{
		FModuleManager::LoadModuleChecked<ISettingsModule>("Settings").ShowViewer(ContainerName, CategoryName, SectionName);
	}

	static void OpenClassWizard()
	{
		OpenAddToProjectDialog(FAddToProjectConfig());
	}

	static TSharedRef<SWindow> CreateReferenceViewerAttributes()
	{
		const FVector2D WindowSize(800, 800);

		const TSharedRef<SGSCAttributeListReferenceViewer> Widget = SNew(SGSCAttributeListReferenceViewer);
		const FText Title = LOCTEXT("AttributeListReferenceViewer_Title", "GAS Companion | Attribute Reference Viewer");

		const TSharedRef<SWindow> Window = SNew(SWindow)
			.Title(Title)
			.HasCloseButton(true)
			.SupportsMaximize(false)
			.SupportsMinimize(false)
			.AutoCenter(EAutoCenter::PreferredWorkArea)
			.ClientSize(WindowSize)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.FillHeight(1)
					[
						Widget
					]
				]
			];

		// NOTE: FGlobalTabmanager::Get()-> is actually dereferencing a SharedReference, not a SharedPtr, so it cannot be null.
		if (FGlobalTabmanager::Get()->GetRootWindow().IsValid())
		{
			FSlateApplication::Get().AddWindowAsNativeChild(Window, FGlobalTabmanager::Get()->GetRootWindow().ToSharedRef());
		}
		else
		{
			FSlateApplication::Get().AddWindow(Window);
		}

		// Set focus to the search box on creation
		FSlateApplication::Get().SetKeyboardFocus(Widget->GetWidgetToFocusOnOpen());
		FSlateApplication::Get().SetUserFocus(0, Widget->GetWidgetToFocusOnOpen());

		return Window;
	}
	
	static void OpenReferenceViewerAttributes()
	{
		GSC_EDITOR_PLOG(Verbose, TEXT("Opening it up"))
		
		if (ReferenceViewerAttributesWindow.IsValid())
		{
			ReferenceViewerAttributesWindow->BringToFront();
		}
		else
		{
			ReferenceViewerAttributesWindow = CreateReferenceViewerAttributes();
			ReferenceViewerAttributesWindow->SetOnWindowClosed(FOnWindowClosed::CreateLambda([](const TSharedRef<SWindow>& Window)
			{
				if (ReferenceViewerAttributesWindow.IsValid())
				{
					ReferenceViewerAttributesWindow.Reset();
				}
			}));
		}
	}

	static void OpenDocumentation()
	{
		const FString URL = FGSCModule::GetDocumentationURL();
		FPlatformProcess::LaunchURL(*URL, nullptr, nullptr);
	}

	static void OpenMailSupport()
	{
		const FString ClipboardString = TEXT("mklabs.unrealengine@gmail.com");
		FPlatformApplicationMisc::ClipboardCopy(*ClipboardString);

		const FText NotificationText = FText::Format(LOCTEXT("MailCopiedToClipboard", "{0} was copied to clipboard"), FText::FromString(ClipboardString));

		FNotificationInfo Info(NotificationText);
		// Info.bFireAndForget = true;
		Info.ExpireDuration = 5.f;
		Info.bUseLargeFont = false;
		Info.bUseThrobber = false;
		Info.bUseSuccessFailIcons = false;
		const TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
		if (Notification.IsValid())
		{
			Notification->SetCompletionState(SNotificationItem::CS_Success);
		}
	}

	static void OpenDiscord()
	{
		FPlatformProcess::LaunchURL(TEXT("https://discord.gg/d4rs4vcX6t"), nullptr, nullptr);
	}
}

void FGASCompanionEditorModule::StartupModule()
{
	InitializeStyles();
	RegisterCommands();

	GSC_EDITOR_LOG(Display, TEXT("GASCompanionEditorModule StartupModule"))

	// Create an editor interface, so the runtime module can access it

	PluginCommands = MakeShared<FUICommandList>();

	// Register the details customization
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		RegisterDetailsCustomization<FGSCGameplayEffectTemplateDetails>("GSCGameplayEffectTemplate", PropertyModule);
		PropertyModule.NotifyCustomizationModuleChanged();
	}

	// Register custom project settings
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings(
			SettingsContainerName,
			SettingsCategoryName,
			SettingsGameplayEffectsSectionName,
			FText::Format(LOCTEXT("Format", "Templates - {0}"), FText::FromName(SettingsGameplayEffectsSectionName)),
			LOCTEXT("GameplayEffectParentNameDesc", "Data Driven way of specifying common parent Gameplay Effect classes that are accessible through File menu"),
			GetMutableDefault<UGSCGameplayEffectCreationMenu>()
		);

		GetDefault<UGSCGameplayEffectCreationMenu>()->AddMenuExtensions();

		SettingsModule->RegisterSettings(
			SettingsContainerName,
			SettingsCategoryName,
			SettingsGameplayAbilitiesSectionName,
			FText::Format(LOCTEXT("Format", "Templates - {0}"), FText::FromName(SettingsGameplayAbilitiesSectionName)),
			LOCTEXT("GameplayAbilityParentNameDesc", "Data Driven way of specifying common parent Gameplay Ability classes that are accessible through File menu"),
			GetMutableDefault<UGSCGameplayAbilityCreationMenu>()
		);

		GetDefault<UGSCGameplayAbilityCreationMenu>()->AddMenuExtensions();
	}

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FGASCompanionEditorModule::RegisterMenus));

	HandleAssetManagerDeprecationMessage();
}

void FGASCompanionEditorModule::ShutdownModule()
{
	FCoreDelegates::OnPostEngineInit.RemoveAll(this);
	
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

	ShutdownStyle();
	UnregisterCommands();

	FLevelEditorModule& LevelEditor = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditor.OnMapChanged().Remove(MapChangedHandle);

	// Unregister property editor customizations
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		UnregisterDetailsCustomization(PropertyModule);
	}

	// Unregister settings
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
	if (SettingsModule)
	{
		SettingsModule->UnregisterSettings(SettingsContainerName, SettingsCategoryName, SettingsGameplayEffectsSectionName);
		SettingsModule->UnregisterSettings(SettingsContainerName, SettingsCategoryName, SettingsGameplayAbilitiesSectionName);
	}
}

void FGASCompanionEditorModule::RegisterCommands()
{
	FGSCAttributesWizardCommands::Register();
}

void FGASCompanionEditorModule::UnregisterCommands()
{
	FGSCAttributesWizardCommands::Unregister();
}

void FGASCompanionEditorModule::InitializeStyles()
{
	FGSCEditorStyle::Initialize();
	FGSCEditorStyle::ReloadTextures();
}

void FGASCompanionEditorModule::ShutdownStyle()
{
	FGSCEditorStyle::Shutdown();
}

EAssetTypeCategories::Type FGASCompanionEditorModule::GetAssetTypeCategory() const
{
	return AssetTypeCategory;
}

void FGASCompanionEditorModule::UnregisterDetailsCustomization(FPropertyEditorModule& PropertyEditorModule)
{
	for (const FName& ClassName : RegisteredClassCustomizations)
	{
		PropertyEditorModule.UnregisterCustomClassLayout(ClassName);
	}
	RegisteredClassCustomizations.Reset();
}

TSharedRef<SWidget> FGASCompanionEditorModule::GenerateToolbarSettingsMenu(TSharedRef<FUICommandList> InCommandList)
{
	// Get all menu extenders for this context menu from the level editor module
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
	const TSharedPtr<FExtender> MenuExtender = LevelEditorModule.AssembleExtenders(InCommandList, LevelEditorModule.GetAllLevelEditorToolbarViewMenuExtenders());

	const FToolMenuContext MenuContext(InCommandList, MenuExtender);
	return UToolMenus::Get()->GenerateWidget("LevelEditor.LevelEditorToolBar.GASCompanionEditor.ComboMenu", MenuContext);
}

void FGASCompanionEditorModule::HandleAssetManagerDeprecationMessage()
{
	const UEngine* EngineSettings = GetDefault<UEngine>();
	GSC_EDITOR_PLOG(Verbose, TEXT("Current AssetManagerClassName: %s"), *EngineSettings->AssetManagerClassName.ToString())

	const bool bIsUsingGSCAssetManager = EngineSettings->AssetManagerClassName.ToString() == TEXT("/Script/GASCompanion.GSCAssetManager");
	GSC_EDITOR_PLOG(Verbose, TEXT("bIsUsingGSCAssetManager: %s"), *LexToString(bIsUsingGSCAssetManager))

	if (!bIsUsingGSCAssetManager)
	{
		// Not using deprecated GSCAssetManager class, nothing to do
		return;
	}

	// In case project is using GSCAssetManager which is now deprecated, notify user it needs to be changed to either the default
	// Engine.AssetManager or a custom project class
	
	const FText NotificationText = LOCTEXT(
		"AssetManagerDeprecationMessage",
		"GSCAssetManager class has been deprecated in 6.2.0\n"
		"\n"
		"Your project is currently setup to use GSCAssetManager and you may experience a crash on project startup, when this class will be removed in a future update (planned for 5.6 release).\n"
		"\n"
		"Please edit your Config/DefaultEngine.ini file to remove or change the value of AssetManagerClassName below the [/Script/Engine.Engine] section.\n"
		"\n"
		"You can also do this in the editor by opening Project Settings > Engine > General Settings > Default Classes > Advanced > Asset Manager Class.\n"
		"\n"
		"Alternately, you can click the below link to reset AssetManagerClassName value to the default Engine.AssetManager value. Would you like to do that now?\n"
	);
	

	FNotificationInfo Info(LOCTEXT("NotificationTitle", "GAS Companion"));
	Info.SubText = NotificationText;
	Info.bFireAndForget = false;
	Info.ExpireDuration = 0.0f;
	Info.FadeOutDuration = 0.0f;
	Info.bUseThrobber = false;
	Info.WidthOverride = 720.f;

	// Info.Hyperlink = FSimpleDelegate::CreateStatic(&FGASCompanionEditorModule::UpdateAssetManagerClass);
	// Info.HyperlinkText = LOCTEXT("SettingsNotifLinkText", "Update Asset Manager Class to use Engine.AssetManager ?");

	Info.ButtonDetails.Add(FNotificationButtonInfo(
		LOCTEXT("AssetManagerNotification_Update", "Reset AssetManagerClassName to default value"),
		LOCTEXT("AssetManagerNotification_Update_Tooltip", "Update EngineSettings.AssetManagerClassName to use /Script/Engine.AssetManager"),
		FSimpleDelegate::CreateRaw(this, &FGASCompanionEditorModule::UpdateAssetManagerClass)
	));
	Info.ButtonDetails.Add(FNotificationButtonInfo(
		LOCTEXT("AssetManagerNotification_OpenSettings", "Open Settings"),
		LOCTEXT(
			"AssetManagerNotification_OpenSettings_Tooltip",
			"Open the Project Settings > General Settings where you can update AssetManagerClassName (below Default Classes > Advanced section)\n"
			"and dismiss this notification (notification will be displayed on next editor startup until AssetManagerClassName is changed)"
		),
		FSimpleDelegate::CreateRaw(this, &FGASCompanionEditorModule::OnOpenEngineSettingsClicked)
	));

	Info.ButtonDetails.Add(FNotificationButtonInfo(
		LOCTEXT("NewPluginsPopupDismiss", "Dismiss"),
		LOCTEXT("NewPluginsPopupDismissTT", "Dismiss this notification (notification will be displayed on next editor startup until AssetManagerClassName is changed)"),
		FSimpleDelegate::CreateRaw(this, &FGASCompanionEditorModule::OnNotificationDismissClicked)
	));
	
	AssetManagerNotification = FSlateNotificationManager::Get().AddNotification(Info);
	if (const TSharedPtr<SNotificationItem> AssetManagerNotificationPin = AssetManagerNotification.Pin())
	{
		AssetManagerNotificationPin->SetCompletionState(SNotificationItem::CS_Pending);
	}
}

void FGASCompanionEditorModule::UpdateAssetManagerClass() const
{
	UEngine* EngineSettings = GetMutableDefault<UEngine>();
	
	// Update engine settings for Asset Manager Class name and reset it back to default
	EngineSettings->AssetManagerClassName = UAssetManager::StaticClass();

	const FString DefaultConfigFilename = EngineSettings->GetDefaultConfigFilename();
	GSC_EDITOR_PLOG(Verbose, TEXT("Save settings to %s"), *DefaultConfigFilename)
	EngineSettings->UpdateSinglePropertyInConfigFile(EngineSettings->GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UEngine, AssetManagerClassName)), DefaultConfigFilename);

	// Notify user
	FNotificationInfo Info(LOCTEXT("NotificationTitle", "GAS Companion"));
	Info.SubText = LOCTEXT(
		"SettingsAssetManagerClassUpdated",
		"Asset Manager Class Name has been updated to use Engine.AssetManager.\n"
		"You may need to restart the editor for it to be active."
	);

	Info.ExpireDuration = 5.f;
	Info.bUseThrobber = false;
	Info.bUseLargeFont = false;
	Info.WidthOverride = 500.f;

	const TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
	if (Notification.IsValid())
	{
		Notification->SetCompletionState(SNotificationItem::CS_None);
	}

	// Fadeout previous notif
	OnNotificationDismissClicked();
}

void FGASCompanionEditorModule::OnOpenEngineSettingsClicked() const
{
	UE::GASCompanionEditor::Private::OpenSettings(TEXT("Project"), TEXT("Engine"), TEXT("General"));
	
	OnNotificationDismissClicked();
}

void FGASCompanionEditorModule::OnNotificationDismissClicked() const
{
	if (const TSharedPtr<SNotificationItem> AssetManagerNotificationPin = AssetManagerNotification.Pin())
	{
		AssetManagerNotificationPin->SetCompletionState(SNotificationItem::CS_None);
		AssetManagerNotificationPin->Fadeout();
	}
}

void FGASCompanionEditorModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);
	
	RegisterComboMenus();

	UToolMenu* MainMenu = UToolMenus::Get()->ExtendMenu("MainFrame.MainMenu.Tools");
	FToolMenuSection& Section = MainMenu->AddSection("GASCompanion", LOCTEXT("GASCompanion", "GASCompanion"));
	Section.AddSubMenu(
		"GASCompanionSubMenu",
		LOCTEXT("QuickSettingsCombo", "GAS Companion"),
		LOCTEXT("QuickSettingsCombo_ToolTip", "GAS Companion settings & tooling"),
		FNewToolMenuDelegate::CreateStatic(&FGASCompanionEditorModule::BuildMenuSections),
		false,
		FSlateIcon(FGSCEditorStyle::GetStyleSetName(), "GASCompanionEditor.OpenPluginWindow")
	);

	if (UGSCDeveloperSettings::Get().bEnableEditorToolbarButton)
	{
		UToolMenu* SettingsToolbar = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.User");
		AddComboButtonToMenu(SettingsToolbar->AddSection("GASCompanion"));
	}

	if (UGSCDeveloperSettings::Get().bEnableEditorStatusBarButton)
	{
		UToolMenu* StatusToolbar = UToolMenus::Get()->ExtendMenu("LevelEditor.StatusBar.Toolbar");
		AddComboButtonToMenu(StatusToolbar->AddSection("GASCompanion"));
	}
}

void FGASCompanionEditorModule::AddComboButtonToMenu(FToolMenuSection& InMenuSection) const
{
	FToolMenuEntry SettingsEntry = FToolMenuEntry::InitComboButton(
		"LevelToolbarGASCompanionSettings",
		FUIAction(),
		FOnGetContent::CreateStatic(&FGASCompanionEditorModule::GenerateToolbarSettingsMenu, PluginCommands.ToSharedRef()),
		LOCTEXT("QuickSettingsCombo", "GAS Companion"),
		LOCTEXT("QuickSettingsCombo_ToolTip", "GAS Companion settings & tooling"),
		FSlateIcon(FGSCEditorStyle::GetStyleSetName(), "GASCompanionEditor.OpenPluginWindow"),
		false
	);
	SettingsEntry.StyleNameOverride = "CalloutToolbar";
	InMenuSection.AddEntry(SettingsEntry);
}

void FGASCompanionEditorModule::RegisterComboMenus()
{
	UToolMenu* Menu = UToolMenus::Get()->RegisterMenu("LevelEditor.LevelEditorToolBar.GASCompanionEditor.ComboMenu");
	BuildMenuSections(Menu);
}

void FGASCompanionEditorModule::BuildMenuSections(UToolMenu* InMenu)
{
	check(InMenu);
	
	using namespace UE::GASCompanionEditor::Private;
	
	{
		FToolMenuSection& Section = InMenu->AddSection("ProjectSettingsSection", LOCTEXT("ProjectSettingsSection", "Configuration"));

		const UGSCDeveloperSettings& DeveloperSettingsAttributes = UGSCDeveloperSettings::Get();

		Section.AddMenuEntry(
			"GASCompanionSettings",
			LOCTEXT("ProjectSettingsMenuLabel", "GAS Companion Settings..."),
			LOCTEXT("ProjectSettingsMenuToolTip", "Change GAS Companion settings of the currently loaded project"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "ProjectSettings.TabIcon"),
			FUIAction(FExecuteAction::CreateLambda([&DeveloperSettingsAttributes]()
			{
				OpenSettings(DeveloperSettingsAttributes.GetContainerName(), DeveloperSettingsAttributes.GetCategoryName(), DeveloperSettingsAttributes.GetSectionName());
			}))
		);

		Section.AddMenuEntry(
			"GASCompanionGameplayAbilitySettings",
			LOCTEXT("ProjectGameplayAbilitiesSettingsMenuLabel", "Gameplay Ability Definitions..."),
			LOCTEXT("ProjectGameplayAbilitiesSettingsMenuToolTip", "Data Driven way of specifying common parent Gameplay Ability classes that are accessible through File menu"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "ProjectSettings.TabIcon"),
			FUIAction(FExecuteAction::CreateLambda([]() { OpenSettings(SettingsContainerName, SettingsCategoryName, SettingsGameplayAbilitiesSectionName); }))
		);

		Section.AddMenuEntry(
			"GASCompanionGameplayEffectsSettings",
			LOCTEXT("ProjectGameplayEffectsSettingsMenuLabel", "Gameplay Effect Definitions..."),
			LOCTEXT("ProjectGameplayEffectsSettingsMenuToolTip", "Data Driven way of specifying common parent Gameplay Effect classes that are accessible through File menu"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "ProjectSettings.TabIcon"),
			FUIAction(FExecuteAction::CreateLambda([]() { OpenSettings(SettingsContainerName, SettingsCategoryName, SettingsGameplayEffectsSectionName); }))
		);
	}

	{
		const FText ShortIDEName = FSourceCodeNavigation::GetSelectedSourceCodeIDE();
		FToolMenuSection& Section = InMenu->AddSection("AttributesSection", LOCTEXT("AttributesHeading", "Attributes"));
		Section.AddMenuEntry(
			"GASCompanionNewAttributeSetClass",
			LOCTEXT("AddAttributeSetClass", "New C++ AttributeSet Class..."),
			FText::Format(LOCTEXT("AddAttributeSetClassTooltip", "Adds C++ AttributeSet code to the project. The code can only be compiled if you have {0} installed."), ShortIDEName),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "MainFrame.AddCodeToProject"),
			FUIAction(FExecuteAction::CreateStatic(&OpenClassWizard))
		);

		Section.AddMenuEntry(
			"GASCompanionAttributeReferenceViewer",
			LOCTEXT("OpenReferenceViewerAttributes", "Open Reference Viewer for Attributes"),
			FText::Format(LOCTEXT("OpenReferenceViewerAttributesTooltip", "Open Reference Viewer for Attributes"), ShortIDEName),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "MainFrame.AddCodeToProject"),
			FUIAction(FExecuteAction::CreateStatic(&OpenReferenceViewerAttributes))
		);
	}

	{
		FToolMenuSection& Section = InMenu->AddSection("Documentation", LOCTEXT("DocumentationHeading", "Documentation"));
		Section.AddMenuEntry(
			"GASCompanionViewDocumentation",
			LOCTEXT("GASCompanionDocumentation", "GAS Companion Documentation"),
			LOCTEXT("GASCompanionDocumentationToolTip", "View online documentation"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.BrowseDocumentation"),
			FUIAction(FExecuteAction::CreateStatic(&OpenDocumentation))
		);
	}

	{
		FToolMenuSection& Section = InMenu->AddSection("Support", LOCTEXT("SupportHeading", "Support"));
		Section.AddMenuEntry(
			"GASCompanionDiscord",
			LOCTEXT("GASCompanionDiscord", "Discord Server"),
			LOCTEXT("GASCompanionDiscordToolTip", "Join the Discord Server for support or chat with the community of GAS Companion users"),
			FSlateIcon(FGSCEditorStyle::GetStyleSetName(), "Icons.Discord"),
			FUIAction(FExecuteAction::CreateStatic(&OpenDiscord))
		);

		Section.AddMenuEntry(
			"GASCompanionMailSupport",
			LOCTEXT("GASCompanionSupport", "Email support"),
			LOCTEXT("GASCompanionSupportToolTip", "Email the dev: mklabs.unrealengine@gmail.com"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Contact"),
			FUIAction(FExecuteAction::CreateStatic(&OpenMailSupport))
		);
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGASCompanionEditorModule, GASCompanionEditor)
