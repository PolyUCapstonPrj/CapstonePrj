// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/MainPlayer.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include  "GameFramework/CharacterMovementComponent.h"

AMainPlayer::AMainPlayer()
{
	PrimaryActorTick.bCanEverTick = true;
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller
	PlayerCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
	PlayerCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	PlayerCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 300.0f, 0.0f); // ...at this rotation rate
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
}

void AMainPlayer::BeginPlay()
{
	Super::BeginPlay();
	if (const ULocalPlayer* LocalPlayer = (GEngine && GetWorld())?GEngine->GetLocalPlayerFromControllerId(GetWorld(), 0):nullptr)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
		{
			if (PlayerMappingContext)
			{
				Subsystem->AddMappingContext(PlayerMappingContext, 0);
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

void AMainPlayer::Look(const FInputActionValue& Value)
{
	FVector2D LookVector = Value.Get<FVector2D>();
	if (Controller)
	{
		AddControllerYawInput(LookVector.X);
		AddControllerPitchInput(LookVector.Y);
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
		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMainPlayer::Look);
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
