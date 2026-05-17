// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DirectionalLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/LocalLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInstance.h"
#include "ADayNightCycle.generated.h"

class UEmissiveConfigDataAsset;
class UTExture2D;
class APostProcessVolume;

USTRUCT(BlueprintType)
struct FLutKeyframe
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0",ClampMax = "24.0"))
	float Hour = 12.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UTexture2D> SourceLUT = nullptr;
};

/** Light on/off event delegate. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLightStateChanged, bool, bLightsOn);

/**
 * Wrapper struct for a group of emissive MIDs.
 * Used so we can keep nested arrays (per-MI groups of MIDs) under UPROPERTY
 * GC protection, since UHT does not support TArray<TArray<>> directly.
 */
USTRUCT()
struct FEmissiveMIDGroup
{
	GENERATED_BODY()

	/** All dynamic material instances created in the world that share the same source MI. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> MIDs;
};

UCLASS()
class CAPSTONEPRJ_API ADayNightCycle : public AActor
{
	GENERATED_BODY()

public:
	ADayNightCycle();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void Tick(float DeltaTime) override;

	// ========== External directional light reference ==========

	/** Directional light Actor in the world (rotation is driven by the level blueprint;
	 *  this class only reads its angle). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightController|Reference")
	TObjectPtr<ADirectionalLight> DirectionalLightActor;

	// ========== Thresholds ==========

	/** Sun pitch threshold: when pitch is greater than this value, it is considered night
	 *  and lights should turn on. Default 5 degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightController|Threshold", meta = (ClampMin = "-90.0", ClampMax = "90.0"))
	float NightPitchThreshold = 5.0f;

	/** Lights-off pitch threshold: lights turn off when pitch falls below this value
	 *  (i.e. once the sun has risen high enough). Default -5 degrees, 10 degrees lower
	 *  than the on-threshold to provide hysteresis and avoid flicker. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightController|Threshold", meta = (ClampMin = "-90.0", ClampMax = "90.0"))
	float LightsOffPitchThreshold = -5.0f;

	// ========== State queries ==========

	/** Returns true if it is currently night (based on the directional light pitch). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LightController")
	bool IsNight() const;

	/** Returns true if the lights should currently be on. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LightController")
	bool ShouldLightsBeOn() const;

	// ========== Events ==========

	/** Fired when lights need to turn on. */
	UPROPERTY(BlueprintAssignable, Category = "LightController|Events")
	FOnLightStateChanged OnLightsOn;

	/** Fired when lights need to turn off. */
	UPROPERTY(BlueprintAssignable, Category = "LightController|Events")
	FOnLightStateChanged OnLightsOff;

	// ========== Emissive activation config ==========

	/** Emissive config data asset for buildings (holds MI list and activation params;
	 *  uses TargetActorTag to find Actors in the world). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightController|Emissive")
	TObjectPtr<UEmissiveConfigDataAsset> EmissiveDataAsset;

	/** Emissive config data asset for street lamps (separated from the building emissive
	 *  so brightness/parameters can be configured independently; the actor source is decided
	 *  by StreetLampTag/StreetLampActors and TargetActorTag is NOT read). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightController|StreetLight|Emissive")
	TObjectPtr<UEmissiveConfigDataAsset> StreetLampEmissiveDataAsset;

	// ========== Street lamp / light control ==========

	/** Whether to enable street lamp light on/off control (works for any LocalLight:
	 *  RectLight / SpotLight / PointLight). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightController|StreetLight")
	bool bEnableStreetLampLightControl = true;

	/** Tag used to find street-lamp Actors. All LocalLightComponents on Actors with this
	 *  tag will be turned on/off. Default is "StreetLamp"; make sure your street-lamp
	 *  Actors have this tag. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightController|StreetLight")
	FName StreetLampTag = FName(TEXT("StreetLamp"));

	/** Optional: directly specify the Actor list to control (takes priority over Tag lookup). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightController|StreetLight")
	TArray<TObjectPtr<AActor>> StreetLampActors;

	// ========== Blueprint-callable functions ==========

	/** Manually force the lights on. */
	UFUNCTION(BlueprintCallable, Category = "LightController")
	void ForceLightsOn();

	/** Manually force the lights off. */
	UFUNCTION(BlueprintCallable, Category = "LightController")
	void ForceLightsOff();

	/** Get the current pitch of the directional light. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LightController")
	float GetSunPitch() const;

	/** Get the current count of valid MIDs. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LightController")
	int32 GetValidMIDCount() const;

	/** Get the current cached light count (all LocalLights: RectLight / SpotLight / PointLight). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LightController")
	int32 GetCachedStreetLampLightCount() const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	/** Editor-only: preview the emissive state. */
	void UpdateEmissivePreview();
#endif

private:
	/** Whether lights were on last frame, used to detect on/off transitions. */
	bool bWasLightsOn = false;

