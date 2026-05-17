// Fill out your copyright notice in the Description page of Project Settings.

#include "ADayNightCycle.h"
#include "UEmissiveConfigDataAsset.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/MeshComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/LocalLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"


namespace
{
	// ============ Watch list for diagnosing the flickering MI ============
	// Path of the MI we want to watch (the one user reported as flickering).
	static const TCHAR* GWatchedMIPath = TEXT("/Game/Building/New/uasset4k/uasset4k/materials/kb3d_cbp_atlasdgbannerb.KB3D_CBP_AtlasDGBannerB");

	// Returns true if the given MaterialInterface (MI or MID) is the watched MI
	// (or a MID whose Parent chain leads to the watched MI).
	static bool IsWatchedMI(const UMaterialInterface* Mat)
	{
		if (!Mat)
		{
			return false;
		}

		// Walk up the parent chain (handles MID->MIC).
		const UMaterialInterface* Cur = Mat;
		while (Cur)
		{
			const FString Path = Cur->GetPathName();
			if (Path == GWatchedMIPath)
			{
				return true;
			}
			if (const UMaterialInstance* MI = Cast<UMaterialInstance>(Cur))
			{
				Cur = MI->Parent;
			}
			else
			{
				break;
			}
		}
		return false;
	}

	static void BuildEmissiveGroupsByMeshScan(
		const TArray<AActor*>& FoundActors,
		UEmissiveConfigDataAsset* DataAsset,
		TArray<FEmissiveMIDGroup>& OutGroupedMIDs,
		float InitialValue,
		const TCHAR* LogPrefix)
	{
		OutGroupedMIDs.Empty();

		if (!DataAsset)
		{
			UE_LOG(LogTemp, Warning, TEXT("%s: DataAsset is not set"), LogPrefix);
			return;
		}

		const FName ParamName = DataAsset->EmissiveParameterName;

		TArray<UMaterialInstance*> LoadedMIs;
		LoadedMIs.Reserve(DataAsset->EmissiveMIs.Num());
		for (const TSoftObjectPtr<UMaterialInstance>& MISoft : DataAsset->EmissiveMIs)
		{
			UMaterialInstance* MI = MISoft.LoadSynchronous();
			if (!MI)
			{
				UE_LOG(LogTemp, Warning, TEXT("%s: failed to load MI: %s"), LogPrefix, *MISoft.ToString());
				continue;
			}
			LoadedMIs.Add(MI);
		}

		if (LoadedMIs.Num() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("%s: EmissiveMIs is empty or all entries failed to load"), LogPrefix);
			return;
		}

		TMap<UMaterialInterface*, int32> MIToGroupIndex;
		for (int32 Index = 0; Index < LoadedMIs.Num(); ++Index)
		{
			MIToGroupIndex.Add(LoadedMIs[Index], Index);
		}

		TArray<FEmissiveMIDGroup> TempGroups;
		TempGroups.SetNum(LoadedMIs.Num());

		for (AActor* Actor : FoundActors)
		{
			if (!IsValid(Actor))
			{
				continue;
			}

			TArray<UMeshComponent*> MeshComponents;
			Actor->GetComponents<UMeshComponent>(MeshComponents);

			for (UMeshComponent* MeshComp : MeshComponents)
			{
				if (!IsValid(MeshComp))
				{
					continue;
				}

				const int32 NumMaterials = MeshComp->GetNumMaterials();
				for (int32 SlotIndex = 0; SlotIndex < NumMaterials; ++SlotIndex)
				{
					UMaterialInterface* SlotMat = MeshComp->GetMaterial(SlotIndex);
					if (!SlotMat)
					{
						continue;
					}

					int32* GroupIndexPtr = MIToGroupIndex.Find(SlotMat);
					if (!GroupIndexPtr)
					{
						if (UMaterialInstanceDynamic* ExistingMID = Cast<UMaterialInstanceDynamic>(SlotMat))
						{
							GroupIndexPtr = MIToGroupIndex.Find(ExistingMID->Parent);
						}
					}
					if (!GroupIndexPtr)
					{
						continue;
					}

					UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(SlotMat);
					const bool bExistingMID = (MID != nullptr);
					if (!MID)
					{
						MID = MeshComp->CreateDynamicMaterialInstance(SlotIndex, LoadedMIs[*GroupIndexPtr]);
					}

					if (MID)
					{
						// Fix A: write the value that matches the current day/night state,
						// instead of unconditionally writing OffValue. This prevents a 1-frame
						// "forced off" right before Tick re-activates groups one by one,
						// which was a major source of the visible flicker.
						MID->SetScalarParameterValue(ParamName, InitialValue);
						TempGroups[*GroupIndexPtr].MIDs.Add(MID);

						// === Watched-MI diagnostic log ===
						if (IsWatchedMI(MID))
						{
							float CurVal = -1.f;
							MID->GetScalarParameterValue(ParamName, CurVal);
							UE_LOG(LogTemp, Warning,
								TEXT("[WatchMI] %s collected: Actor=%s, Mesh=%s, Slot=%d, MID=%p (Parent=%s), bExistingMID=%d, GroupIndex=%d, ParamName=%s, InitialValueWritten=%.3f, CurValAfterWrite=%.3f"),
								LogPrefix,
								*Actor->GetName(),
								*MeshComp->GetName(),
								SlotIndex,
								MID,
								MID->Parent ? *MID->Parent->GetName() : TEXT("<null>"),
								bExistingMID ? 1 : 0,
								*GroupIndexPtr,
								*ParamName.ToString(),
								InitialValue,
								CurVal);
						}
					}
				}
			}
		}

