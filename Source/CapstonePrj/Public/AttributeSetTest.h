// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Attributes/GSCAttributeSetBase.h"
#include "AbilitySystemComponent.h"
#include "AttributeSetTest.generated.h"

UCLASS()
class CAPSTONEPRJ_API UAttributeSetTest : public UGSCAttributeSetBase
{
	GENERATED_BODY()

public:

	// Sets default values for this AttributeSet attributes
	UAttributeSetTest();

    // AttributeSet Overrides
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
        
    UPROPERTY(BlueprintReadOnly, Category = "", ReplicatedUsing = OnRep_Health)
    FGameplayAttributeData Health = 100.0;
    ATTRIBUTE_ACCESSORS(UAttributeSetTest, Health)    

protected:
    
    UFUNCTION()
    virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);
};
