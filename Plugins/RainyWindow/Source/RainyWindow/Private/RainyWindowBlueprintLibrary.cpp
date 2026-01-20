// Copyright Epic Games, Inc. All Rights Reserved.

#include "RainyWindowBlueprintLibrary.h"
#include "RainyWindowSceneViewExtension.h"

void URainyWindowBlueprintLibrary::SetRainEnabled(bool bEnabled)
{
	FRainyWindowSceneViewExtension::SetRainEnabled(bEnabled);
}

void URainyWindowBlueprintLibrary::SetRainAmount(float Amount)
{
	FRainyWindowSceneViewExtension::SetRainAmount(Amount);
}

void URainyWindowBlueprintLibrary::ResetRainToDefault()
{
	FRainyWindowSceneViewExtension::ResetToDefault();
}

void URainyWindowBlueprintLibrary::SetCameraZoomEnabled(bool bEnabled)
{
	FRainyWindowSceneViewExtension::SetCameraZoomEnabled(bEnabled);
}

bool URainyWindowBlueprintLibrary::IsCameraZoomEnabled()
{
	return FRainyWindowSceneViewExtension::IsCameraZoomEnabled();
}


bool URainyWindowBlueprintLibrary::IsRainEnabled()

{
	return FRainyWindowSceneViewExtension::IsRainEnabled();
}

float URainyWindowBlueprintLibrary::GetRainAmount()
{
	return FRainyWindowSceneViewExtension::GetRainAmount();
}