	/** Whether delayed initialization has finished. */
	bool bInitialized = false;

	/** Debug log timer. */
	float DebugTimer = 0.f;

	/** Watched-MI sampling timer (used by the watch-list diagnostic in Tick). */
	float WatchedMISampleTimer = 0.f;

	/** Delayed init function: waits for World Partition streaming to complete
	 *  before initializing the subsystems. */
	void DelayedInit();


	//===========LUT-based post process volume control (optional, not fully implemented) ==========
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightController|LUT")
	bool bEnableLutBlending = true;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightController|LUT")
	TArray<FLutKeyframe> LutKeyframes;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightController|LUT")
	TSoftObjectPtr<APostProcessVolume> TargetPostProcessVolume;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightController|LUT", meta = (ClampMin = "0.05", ClampMax = "10.0"))
	float LutUpdateInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightController|LUT", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LutIntensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightController|LUT", meta = (ClampMin = "0.0", ClampMax = "24.0"))
	float CurrentHour = 12.f;

	UFUNCTION(BlueprintCallable, Category = "LightController|LUT")
	void SetCurrentHour(float InHour);

private:

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> DynamicLUT = nullptr;

	float LutAccumTime = 0.f;


	float LutDebugTimer = 0.f;


	bool bLutFirstApplyLogged = false;

	void EnsureDynamicLUT();


	void UpdateBlendedLUT();


	void ApplyLUTToPostProcess();

	// ========== Emissive activation runtime data ==========

	/** Per-MI groups of dynamic material instances.
	 *  Outer index matches the order of EmissiveMIs in the data asset.
	 *  Inner array contains all MIDs created on Mesh slots in the world for that MI.
	 *  Marked UPROPERTY(Transient) so MIDs are not collected when World Partition unloads
	 *  or GC runs, which would otherwise produce dangling pointers. */
	UPROPERTY(Transient)
	TArray<FEmissiveMIDGroup> GroupedMIDs;

	/** Street lamp emissive: per-MI groups of dynamic material instances. */
	UPROPERTY(Transient)
	TArray<FEmissiveMIDGroup> StreetLampGroupedMIDs;

	/** Index of the currently activated MI group (matches EmissiveMIs in the data asset). */
	int32 CurrentMIGroupIndex = 0;

	/** Whether all emissive groups have finished activating. */
	bool bAllEmissiveActivated = false;

	/** Timer handle for emissive activation. */
	FTimerHandle EmissiveTimerHandle;

	/** Timer handle for street-lamp emissive activation. */
	FTimerHandle StreetLampEmissiveTimerHandle;

	/** Timer handle for delayed init (kept as a member so EndPlay can clear it). */
	FTimerHandle InitTimerHandle;

	/** Initialize the emissive system: collect mesh materials, create MIDs, group by MI. */
	void InitEmissiveSystem();

	/** Initialize the street-lamp emissive system: scans only street-lamp Actors
	 *  (StreetLampActors / StreetLampTag). */
	void InitStreetLampEmissiveSystem();

	/** Timer callback: activate all MIDs of the next MI group. */
	void ActivateNextEmissive();

	/** Timer callback: activate all MIDs of the next street-lamp MI group. */
	void ActivateNextStreetLampEmissive();

	/** Turn off all emissive MIDs (called when lights go off). */
	void DeactivateAllEmissive();

	/** Turn off all street-lamp emissive MIDs (called when lights go off). */
	void DeactivateAllStreetLampEmissive();

	/** Single source of truth: directly apply on/off emissive state to ALL cached MIDs
	 *  (both building and street-lamp groups). This is idempotent and avoids the
	 *  half-on/half-off intermediate states caused by the staggered timer-based
	 *  activation path. Called on day/night transitions in Tick. */
	void ApplyEmissiveState(bool bOn);

	// ========== Street lamp light runtime data ==========

	/** Cached light component references (using ULocalLightComponent base class so
	 *  RectLight / SpotLight / PointLight all work). */
	TArray<TWeakObjectPtr<ULocalLightComponent>> CachedStreetLampLights;

	/** Initialize the light system: collect all LocalLightComponents on Actors with the configured tag. */
	void InitStreetLampLightSystem();

	/** Collect street-lamp Actors (prefers StreetLampActors, then falls back to StreetLampTag). */
	void CollectStreetLampActors(TArray<AActor*>& OutActors) const;

	/** Turn on all street-lamp lights. */
	void ActivateStreetLampLights();

	/** Turn off all street-lamp lights. */
	void DeactivateStreetLampLights();
};