		int32 TotalMIDCount = 0;
		for (int32 Index = 0; Index < TempGroups.Num(); ++Index)
		{
			if (TempGroups[Index].MIDs.Num() > 0)
			{
				TotalMIDCount += TempGroups[Index].MIDs.Num();
				OutGroupedMIDs.Add(MoveTemp(TempGroups[Index]));
				UE_LOG(LogTemp, Log, TEXT("%s: MI [%s] matched %d Mesh Slot(s)"), LogPrefix, *LoadedMIs[Index]->GetName(), OutGroupedMIDs.Last().MIDs.Num());
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("%s: MI [%s] has no matching Mesh Slot in the world"), LogPrefix, *LoadedMIs[Index]->GetName());
			}
		}

		UE_LOG(LogTemp, Warning, TEXT("%s: init done, %d MI group(s), %d MID(s) total"), LogPrefix, OutGroupedMIDs.Num(), TotalMIDCount);
	}
}

ADayNightCycle::ADayNightCycle()
{
	PrimaryActorTick.bCanEverTick = true;

	// This class no longer owns a directional light component; it is just an empty Actor used as a light controller.
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ADayNightCycle::BeginPlay()
{
	Super::BeginPlay();

	// Delay initialization to wait for World Partition streaming to finish.
	GetWorldTimerManager().SetTimer(
		InitTimerHandle,
		this,
		&ADayNightCycle::DelayedInit,
		2.0f, // 2 seconds delay to wait for streaming
		false
	);
}

void ADayNightCycle::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Clear all timers to avoid callbacks after this Actor (or its MIDs) is destroyed.
	if (UWorld* World = GetWorld())
	{
		FTimerManager& TM = World->GetTimerManager();
		TM.ClearTimer(InitTimerHandle);
		TM.ClearTimer(EmissiveTimerHandle);
		TM.ClearTimer(StreetLampEmissiveTimerHandle);
		// Safety net: clear any remaining timers bound to this object.
		TM.ClearAllTimersForObject(this);
	}

	Super::EndPlay(EndPlayReason);
}

void ADayNightCycle::DelayedInit()
{
	// Fix D: decide the initial state using the HARD threshold (NightPitchThreshold),
	// bypassing the hysteresis logic in ShouldLightsBeOn(). Otherwise, when the sun
	// pitch lies in the hysteresis dead zone [LightsOffPitchThreshold, NightPitchThreshold]
	// at startup, the state we pick may disagree with what the scene's MICs actually
	// look like, causing a visible mismatch on the first frames.
	bWasLightsOn = (GetSunPitch() > NightPitchThreshold);

	// Initialize the emissive system. Both building and street-lamp init now write the
	// value that matches the current day/night state (Fix A), so MIDs come up consistent
	// with the rest of the scene without a transient "all off" frame.
	InitEmissiveSystem();
	InitStreetLampEmissiveSystem();

	// Initialize the StreetLampLight system.
	InitStreetLampLightSystem();

	// Fix E: apply the desired state to ALL MIDs in one shot through the single source
	// of truth, so we do not depend on the staggered ActivateNext... timers to converge.
	ApplyEmissiveState(bWasLightsOn);

	if (bWasLightsOn)
	{
		UE_LOG(LogTemp, Warning, TEXT("DayNightCycle: delayed init done, currently night, activating Emissive (BuildingMIGroups=%d, StreetLampMIGroups=%d, StreetLampLights=%d)"), GroupedMIDs.Num(), StreetLampGroupedMIDs.Num(), CachedStreetLampLights.Num());
		ActivateStreetLampLights();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("DayNightCycle: delayed init done, currently day (BuildingMIGroups=%d, StreetLampMIGroups=%d, StreetLampLights=%d)"), GroupedMIDs.Num(), StreetLampGroupedMIDs.Num(), CachedStreetLampLights.Num());
		DeactivateStreetLampLights();
	}

	// Mark all groups as already activated so the staggered timer path becomes a no-op;
	// the authoritative state has already been applied via ApplyEmissiveState above.
	CurrentMIGroupIndex = GroupedMIDs.Num();
	bAllEmissiveActivated = true;

	bInitialized = true;
}

