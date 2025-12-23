// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "Base/CommonBase.h"
#include "GameplayTagContainer.h"
#include "GameplayAbilitySpec.h"
#include "GameplayEffectTypes.h"
#include "Base/CombatStyleBase.h"
#include "MainPlayer.generated.h"

class USpringArmComponent;
class UCameraComponent;
class USkeletalMesh;
class UMaterialInterface;
class UInputAction;
class UInputMappingContext;
struct  FInputActionValue;


/**
 * 
 */
UCLASS()
class CAPSTONEPRJ_API AMainPlayer : public ACommonBase
{
	GENERATED_BODY()
	
public:
	AMainPlayer();
	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	UFUNCTION(BlueprintCallable, Category="Change Combat Styles")
	void ApplyCombatStyle(UCombatStyleBase* NewCombatStyle);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat Styles")
	TObjectPtr<UCombatStyleBase> DefaultCombatStyle;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
    TArray<TSubclassOf<class UGameplayAbility>> BaseAbilities;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	
	UPROPERTY()
	TObjectPtr<USkeletalMesh> DefaultMesh;
	
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInterface>> Materials;
	
	TArray<FGameplayAbilitySpecHandle> CurrentStyleAbilityHandles;

	// Track current passive effect to remove it when switching
	FActiveGameplayEffectHandle CurrentStylePassiveHandle;
	
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta =(AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta =(AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> PlayerCamera;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> PlayerMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LookAction;
	
};
