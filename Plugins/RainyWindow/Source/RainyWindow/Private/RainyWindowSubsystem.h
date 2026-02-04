// Copyright Epic Games, Inc. All Rights Reserved.
// Subsystem 用于管理 RainyWindow SceneViewExtension 的生命周期

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "RainyWindowSubsystem.generated.h"

UCLASS()
class URainyWindowSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	TSharedPtr<class FRainyWindowSceneViewExtension, ESPMode::ThreadSafe> RainyWindowSceneViewExtension;
};
