// Copyright Epic Games, Inc. All Rights Reserved.

#include "RainyWindow.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

#define LOCTEXT_NAMESPACE "FRainyWindowModule"

void FRainyWindowModule::StartupModule()
{
	// ×¢²áshaderÄ¿Â¼
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("RainyWindow"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugins/RainyWindow"), PluginShaderDir);
}

void FRainyWindowModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FRainyWindowModule, RainyWindow)