void ADayNightCycle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bInitialized || !DirectionalLightActor)
	{
		return;
	}

	// Debug log: print current state every couple of seconds.
	DebugTimer += DeltaTime;
	if (DebugTimer > 2.0f)
	{
		FRotator Rot = DirectionalLightActor->GetActorRotation();
		UE_LOG(LogTemp, Warning, TEXT("DayNightCycle: Rotation=(P:%.1f, Y:%.1f, R:%.1f), GetSunPitch=%.1f, NightThreshold=%.1f, bWasLightsOn=%d, ShouldLightsBeOn=%d, BuildingMIGroups=%d, StreetLampMIGroups=%d"),
			Rot.Pitch, Rot.Yaw, Rot.Roll,
			GetSunPitch(), NightPitchThreshold, bWasLightsOn ? 1 : 0, ShouldLightsBeOn() ? 1 : 0, GroupedMIDs.Num(), StreetLampGroupedMIDs.Num());
		DebugTimer = 0.f;
	}

	// === Watched-MI sampling: every 1 second, read the current Emissive param value
	//     of all MIDs whose parent chain leads to the watched MI. If the value keeps
	//     flipping between samples, that is direct evidence of "flickering".
	WatchedMISampleTimer += DeltaTime;
	if (WatchedMISampleTimer >= 1.0f)
	{
		WatchedMISampleTimer = 0.f;

		auto SampleGroups = [this](const TArray<FEmissiveMIDGroup>& Groups, UEmissiveConfigDataAsset* DA, const TCHAR* Tag)
		{
			if (!DA)
			{
				return;
			}
			const FName ParamName = DA->EmissiveParameterName;
			int32 Idx = 0;
			for (const FEmissiveMIDGroup& Group : Groups)
			{
				for (UMaterialInstanceDynamic* MID : Group.MIDs)
				{
					if (!IsValid(MID))
					{
						continue;
					}
					if (!IsWatchedMI(MID))
					{
						continue;
					}
					float CurVal = -1.f;
					MID->GetScalarParameterValue(ParamName, CurVal);
					UE_LOG(LogTemp, Warning,
						TEXT("[WatchMI-Sample] %s [%d]: MID=%p, Parent=%s, Param=%s, CurVal=%.4f, bWasLightsOn=%d"),
						Tag, Idx, MID,
						MID->Parent ? *MID->Parent->GetName() : TEXT("<null>"),
						*ParamName.ToString(),
						CurVal,
						bWasLightsOn ? 1 : 0);
					Idx++;
				}
			}
		};

		SampleGroups(GroupedMIDs, EmissiveDataAsset, TEXT("Building"));
		SampleGroups(StreetLampGroupedMIDs, StreetLampEmissiveDataAsset, TEXT("StreetLamp"));
	}

	// Detect on/off transitions (based on directional light pitch).
	const bool bShouldLightsOn = ShouldLightsBeOn();
	if (!bWasLightsOn && bShouldLightsOn)
	{
		UE_LOG(LogTemp, Warning, TEXT("DayNightCycle: >>> lights ON triggered! Pitch=%.1f, BuildingMIGroups=%d, StreetLampMIGroups=%d, StreetLampLights=%d"), GetSunPitch(), GroupedMIDs.Num(), StreetLampGroupedMIDs.Num(), GetCachedStreetLampLightCount());
		bWasLightsOn = true;
		OnLightsOn.Broadcast(true);

		// MID cache lost (likely due to streaming): re-init the building/street-lamp emissive systems.
		if (GetValidMIDCount() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("DayNightCycle: MID cache is empty, re-initializing building/street-lamp emissive systems"));
			InitEmissiveSystem();
			InitStreetLampEmissiveSystem();
		}

		// Fix E: apply the ON state through the single source of truth so all MIDs
		// flip in the same frame, eliminating the half-on/half-off intermediate.
		ApplyEmissiveState(true);
		CurrentMIGroupIndex = GroupedMIDs.Num();
		bAllEmissiveActivated = true;

		ActivateStreetLampLights();
	}
	else if (bWasLightsOn && !bShouldLightsOn)
	{
		UE_LOG(LogTemp, Warning, TEXT("DayNightCycle: >>> lights OFF triggered! Pitch=%.1f"), GetSunPitch());
		bWasLightsOn = false;
		OnLightsOff.Broadcast(false);

		// Cancel any in-flight staggered activation timers and force the OFF state.
		GetWorldTimerManager().ClearTimer(EmissiveTimerHandle);
		GetWorldTimerManager().ClearTimer(StreetLampEmissiveTimerHandle);
		ApplyEmissiveState(false);
		CurrentMIGroupIndex = 0;
		bAllEmissiveActivated = false;

		DeactivateStreetLampLights();
	}

#if WITH_EDITOR
	LutDebugTimer += DeltaTime;
	const bool bShouldLogLut = (LutDebugTimer >= 5.f);
	if (bShouldLogLut)
	{
		LutDebugTimer = 0.f;
		APostProcessVolume* PPV_Debug = TargetPostProcessVolume.LoadSynchronous();
		UE_LOG(LogTemp, Warning,
			TEXT("[LUT-Diag] bEnable=%d, Keyframes=%d, CurrentHour=%.2f, PPV=%s, PPV.Enabled=%s, DynamicLUT=%s, Intensity=%.2f"),
			bEnableLutBlending ? 1 : 0,
			LutKeyframes.Num(),
			CurrentHour,
			PPV_Debug ? *PPV_Debug->GetName() : TEXT("<NULL>"),
			(PPV_Debug && PPV_Debug->bEnabled) ? TEXT("true") : TEXT("false"),
			DynamicLUT ? *DynamicLUT->GetName() : TEXT("<NULL>"),
			LutIntensity);

		if (!bEnableLutBlending)
		{
			UE_LOG(LogTemp, Warning, TEXT("[LUT-Diag] ❌ bEnableLutBlending=false，LUT 系统未启用！"));
		}
		else if (LutKeyframes.Num() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[LUT-Diag] ❌ LutKeyframes 为空！请在 Details 面板的 'LightController|LUT' 分类下添加关键帧。"));
		}
		else if (!PPV_Debug)
		{
			UE_LOG(LogTemp, Warning, TEXT("[LUT-Diag] ❌ TargetPostProcessVolume 未指定！请在 Details 面板中拖入场景里的 PostProcessVolume。"));
		}
		else if (!PPV_Debug->bEnabled)
		{
			UE_LOG(LogTemp, Warning, TEXT("[LUT-Diag] ⚠ PostProcessVolume.bEnabled=false！请在 PPV 的 Details 面板里勾选 'Enabled'。"));
		}
	}

	if (bEnableLutBlending && LutKeyframes.Num() > 0)
	{
		LutAccumTime += DeltaTime;
		if (LutAccumTime >= LutUpdateInterval)
		{
			LutAccumTime = 0.f;
			UpdateBlendedLUT();
			ApplyLUTToPostProcess();
		}
	}
