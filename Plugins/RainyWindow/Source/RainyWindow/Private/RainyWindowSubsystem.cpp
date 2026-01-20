// Copyright Epic Games, Inc. All Rights Reserved.
// Subsystem 用于管理 RainyWindow SceneViewExtension 的生命周期

#include "RainyWindowSubsystem.h"
#include "RainyWindowSceneViewExtension.h"
#include "SceneViewExtension.h"

void URainyWindowSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	RainyWindowSceneViewExtension = FSceneViewExtensions::NewExtension<FRainyWindowSceneViewExtension>();
	UE_LOG(LogTemp, Log, TEXT("RainyWindow: Subsystem initialized & SceneViewExtension created"));
}

void URainyWindowSubsystem::Deinitialize()
{
	// 正确的清理方式：先禁用扩展
	{
		RainyWindowSceneViewExtension->IsActiveThisFrameFunctions.Empty();

		FSceneViewExtensionIsActiveFunctor IsActiveFunctor;
		IsActiveFunctor.IsActiveFunction = [](const ISceneViewExtension* SceneViewExtension, const FSceneViewExtensionContext& Context)
		{
			return TOptional<bool>(false);
		};

		RainyWindowSceneViewExtension->IsActiveThisFrameFunctions.Add(IsActiveFunctor);
	}

	RainyWindowSceneViewExtension.Reset();
	RainyWindowSceneViewExtension = nullptr;

	UE_LOG(LogTemp, Log, TEXT("RainyWindow: Subsystem deinitialized & SceneViewExtension destroyed"));
}