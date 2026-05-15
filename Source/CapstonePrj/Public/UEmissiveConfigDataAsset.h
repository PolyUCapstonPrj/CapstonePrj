#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Materials/MaterialInstance.h"
#include "UEmissiveConfigDataAsset.generated.h"

/**
 * Emissive config data asset: manages a list of Material Instances whose
 * emissive parameter should be activated/deactivated in order.
 * Can be used for buildings (the building path uses TargetActorTag to find
 * actors in the world), and also for street lamps or other use cases (in
 * which case TargetActorTag is ignored and the caller decides the actor
 * source).
 */
UCLASS(BlueprintType)
class CAPSTONEPRJ_API UEmissiveConfigDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    /** All Material Instances whose emissive parameter will be activated, in activation order. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emissive")
    TArray<TSoftObjectPtr<UMaterialInstance>> EmissiveMIs;

    /** Interval (seconds) between activations of consecutive MI groups. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emissive", meta = (ClampMin = "0.1"))
    float ActivationInterval = 2.0f;

    /** Name of the scalar parameter on the MI that controls emissive strength. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emissive")
    FName EmissiveParameterName = TEXT("EmissiveStrength");

    /** Value to set when activating (default 1.0 = fully on). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emissive", meta = (ClampMin = "0.0"))
    float EmissiveOnValue = 1.0f;

    /** Value to set when deactivating (default 0.0 = fully off). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emissive", meta = (ClampMin = "0.0"))
    float EmissiveOffValue = 0.0f;

    /** Tag used to find target Actors in the world.
     *  Only the building emissive path reads this field; the street-lamp
     *  emissive path is driven by ADayNightCycle::StreetLampTag and ignores it. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emissive")
    FName TargetActorTag = TEXT("EmissiveBuilding");
};