// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Projectile/Components/AGR_ProjectileLauncherComponent.h"

#include "AGR_ProjectileFunctionLibrary.generated.h"

/**
 * AGR Projectile Blueprint Function Library.
 */
UCLASS()
class AGR_PROJECTILE_RUNTIME_API UAGR_ProjectileFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Gets the AGR Projectile Launcher component from an actor.
     * @param InActor The actor from which to get the component.
     * @return Projectile Launcher component or null if not found.
     */
    UFUNCTION(
        BlueprintPure,
        Category="3Studio AGR|Get Component",
        meta=(ReturnDisplayName="Projectile Launcher Component"))
    static UAGR_ProjectileLauncherComponent* GetProjectileLauncherComponent(
        UPARAM(DisplayName="Actor") const AActor* const InActor);

    /**
     * Performs a trace collision query under the mouse cursor to detect where the player is aiming at.
     * In case of a detected hit, also calculates the rotation needed to look at the impact point from the given origin.
     * @param bOutSuccess True if the trace was successful, false otherwise.
     * @param OutAimAtLocation The location of the impact point under the cursor.
     * @param OutLookAtRotation The rotation needed to look at the impact point of a detected object the cursor is aiming at.
     * @param OutHitResult The hit result of the line trace.
     * @param InAimOrigin The location to aim from.
     * @param InPlayerController Player controller that controls the cursor.
     * @param InTraceChannel Collision channel to use for the trace.
     * @param bInTraceComplex True to test against complex collision, false to test against simplified collision.
     */
    UFUNCTION(
        BlueprintPure,
        Category="3Studio AGR|Aiming",
        meta=(AutoCreateRefTerm=", InAimOrigin"))
    static void CursorAim(
        UPARAM(DisplayName="Success") bool& bOutSuccess,
        UPARAM(DisplayName="Aim at Location") FVector& OutAimAtLocation,
        UPARAM(DisplayName="Look at Rotation") FRotator& OutLookAtRotation,
        UPARAM(DisplayName="Hit Result") FHitResult& OutHitResult,
        UPARAM(DisplayName="Aim Origin") const FVector& InAimOrigin,
        UPARAM(DisplayName="Player Controller") const APlayerController* InPlayerController,
        UPARAM(DisplayName="Trace Channel") const ECollisionChannel InTraceChannel,
        UPARAM(DisplayName="Trace Complex") const bool bInTraceComplex);

    /**
     * Calculates the location and rotation of where the player controller is aiming at. The aim point is the
     * center of the screen, but it can be any offset if needed.
     * 
     * @param bOutSuccess True if the trace was successful, false otherwise.
     * @param OutAimAtLocation The location the camera is aiming at.
     * @param OutLookAtRotation The rotation needed to look at the impact point of a detected object the camera is aiming at.
     * @param OutHitResult The hit result of the line trace.
     * @param InAimOrigin The location to aim from.
     * @param InPlayerController Player controller that controls the camera.
     * @param InTraceDistance The distance to trace from the screen position into the world.
     * @param InTraceChannel The collision channel to use for the trace.
     * @param bInTraceComplex True to test against complex collision, false to test against simplified collision.
     * @param InActorsToIgnore The list of actors to ignore during the trace.
     * @param InScreenSpaceAimCoords The screen-space coordinates (0-1) for the trace starting point.
     */
    UFUNCTION(
        BlueprintPure,
        Category="3Studio AGR|Aiming",
        meta=(AutoCreateRefTerm="InActorsToIgnore, InAimOrigin"))
    static void CameraAim(
        UPARAM(DisplayName="Success") bool& bOutSuccess,
        UPARAM(DisplayName="Aim at Location") FVector& OutAimAtLocation,
        UPARAM(DisplayName="Look at Rotation") FRotator& OutLookAtRotation,
        UPARAM(DisplayName="Hit Result") FHitResult& OutHitResult,
        UPARAM(DisplayName="Aim Origin") const FVector& InAimOrigin,
        UPARAM(DisplayName="Player Controller") const APlayerController* InPlayerController,
        UPARAM(DisplayName="Trace Distance") const float InTraceDistance,
        UPARAM(DisplayName="Trace Channel") const ECollisionChannel InTraceChannel,
        UPARAM(DisplayName="Trace Complex") const bool bInTraceComplex,
        UPARAM(DisplayName="Actors to Ignore") const TArray<AActor*>& InActorsToIgnore,
        UPARAM(DisplayName="Screen Space Aim Coords") const FVector2D& InScreenSpaceAimCoords = FVector2D(0.5f, 0.5f));

    /**
     * Finds world-space absolute rotation of an actor based on its velocity XYZ.
     * @param InTarget The target actor used for the rotation calculation.
     * @return World-space absolute rotation of an actor.
     */
    UFUNCTION(BlueprintPure, Category="3Studio AGR|Transform", meta=(ReturnDisplayName="Rotation"))
    static FRotator FindActorVelocityRotation(UPARAM(DisplayName="Target") const AActor* InTarget);

    /**
     * Calculates the angle of emergence (exit) in radians, representing the alignment of the velocity direction with the hit normal.
     * @param InVelocity Input velocity vector.
     * @param InHitNormal Input hit normal vector.
     * @return The angle of emergence.
     */
    UFUNCTION(BlueprintPure, Category="3Studio AGR|Math", meta=(ReturnDisplayName="Angle"))
    static float CalculateAngleOfEmergence(
        UPARAM(DisplayName="Velocity") const FVector& InVelocity,
        UPARAM(DisplayName="Hit Normal") const FVector& InHitNormal);

    /**
     * Finds worlds-space rotation from objects location and velocity.
     * @param InLocation The starting location for the rotation calculation.
     * @param InVelocity The velocity vector indicating the direction to look towards.
     * @return Worlds-space rotation.
     */
    UFUNCTION(BlueprintPure, Category="3Studio AGR|Transform", meta=(ReturnDisplayName="Rotation"))
    static FRotator FindVelocityRotation(
        UPARAM(DisplayName="Location") const FVector& InLocation,
        UPARAM(DisplayName="Velocity") const FVector& InVelocity);

    /**
     * Builds a list of all actors attached to the given owner and its instigator.
     * The owner and instigator will be included in this list.
     *
     * @param InOwner The owner from which to get all actors.
     * @returns List of actors.
     */
    UFUNCTION(BlueprintCallable, Category="3Studio AGR|Helper")
    static TArray<AActor*> BuildActorList(
        UPARAM(DisplayName="Owner") AActor* const InOwner);

};