#endif
}

int32 ADayNightCycle::GetValidMIDCount() const
{
	int32 Count = 0;
	for (const FEmissiveMIDGroup& Group : GroupedMIDs)
	{
		for (UMaterialInstanceDynamic* MID : Group.MIDs)
		{
			if (IsValid(MID))
			{
				Count++;
			}
		}
	}
	for (const FEmissiveMIDGroup& Group : StreetLampGroupedMIDs)
	{
		for (UMaterialInstanceDynamic* MID : Group.MIDs)
		{
			if (IsValid(MID))
			{
				Count++;
			}
		}
	}
	return Count;
}

float ADayNightCycle::GetSunPitch() const
{
	if (DirectionalLightActor)
	{
		// Get the directional light pitch.
		return DirectionalLightActor->GetActorRotation().Pitch;
	}
	return 0.f;
}

bool ADayNightCycle::IsNight() const
{
	// Positive pitch means the sun is below the horizon (night).
	// When Pitch > NightPitchThreshold (default 5) it is considered night.
	return GetSunPitch() > NightPitchThreshold;
}

bool ADayNightCycle::ShouldLightsBeOn() const
{
	// In this scene a positive pitch means the sun is below the horizon (night).
	// Use a hysteresis threshold to avoid flicker:
	//   On:  Pitch > NightPitchThreshold (default 5)
	//   Off: Pitch < LightsOffPitchThreshold (default -5)
	if (bWasLightsOn)
	{
		// Currently on: only turn off once the sun has risen above LightsOffPitchThreshold.
		return GetSunPitch() > LightsOffPitchThreshold;
	}
	else
	{
		// Currently off: only turn on once the sun has fallen below NightPitchThreshold.
		return GetSunPitch() > NightPitchThreshold;
	}
}

void ADayNightCycle::ForceLightsOn()
{
	if (!bWasLightsOn)
	{
		bWasLightsOn = true;
		OnLightsOn.Broadcast(true);
		if (GetValidMIDCount() == 0)
		{
			InitEmissiveSystem();
			InitStreetLampEmissiveSystem();
		}
		// Fix E: route through the single source of truth.
		ApplyEmissiveState(true);
		CurrentMIGroupIndex = GroupedMIDs.Num();
		bAllEmissiveActivated = true;
		ActivateStreetLampLights();
	}
}

void ADayNightCycle::ForceLightsOff()
{
	if (bWasLightsOn)
	{
		bWasLightsOn = false;
		OnLightsOff.Broadcast(false);
		GetWorldTimerManager().ClearTimer(EmissiveTimerHandle);
		GetWorldTimerManager().ClearTimer(StreetLampEmissiveTimerHandle);
		// Fix E: route through the single source of truth.
		ApplyEmissiveState(false);
		CurrentMIGroupIndex = 0;
		bAllEmissiveActivated = false;
		DeactivateStreetLampLights();
	}
}

#if WITH_EDITOR
void ADayNightCycle::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();

	// Refresh the editor preview when emissive-related properties change.
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ADayNightCycle, EmissiveDataAsset)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(ADayNightCycle, NightPitchThreshold)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(ADayNightCycle, LightsOffPitchThreshold)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(ADayNightCycle, DirectionalLightActor))
	{
		UpdateEmissivePreview();
	}
}

void ADayNightCycle::UpdateEmissivePreview()
{
	if (!EmissiveDataAsset)
	{
		return;
	}

	// Editor preview: decide whether lights should be on based on the current directional light angle.
	const bool bShouldBeOn = IsNight();
	const FName ParamName = EmissiveDataAsset->EmissiveParameterName;
	const float TargetValue = bShouldBeOn ? EmissiveDataAsset->EmissiveOnValue : EmissiveDataAsset->EmissiveOffValue;

	for (const TSoftObjectPtr<UMaterialInstance>& MISoft : EmissiveDataAsset->EmissiveMIs)
	{
		UMaterialInstance* MI = MISoft.LoadSynchronous();
		if (!MI)
		{
			continue;
		}

		UMaterialInstanceConstant* MIC = Cast<UMaterialInstanceConstant>(MI);
		if (MIC)
		{
			FMaterialParameterInfo ParamInfo(ParamName);
			MIC->SetScalarParameterValueEditorOnly(ParamInfo, TargetValue);
		}
	}
}
#endif

// ========== Emissive activation system ==========

void ADayNightCycle::InitEmissiveSystem()
{
	GroupedMIDs.Empty();
	CurrentMIGroupIndex = 0;
	bAllEmissiveActivated = false;

	if (!EmissiveDataAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("DayNightCycle: EmissiveDataAsset is not set!"));
		return;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), EmissiveDataAsset->TargetActorTag, FoundActors);

	UE_LOG(LogTemp, Log, TEXT("EmissiveAtlas(Building): looking for Actors with Tag='%s', found %d"),
		*EmissiveDataAsset->TargetActorTag.ToString(), FoundActors.Num());

	// Fix A: pick the initial value based on the current day/night state, so newly
	// created MIDs come up matching the rest of the scene rather than always being
	// forced to OffValue (which caused a 1-frame "all off" right before Tick re-
	// activated groups one by one).
	const float InitialValue = bWasLightsOn
		? EmissiveDataAsset->EmissiveOnValue
		: EmissiveDataAsset->EmissiveOffValue;
	BuildEmissiveGroupsByMeshScan(FoundActors, EmissiveDataAsset, GroupedMIDs, InitialValue, TEXT("EmissiveAtlas(Building)"));
}

