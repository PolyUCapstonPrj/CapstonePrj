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
	static void BuildEmissiveGroupsByMeshScan(
		const TArray<AActor*>& FoundActors,
		UEmissiveConfigDataAsset* DataAsset,
		TArray<FEmissiveMIDGroup>& OutGroupedMIDs,
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
					if (!MID)
					{
						MID = MeshComp->CreateDynamicMaterialInstance(SlotIndex, LoadedMIs[*GroupIndexPtr]);
					}

					if (MID)
					{
						MID->SetScalarParameterValue(ParamName, DataAsset->EmissiveOffValue);
						TempGroups[*GroupIndexPtr].MIDs.Add(MID);
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
	// Initialize the emissive system.
	InitEmissiveSystem();

	// Initialize the street-lamp emissive system (separated from buildings).
	InitStreetLampEmissiveSystem();

	// Initialize the StreetLampLight system.
	InitStreetLampLightSystem();

	bWasLightsOn = ShouldLightsBeOn();

	// If lights should already be on, kick off the activation sequence immediately.
	if (bWasLightsOn)
	{
		UE_LOG(LogTemp, Warning, TEXT("DayNightCycle: delayed init done, currently night, activating Emissive (BuildingMIGroups=%d, StreetLampMIGroups=%d, StreetLampLights=%d)"), GroupedMIDs.Num(), StreetLampGroupedMIDs.Num(), CachedStreetLampLights.Num());
		ActivateNextEmissive();
		ActivateNextStreetLampEmissive();
		ActivateStreetLampLights();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("DayNightCycle: delayed init done, currently day (BuildingMIGroups=%d, StreetLampMIGroups=%d, StreetLampLights=%d)"), GroupedMIDs.Num(), StreetLampGroupedMIDs.Num(), CachedStreetLampLights.Num());
		DeactivateStreetLampLights();
	}

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
		else
		{
			// MID cache is valid: just reset the activation index.
			CurrentMIGroupIndex = 0;
			bAllEmissiveActivated = false;
		}
		ActivateNextEmissive();
		ActivateNextStreetLampEmissive();
		ActivateStreetLampLights();
	}
	else if (bWasLightsOn && !bShouldLightsOn)
	{
		UE_LOG(LogTemp, Warning, TEXT("DayNightCycle: >>> lights OFF triggered! Pitch=%.1f"), GetSunPitch());
		bWasLightsOn = false;
		OnLightsOff.Broadcast(false);
		DeactivateAllEmissive();
		DeactivateAllStreetLampEmissive();
		DeactivateStreetLampLights();
	}
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
		ActivateNextEmissive();
		ActivateNextStreetLampEmissive();
		ActivateStreetLampLights();
	}
}

void ADayNightCycle::ForceLightsOff()
{
	if (bWasLightsOn)
	{
		bWasLightsOn = false;
		OnLightsOff.Broadcast(false);
		DeactivateAllEmissive();
		DeactivateAllStreetLampEmissive();
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

	BuildEmissiveGroupsByMeshScan(FoundActors, EmissiveDataAsset, GroupedMIDs, TEXT("EmissiveAtlas(Building)"));
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

	BuildEmissiveGroupsByMeshScan(FoundActors, StreetLampEmissiveDataAsset, StreetLampGroupedMIDs, TEXT("EmissiveAtlas(StreetLamp)"));
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
