// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/MainPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"

AMainPlayer::AMainPlayer()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 300.0f, 0.0f); // ...at this rotation rate
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
}

void AMainPlayer::BeginPlay()
{
	Super::BeginPlay();
	
	
	// Grant base abilities
	if (AbilitySystemComponent && HasAuthority())
	{
		for (TSubclassOf<UGameplayAbility>& AbilityClass : BaseAbilities)
		{
			if (AbilityClass)
			{
				AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this));
			}
		}
	}
}

void AMainPlayer::Move(const FInputActionValue& Value)
{
	FVector2D MoveVector = Value.Get<FVector2D>();
	if (Controller)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(ForwardDirection, MoveVector.X);
		AddMovementInput(RightDirection, MoveVector.Y);
	}
}

void AMainPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AMainPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMainPlayer::Move);
		}
	}
}

void AMainPlayer::ApplyCombatStyle(UCombatStyleBase* NewCombatStyle)
{
	if (!NewCombatStyle)
	{
		return;
	}

	// 1. Handle Mesh
	if (NewCombatStyle->StyleMesh)
	{
		GetMesh()->SetSkeletalMeshAsset(NewCombatStyle->StyleMesh);
	}
	else if (DefaultMesh)
	{
		UE_LOG(LogTemp, Log, TEXT("ApplyCombatStyle: Reverting to Default Mesh %s"), *DefaultMesh->GetName());
		// Revert to default if the style doesn't specify a mesh
		GetMesh()->SetSkeletalMeshAsset(DefaultMesh);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ApplyCombatStyle: NewStyle has no mesh and DefaultMesh is null"));
	}

	// 2. Handle AnimInstance
	if (NewCombatStyle->AnimInstance)
	{
		UE_LOG(LogTemp, Log, TEXT("ApplyCombatStyle: Setting AnimInstance to %s"), *NewCombatStyle->AnimInstance->GetName());
		GetMesh()->SetAnimInstanceClass(NewCombatStyle->AnimInstance);
	}
}