void ADayNightCycle::InitStreetLampEmissiveSystem()
{
	StreetLampGroupedMIDs.Empty();

	if (!StreetLampEmissiveDataAsset)
	{
		UE_LOG(LogTemp, Log, TEXT("EmissiveAtlas(StreetLamp): StreetLampEmissiveDataAsset is not set, skipping init"));
		return;
	}

	TArray<AActor*> FoundActors;
	CollectStreetLampActors(FoundActors);

	UE_LOG(LogTemp, Log, TEXT("EmissiveAtlas(StreetLamp): collected %d street-lamp Actor(s)"), FoundActors.Num());

	// Fix A: same rationale as in InitEmissiveSystem.
	const float InitialValue = bWasLightsOn
		? StreetLampEmissiveDataAsset->EmissiveOnValue
		: StreetLampEmissiveDataAsset->EmissiveOffValue;
	BuildEmissiveGroupsByMeshScan(FoundActors, StreetLampEmissiveDataAsset, StreetLampGroupedMIDs, InitialValue, TEXT("EmissiveAtlas(StreetLamp)"));
}

// Fix E: the single source of truth for the emissive state. Every code path that
// changes the day/night emissive state should funnel through here, so we never end
// up with one set of MIDs at OnValue and another at OffValue at the same time.
void ADayNightCycle::ApplyEmissiveState(bool bOn)
{
	auto Apply = [bOn](const TArray<FEmissiveMIDGroup>& Groups, UEmissiveConfigDataAsset* DA, const TCHAR* Tag)
	{
		if (!DA)
		{
			return;
		}
		const FName ParamName = DA->EmissiveParameterName;
		const float Value = bOn ? DA->EmissiveOnValue : DA->EmissiveOffValue;

		int32 Applied = 0;
		int32 Total = 0;
		for (const FEmissiveMIDGroup& Group : Groups)
		{
			for (UMaterialInstanceDynamic* MID : Group.MIDs)
			{
				Total++;
				if (!IsValid(MID))
				{
					continue;
				}
				MID->SetScalarParameterValue(ParamName, Value);
				Applied++;

				if (IsWatchedMI(MID))
				{
					UE_LOG(LogTemp, Warning,
						TEXT("[WatchMI] %s.ApplyEmissiveState %s: MID=%p, Parent=%s, ValueWritten=%.3f"),
						Tag,
						bOn ? TEXT("ON") : TEXT("OFF"),
						MID,
						MID->Parent ? *MID->Parent->GetName() : TEXT("<null>"),
						Value);
				}
			}
		}

		UE_LOG(LogTemp, Log, TEXT("%s.ApplyEmissiveState %s: %d/%d MID(s) updated"),
			Tag, bOn ? TEXT("ON") : TEXT("OFF"), Applied, Total);
	};

	Apply(GroupedMIDs, EmissiveDataAsset, TEXT("Building"));
	Apply(StreetLampGroupedMIDs, StreetLampEmissiveDataAsset, TEXT("StreetLamp"));
}

void ADayNightCycle::ActivateNextEmissive()
{
	if (!EmissiveDataAsset || GroupedMIDs.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("DayNightCycle: ActivateNextEmissive failed - DataAsset=%s, MIGroups=%d"),
			EmissiveDataAsset ? TEXT("valid") : TEXT("null"), GroupedMIDs.Num());
		return;
	}

	if (bAllEmissiveActivated)
	{
		return;
	}

	if (CurrentMIGroupIndex < GroupedMIDs.Num())
	{
		// Activate all MIDs in the current MI group.
		const TArray<TObjectPtr<UMaterialInstanceDynamic>>& Group = GroupedMIDs[CurrentMIGroupIndex].MIDs;
		int32 ActivatedInGroup = 0;

		for (UMaterialInstanceDynamic* MID : Group)
		{
			if (IsValid(MID))
			{
				MID->SetScalarParameterValue(
					EmissiveDataAsset->EmissiveParameterName,
					EmissiveDataAsset->EmissiveOnValue
				);
				ActivatedInGroup++;

				// === Watched-MI diagnostic log ===
				if (IsWatchedMI(MID))
				{
					UE_LOG(LogTemp, Warning,
						TEXT("[WatchMI] Building.ActivateNextEmissive ON: MID=%p, Parent=%s, GroupIdx=%d/%d, OnValueWritten=%.3f"),
						MID,
						MID->Parent ? *MID->Parent->GetName() : TEXT("<null>"),
						CurrentMIGroupIndex, GroupedMIDs.Num(),
						EmissiveDataAsset->EmissiveOnValue);
				}
			}
		}

		UE_LOG(LogTemp, Log, TEXT("EmissiveAtlas: activated MI group %d/%d, %d MID(s) in this group"),
			CurrentMIGroupIndex + 1, GroupedMIDs.Num(), ActivatedInGroup);

		CurrentMIGroupIndex++;

		if (CurrentMIGroupIndex < GroupedMIDs.Num())
		{
			GetWorldTimerManager().SetTimer(
				EmissiveTimerHandle,
				this,
				&ADayNightCycle::ActivateNextEmissive,
				EmissiveDataAsset->ActivationInterval,
				false
			);
		}
		else
		{
			bAllEmissiveActivated = true;
			UE_LOG(LogTemp, Log, TEXT("EmissiveAtlas: all %d MI group(s) activated"), GroupedMIDs.Num());
		}
	}
}

