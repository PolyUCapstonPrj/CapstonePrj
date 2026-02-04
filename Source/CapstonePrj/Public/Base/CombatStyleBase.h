// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "CombatStyleBase.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class CAPSTONEPRJ_API UCombatStyleBase : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Style")
	FName StyleName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Style")
	FGameplayTag StyleTag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Style")
	USkeletalMesh* StyleMesh;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Style")
	TSubclassOf<UAnimInstance> AnimInstance;
};
