// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#include "Libs/AGR_CoreFunctionLibrary.h"

#include "KismetTraceUtils.h"
#include "PhysicsInterfaceDeclaresCore.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/Engine.h"
#include "Engine/HitResult.h"
#include "PhysicsEngine/PhysicsSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AGR_CoreFunctionLibrary)

const TArray<TEnumAsByte<EObjectTypeQuery>> UAGR_CoreFunctionLibrary::AllObjectTypes = {
    ObjectTypeQuery1,
    ObjectTypeQuery2,
    ObjectTypeQuery3,
    ObjectTypeQuery4,
    ObjectTypeQuery5,
    ObjectTypeQuery6,
    ObjectTypeQuery7,
    ObjectTypeQuery8,
    ObjectTypeQuery9,
    ObjectTypeQuery10,
    ObjectTypeQuery11,
    ObjectTypeQuery12,
    ObjectTypeQuery13,
    ObjectTypeQuery14,
    ObjectTypeQuery15,
    ObjectTypeQuery16,
    ObjectTypeQuery17,
    ObjectTypeQuery18,
    ObjectTypeQuery19,
    ObjectTypeQuery20,
    ObjectTypeQuery21,
    ObjectTypeQuery22,
    ObjectTypeQuery23,
    ObjectTypeQuery24,
    ObjectTypeQuery25,
    ObjectTypeQuery26,
    ObjectTypeQuery27,
    ObjectTypeQuery28,
    ObjectTypeQuery29,
    ObjectTypeQuery30,
    ObjectTypeQuery31,
    ObjectTypeQuery32
};

bool UAGR_CoreFunctionLibrary::XRaytrace(
    TArray<FHitResult>& OutHits,
    const UObject* InWorldContextObject,
    const ETraceTypeQuery InTraceChannel,
    const TArray<TEnumAsByte<EObjectTypeQuery>>& InObjectTypes,
    const FVector& InStart,
    const FVector& InEnd,
    const bool bInTraceComplex,
    const TArray<AActor*>& InActorsToIgnore,
    const EDrawDebugTrace::Type InDrawDebugType,
    const bool bInIgnoreSelf,
    const FLinearColor InTraceColor,
    const FLinearColor InTraceHitColor,
    const float InDrawTime)
{
    const UWorld* World = GEngine->GetWorldFromContextObject(
        InWorldContextObject,
        EGetWorldErrorMode::LogAndReturnNull);
    if(!IsValid(World))
    {
        return false;
    }

    static const FName LineTraceMultiName(TEXT("MultiLineTraceByObjectsWithTraceChannel"));
    const FCollisionQueryParams Params = ConfigureCollisionParams(
        LineTraceMultiName,
        bInTraceComplex,
        InActorsToIgnore,
        bInIgnoreSelf,
        InWorldContextObject);

    const FCollisionObjectQueryParams ObjectParams =
        ConfigureCollisionObjectParams(InObjectTypes.IsEmpty() ? AllObjectTypes : InObjectTypes);
    if(!ObjectParams.IsValid())
    {
        UE_LOG(LogBlueprintUserMessages, Warning, TEXT("Invalid object types"));
        return false;
    }

    const ECollisionChannel CollisionChannel = UEngineTypes::ConvertToCollisionChannel(InTraceChannel);
    TArray<FHitResult> CachedHits;
    FPhysicsInterface::RaycastMulti(
        World,
        CachedHits,
        InStart,
        InEnd,
        CollisionChannel,
        Params,
        FCollisionResponseParams::DefaultResponseParam,
        ObjectParams);

    for(const FHitResult& Hit : CachedHits)
    {
        if(!Hit.bBlockingHit)
        {
            continue;
        }

        const UPrimitiveComponent* HitComponent = Hit.GetComponent();
        if(!IsValid(HitComponent))
        {
            continue;
        }

        const ECollisionResponse ResponseToChannel = HitComponent->GetCollisionResponseToChannel(CollisionChannel);
        if(ResponseToChannel == ECR_Block)
        {
            OutHits.Add(Hit);
        }
    }

    const bool bHit = OutHits.Num() > 0;

#if ENABLE_DRAW_DEBUG
    DrawDebugLineTraceMulti(
        World,
        InStart,
        InEnd,
        InDrawDebugType,
        bHit,
        OutHits,
        InTraceColor,
        InTraceHitColor,
        InDrawTime);
#endif

    return bHit;
}

FCollisionQueryParams UAGR_CoreFunctionLibrary::ConfigureCollisionParams(
    const FName& TraceTag,
    const bool bTraceComplex,
    const TArray<AActor*>& ActorsToIgnore,
    const bool bIgnoreSelf,
    const UObject* WorldContextObject)
{
    FCollisionQueryParams Params(TraceTag, SCENE_QUERY_STAT_ONLY(AGR_CoreFunctionLibrary), bTraceComplex);
    Params.bReturnPhysicalMaterial = true;
    // Ask for face index, as long as we didn't disable globally
    Params.bReturnFaceIndex = !UPhysicsSettings::Get()->bSuppressFaceRemapTable;
    Params.AddIgnoredActors(ActorsToIgnore);

    if(!bIgnoreSelf)
    {
        return Params;
    }

    const AActor* IgnoreActor = Cast<AActor>(WorldContextObject);
    if(IgnoreActor)
    {
        Params.AddIgnoredActor(IgnoreActor);
        return Params;
    }

    // find owner
    const UObject* CurrentObject = WorldContextObject;
    while(CurrentObject)
    {
        CurrentObject = CurrentObject->GetOuter();
        IgnoreActor = Cast<AActor>(CurrentObject);
        if(IgnoreActor)
        {
            Params.AddIgnoredActor(IgnoreActor);
            break;
        }
    }

    return Params;
}

FCollisionObjectQueryParams UAGR_CoreFunctionLibrary::ConfigureCollisionObjectParams(
    const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes)
{
    TArray<TEnumAsByte<ECollisionChannel>> CollisionObjectTraces;
    CollisionObjectTraces.AddUninitialized(ObjectTypes.Num());

    for(auto Iter = ObjectTypes.CreateConstIterator(); Iter; ++Iter)
    {
        CollisionObjectTraces[Iter.GetIndex()] = UEngineTypes::ConvertToCollisionChannel(*Iter);
    }

    FCollisionObjectQueryParams ObjectParams;
    for(auto Iter = CollisionObjectTraces.CreateConstIterator(); Iter; ++Iter)
    {
        const ECollisionChannel& Channel = (*Iter);
        if(FCollisionObjectQueryParams::IsValidObjectQuery(Channel))
        {
            ObjectParams.AddObjectTypesToQuery(Channel);
        }
        else
        {
            UE_LOG(LogBlueprintUserMessages, Warning, TEXT("%d isn't valid object type"), (int32)Channel);
        }
    }

    return ObjectParams;
}