void ADayNightCycle::ActivateNextStreetLampEmissive()
{
	if (!StreetLampEmissiveDataAsset || StreetLampGroupedMIDs.Num() == 0)
	{
		return;
	}

	for (const FEmissiveMIDGroup& Group : StreetLampGroupedMIDs)
	{
		for (UMaterialInstanceDynamic* MID : Group.MIDs)
		{
			if (IsValid(MID))
			{
				MID->SetScalarParameterValue(
					StreetLampEmissiveDataAsset->EmissiveParameterName,
					StreetLampEmissiveDataAsset->EmissiveOnValue
				);

				// === Watched-MI diagnostic log ===
				if (IsWatchedMI(MID))
				{
					UE_LOG(LogTemp, Warning,
						TEXT("[WatchMI] StreetLamp.ActivateNextStreetLampEmissive ON: MID=%p, Parent=%s, OnValueWritten=%.3f"),
						MID,
						MID->Parent ? *MID->Parent->GetName() : TEXT("<null>"),
						StreetLampEmissiveDataAsset->EmissiveOnValue);
				}
			}
		}
	}
}

void ADayNightCycle::DeactivateAllEmissive()
{
	GetWorldTimerManager().ClearTimer(EmissiveTimerHandle);

	if (!EmissiveDataAsset)
	{
		return;
	}

	const FName ParamName = EmissiveDataAsset->EmissiveParameterName;
	const float OffValue = EmissiveDataAsset->EmissiveOffValue;

	// Walk all groups and turn off all MIDs.
	int32 DeactivatedCount = 0;
	int32 TotalCount = 0;
	for (const FEmissiveMIDGroup& Group : GroupedMIDs)
	{
		for (UMaterialInstanceDynamic* MID : Group.MIDs)
		{
			TotalCount++;
			if (IsValid(MID))
			{
				MID->SetScalarParameterValue(ParamName, OffValue);
				DeactivatedCount++;

				// === Watched-MI diagnostic log ===
				if (IsWatchedMI(MID))
				{
					UE_LOG(LogTemp, Warning,
						TEXT("[WatchMI] Building.DeactivateAllEmissive OFF: MID=%p, Parent=%s, OffValueWritten=%.3f"),
						MID,
						MID->Parent ? *MID->Parent->GetName() : TEXT("<null>"),
						OffValue);
				}
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("EmissiveAtlas: deactivated %d/%d Emissive MID(s)"), DeactivatedCount, TotalCount);

	// Do NOT clear the cache! Just reset the activation index so the cached MIDs
	// can be reused next time the lights turn on.
	CurrentMIGroupIndex = 0;
	bAllEmissiveActivated = false;
}

void ADayNightCycle::DeactivateAllStreetLampEmissive()
{
	GetWorldTimerManager().ClearTimer(StreetLampEmissiveTimerHandle);

	if (!StreetLampEmissiveDataAsset)
	{
		return;
	}

	const FName ParamName = StreetLampEmissiveDataAsset->EmissiveParameterName;
	const float OffValue = StreetLampEmissiveDataAsset->EmissiveOffValue;

	for (const FEmissiveMIDGroup& Group : StreetLampGroupedMIDs)
	{
		for (UMaterialInstanceDynamic* MID : Group.MIDs)
		{
			if (IsValid(MID))
			{
				MID->SetScalarParameterValue(ParamName, OffValue);

				// === Watched-MI diagnostic log ===
				if (IsWatchedMI(MID))
				{
					UE_LOG(LogTemp, Warning,
						TEXT("[WatchMI] StreetLamp.DeactivateAllStreetLampEmissive OFF: MID=%p, Parent=%s, OffValueWritten=%.3f"),
						MID,
						MID->Parent ? *MID->Parent->GetName() : TEXT("<null>"),
						OffValue);
				}
			}
		}
	}
}

void ADayNightCycle::CollectStreetLampActors(TArray<AActor*>& OutActors) const
{
	OutActors.Empty();

	if (StreetLampActors.Num() > 0)
	{
		for (AActor* Actor : StreetLampActors)
		{
			if (IsValid(Actor))
			{
				OutActors.Add(Actor);
			}
		}
		return;
	}

	if (!StreetLampTag.IsNone())
	{
		TArray<AActor*> FoundActorPtrs;
		UGameplayStatics::GetAllActorsWithTag(GetWorld(), StreetLampTag, FoundActorPtrs);
		for (AActor* Actor : FoundActorPtrs)
		{
			if (IsValid(Actor))
			{
				OutActors.Add(Actor);
			}
		}
	}
}

// ========== StreetLampLight control system ==========

void ADayNightCycle::InitStreetLampLightSystem()
{
	CachedStreetLampLights.Empty();

	if (!bEnableStreetLampLightControl)
	{
		UE_LOG(LogTemp, Log, TEXT("DayNightCycle: street-lamp light control is disabled, skipping init."));
		return;
	}

	TArray<AActor*> FoundActors;

	// Prefer the manually specified Actor list.
	if (StreetLampActors.Num() > 0)
	{
		for (AActor* Actor : StreetLampActors)
		{
			if (IsValid(Actor))
			{
				FoundActors.Add(Actor);
			}
		}
		UE_LOG(LogTemp, Log, TEXT("DayNightCycle: using %d manually specified street-lamp Actor(s)"), FoundActors.Num());
	}
	else if (!StreetLampTag.IsNone())
	{
		// Fall back to Tag-based lookup.
		UGameplayStatics::GetAllActorsWithTag(GetWorld(), StreetLampTag, FoundActors);
		UE_LOG(LogTemp, Log, TEXT("DayNightCycle: looked up %d street-lamp Actor(s) by Tag='%s'"), FoundActors.Num(), *StreetLampTag.ToString());
	}

	// Collect every LocalLightComponent (base class, supports RectLight / SpotLight / PointLight).
	int32 RectLightNum = 0;
	int32 SpotLightNum = 0;
	int32 OtherLightNum = 0;
	for (AActor* Actor : FoundActors)
	{
		TArray<ULocalLightComponent*> LocalLights;
		Actor->GetComponents<ULocalLightComponent>(LocalLights);

		for (ULocalLightComponent* LocalLight : LocalLights)
		{
			if (!IsValid(LocalLight))
			{
				continue;
			}

			CachedStreetLampLights.Add(LocalLight);

			if (LocalLight->IsA<URectLightComponent>())
			{
				RectLightNum++;
			}
			else if (LocalLight->IsA<USpotLightComponent>())
			{
				SpotLightNum++;
			}
			else
			{
				OtherLightNum++;
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("DayNightCycle: street-lamp lights init done, cached %d light(s) (RectLight=%d, SpotLight=%d, Other=%d)"),
		CachedStreetLampLights.Num(), RectLightNum, SpotLightNum, OtherLightNum);
}

void ADayNightCycle::ActivateStreetLampLights()
{
	if (!bEnableStreetLampLightControl)
	{
		return;
	}

	int32 ActivatedCount = 0;
	for (TWeakObjectPtr<ULocalLightComponent>& WeakLight : CachedStreetLampLights)
	{
		if (WeakLight.IsValid())
		{
			WeakLight->SetVisibility(true);
			ActivatedCount++;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("DayNightCycle: turned ON %d/%d street-lamp light(s)"), ActivatedCount, CachedStreetLampLights.Num());
}

void ADayNightCycle::DeactivateStreetLampLights()
{
	if (!bEnableStreetLampLightControl)
	{
		return;
	}

	int32 DeactivatedCount = 0;
	for (TWeakObjectPtr<ULocalLightComponent>& WeakLight : CachedStreetLampLights)
	{
		if (WeakLight.IsValid())
		{
			WeakLight->SetVisibility(false);
			DeactivatedCount++;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("DayNightCycle: turned OFF %d/%d street-lamp light(s)"), DeactivatedCount, CachedStreetLampLights.Num());
}

int32 ADayNightCycle::GetCachedStreetLampLightCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<ULocalLightComponent>& WeakLight : CachedStreetLampLights)
	{
		if (WeakLight.IsValid())
		{
			Count++;
		}
	}
	return Count;
}

void ADayNightCycle::SetCurrentHour(float Hour)
{
	CurrentHour = FMath::Fmod(FMath::Fmod(Hour, 24.f) + 24.f, 24.f); // Normalize to [0, 24)
}

void ADayNightCycle::EnsureDynamicLUT()
{
	if (DynamicLUT && IsValid(DynamicLUT)) return;
	constexpr int32 W = 256;
	constexpr int32 H = 16;
	DynamicLUT = UTexture2D::CreateTransient(W, H, PF_B8G8R8A8, TEXT("DayNightDynamicLUT"));
	if (!DynamicLUT)
	{
		UE_LOG(LogTemp, Warning, TEXT("DayNightCycle: Failed to create DynamicLUT"));
		return;
	}
	DynamicLUT->SRGB = false;
	DynamicLUT->Filter = TextureFilter::TF_Bilinear;
	DynamicLUT->AddressX = TextureAddress::TA_Clamp;
	DynamicLUT->AddressY = TextureAddress::TA_Clamp;
	DynamicLUT->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
	DynamicLUT->LODGroup = TEXTUREGROUP_ColorLookupTable;
}

void ADayNightCycle::UpdateBlendedLUT()
{
	if (LutKeyframes.Num() == 0)
	{
		return;
	}
	EnsureDynamicLUT();
	if (!DynamicLUT || !IsValid(DynamicLUT))
	{
		return;
	}
	// Find the two keyframes to blend between based on CurrentHour.

	const int32 Num = LutKeyframes.Num();
	int32 NextIdx = 0;
	bool bFound = true;
	for (int32 i = 0; i < Num; ++i)
	{
		if (LutKeyframes[i].Hour >= CurrentHour)
		{
			NextIdx = i;
			bFound = true;
			break;
		}
	}
	if (!bFound)
	{
		NextIdx = 0; // Wrap around to first keyframe
	}
	const int32 PreIdx = (NextIdx - 1 + Num) % Num;
	UTexture2D* PreLUT = LutKeyframes[PreIdx].SourceLUT;
	UTexture2D* NextLUT = LutKeyframes[NextIdx].SourceLUT;
	if (!PreLUT || !NextLUT)
	{
		UE_LOG(LogTemp, Warning, TEXT("DayNightCycle: Invalid LUT in keyframes (PreIdx=%d, NextIdx=%d)"), PreIdx, NextIdx);
		return;
	}

	float PreHour = LutKeyframes[PreIdx].Hour;
	float NextHour = LutKeyframes[NextIdx].Hour;
	float Cur = CurrentHour;
	if (NextHour < PreHour)
	{
		NextHour += 24.f; // Handle wrap-around
		if (Cur < PreHour)
		{
			Cur += 24.f;
		}
	}
	const float Alpha = FMath::Clamp((Cur - PreHour) / (NextHour - PreHour + KINDA_SMALL_NUMBER), 0.f, 1.f);

#if WITH_EDITOR
	UE_LOG(LogTemp, Warning, TEXT("DayNightCycle: Blending LUTs (PreIdx=%d, NextIdx=%d, PreHour=%.2f, NextHour=%.2f, CurrentHour=%.2f, Alpha=%.3f)"),
		PreIdx, NextIdx, LutKeyframes[PreIdx].Hour, LutKeyframes[NextIdx].Hour, CurrentHour, Alpha);
#endif
	auto IsValidLUT = [](UTexture2D* LUT) -> bool
	{
		return LUT && IsValid(LUT) && LUT->GetSizeX() == 256 && LUT->GetSizeY() == 16;
	};
	if (!IsValidLUT(PreLUT) || !IsValidLUT(NextLUT))
	{
		UE_LOG(LogTemp, Warning, TEXT("DayNightCycle: One of the LUTs in keyframes is invalid or has wrong dimensions (PreIdx=%d, NextIdx=%d)"), PreIdx, NextIdx);
		return;
	}

	FTexture2DMipMap& PreMip = PreLUT->GetPlatformData()->Mips[0];
	FTexture2DMipMap& NextMip = NextLUT->GetPlatformData()->Mips[0];
	FTexture2DMipMap& DestMip = DynamicLUT->GetPlatformData()->Mips[0];

	const uint8* PreData = static_cast<const uint8*>(PreMip.BulkData.LockReadOnly());
	const uint8* NextData = static_cast<const uint8*>(NextMip.BulkData.LockReadOnly());
	uint8* DestData = static_cast<uint8*>(DestMip.BulkData.Lock(LOCK_READ_WRITE));

	if (PreData && NextData && DestData)
	{
		const int32 PixelCount = 256 * 16;
		for (int32 i = 0; i < PixelCount; ++i)
		{
			int32 Offset = i * 4;
			FLinearColor PreColor = FLinearColor(
				PreData[Offset + 2] / 255.f,
				PreData[Offset + 1] / 255.f,
				PreData[Offset + 0] / 255.f,
				PreData[Offset + 3] / 255.f);
			FLinearColor NextColor = FLinearColor(
				NextData[Offset + 2] / 255.f,
				NextData[Offset + 1] / 255.f,
				NextData[Offset + 0] / 255.f,
				NextData[Offset + 3] / 255.f);
			FLinearColor BlendedColor = FMath::Lerp(PreColor, NextColor, Alpha);
			DestData[Offset + 0] = FMath::RoundToInt(BlendedColor.B * 255.f);
			DestData[Offset + 1] = FMath::RoundToInt(BlendedColor.G * 255.f);
			DestData[Offset + 2] = FMath::RoundToInt(BlendedColor.R * 255.f);
			DestData[Offset + 3] = FMath::RoundToInt(BlendedColor.A * LutIntensity);
		}
	}
	else 
	{
		UE_LOG(LogTemp, Error, TEXT("DayNightCycle: Failed to lock LUT data for blending"));
	}
	DestMip.BulkData.Unlock();
	PreMip.BulkData.Unlock();
	NextMip.BulkData.Unlock();
	DynamicLUT->UpdateResource();
}

void ADayNightCycle::ApplyLUTToPostProcess()
{
	if (!DynamicLUT)
	{
		UE_LOG(LogTemp, Warning, TEXT("DynamicLUT is Empty"));
		return;
	}

	APostProcessVolume* PPV = TargetPostProcessVolume.LoadSynchronous();
	if (!PPV)
	{
		UE_LOG(LogTemp, Warning, TEXT("TargetPostProcessVolume is Empty"));
		return;
	}

	PPV->Settings.bOverride_ColorGradingLUT = true;
	PPV->Settings.bOverride_ColorGradingIntensity = true;
	PPV->Settings.ColorGradingLUT = DynamicLUT;
	PPV->Settings.ColorGradingIntensity = LutIntensity;

	if (!bLutFirstApplyLogged)
	{
		bLutFirstApplyLogged = true;
		UE_LOG(LogTemp, Warning,
			TEXT("LUT Apply Successful, PPV='%s', bEnabled=%s, Priority=%.1f, Unbound=%s, Intensity=%.2f"),
			*PPV->GetName(),
			PPV->bEnabled ? TEXT("true") : TEXT("false"),
			PPV->Priority,
			PPV->bUnbound ? TEXT("true") : TEXT("false"),
			LutIntensity);

		if (!PPV->bEnabled)
		{
			UE_LOG(LogTemp, Warning, TEXT("PPV.bEnabled=false, players will not see the effect! Please enable PPV."));
		}
		if (!PPV->bUnbound)
		{
			UE_LOG(LogTemp, Warning, TEXT("PPV.bUnbound=false, players must enter the PPV volume to see the effect. Consider enabling 'Infinite Extent (Unbound)'."));
		}
	}
}