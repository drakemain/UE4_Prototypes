// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "InventoryPrototype.h"
#include "InventoryPrototypeCharacter.h"
#include "InventoryPrototypeProjectile.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/InputSettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// AInventoryPrototypeCharacter

AInventoryPrototypeCharacter::AInventoryPrototypeCharacter()
{
    //PrimaryActorTick.bCanEverTick = true;
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->AttachParent = GetCapsuleComponent();
	FirstPersonCameraComponent->RelativeLocation = FVector(0, 0, 64.f); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->AttachParent = FirstPersonCameraComponent;
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;

	// Create a gun mesh component
	FP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	FP_Gun->SetOnlyOwnerSee(true);			// only the owning player will see this mesh
	FP_Gun->bCastDynamicShadow = false;
	FP_Gun->CastShadow = false;
	FP_Gun->AttachTo(Mesh1P, TEXT("GripPoint"), EAttachLocation::SnapToTargetIncludingScale, true);


	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 30.0f, 10.0f);

	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for Mesh1P are set in the
	// derived blueprint asset named MyCharacter (to avoid direct content references in C++)


    //stamina and movement
    this->stamina = 500;
    this->maxStamina = 1000;
    this->staminaRate = 1;
    this->isRunning = true;

    //inventory and items
    Item* item = &testItem1;
    item = new Item(0, "TEST", 5);
}

//////////////////////////////////////////////////////////////////////////
// Input

void AInventoryPrototypeCharacter::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	// set up gameplay key bindings
	check(InputComponent);

	InputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	InputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	
	//InputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AInventoryPrototypeCharacter::TouchStarted);
	if( EnableTouchscreenMovement(InputComponent) == false )
	{
		InputComponent->BindAction("Fire", IE_Pressed, this, &AInventoryPrototypeCharacter::OnFire);
	}
	
	InputComponent->BindAxis("MoveForward", this, &AInventoryPrototypeCharacter::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &AInventoryPrototypeCharacter::MoveRight);
	
	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	InputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	InputComponent->BindAxis("TurnRate", this, &AInventoryPrototypeCharacter::TurnAtRate);
	InputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	InputComponent->BindAxis("LookUpRate", this, &AInventoryPrototypeCharacter::LookUpAtRate);
}

void AInventoryPrototypeCharacter::OnFire()
{ 
	// try and fire a projectile
	if (ProjectileClass != NULL)
	{
		const FRotator SpawnRotation = GetControlRotation();
		// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
		const FVector SpawnLocation = GetActorLocation() + SpawnRotation.RotateVector(GunOffset);

		UWorld* const World = GetWorld();
		if (World != NULL)
		{
			// spawn the projectile at the muzzle
			World->SpawnActor<AInventoryPrototypeProjectile>(ProjectileClass, SpawnLocation, SpawnRotation);
		}
	}

	// try and play the sound if specified
	if (FireSound != NULL)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	// try and play a firing animation if specified
	if(FireAnimation != NULL)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
		if(AnimInstance != NULL)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}

}

void AInventoryPrototypeCharacter::BeginTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if( TouchItem.bIsPressed == true )
	{
		return;
	}
	TouchItem.bIsPressed = true;
	TouchItem.FingerIndex = FingerIndex;
	TouchItem.Location = Location;
	TouchItem.bMoved = false;
}

void AInventoryPrototypeCharacter::EndTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == false)
	{
		return;
	}
	if( ( FingerIndex == TouchItem.FingerIndex ) && (TouchItem.bMoved == false) )
	{
		OnFire();
	}
	TouchItem.bIsPressed = false;
}

void AInventoryPrototypeCharacter::TouchUpdate(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if ((TouchItem.bIsPressed == true) && ( TouchItem.FingerIndex==FingerIndex))
	{
		if (TouchItem.bIsPressed)
		{
			if (GetWorld() != nullptr)
			{
				UGameViewportClient* ViewportClient = GetWorld()->GetGameViewport();
				if (ViewportClient != nullptr)
				{
					FVector MoveDelta = Location - TouchItem.Location;
					FVector2D ScreenSize;
					ViewportClient->GetViewportSize(ScreenSize);
					FVector2D ScaledDelta = FVector2D( MoveDelta.X, MoveDelta.Y) / ScreenSize;									
					if (ScaledDelta.X != 0.0f)
					{
						TouchItem.bMoved = true;
						float Value = ScaledDelta.X * BaseTurnRate;
						AddControllerYawInput(Value);
					}
					if (ScaledDelta.Y != 0.0f)
					{
						TouchItem.bMoved = true;
						float Value = ScaledDelta.Y* BaseTurnRate;
						AddControllerPitchInput(Value);
					}
					TouchItem.Location = Location;
				}
				TouchItem.Location = Location;
			}
		}
	}
}

void AInventoryPrototypeCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AInventoryPrototypeCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AInventoryPrototypeCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AInventoryPrototypeCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

bool AInventoryPrototypeCharacter::EnableTouchscreenMovement(class UInputComponent* InputComponent)
{
	bool bResult = false;
	if(FPlatformMisc::GetUseVirtualJoysticks() || GetDefault<UInputSettings>()->bUseMouseForTouch )
	{
		bResult = true;
		InputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AInventoryPrototypeCharacter::BeginTouch);
		InputComponent->BindTouch(EInputEvent::IE_Released, this, &AInventoryPrototypeCharacter::EndTouch);
		InputComponent->BindTouch(EInputEvent::IE_Repeat, this, &AInventoryPrototypeCharacter::TouchUpdate);
	}
	return bResult;
}

//Stamina and Movement
void AInventoryPrototypeCharacter::staminaCheck() {
    if (staminaRate < 0) {
        if (stamina > 0) {
            modifyStamina();
        }
        else if (stamina < 0) {
            stamina = 0;
        }
    }
    else if (staminaRate > 0) {
        if (stamina < maxStamina) {
            modifyStamina();
        }
        else if (stamina > maxStamina) {
            stamina = maxStamina;
        }
    }
    UE_LOG(LogTemp, Log, TEXT("Stamina: %d"), this->stamina);
}

void AInventoryPrototypeCharacter::movingCheck() {
    
    
    if (this->GetVelocity().IsZero() && this->staminaRate != 5) {
        this->staminaRate = 5;
    }
    else {
        this->setMovingStaminaRate();
    }
    UE_LOG(LogTemp, Log, TEXT("Stamina Rate: %d"), this->staminaRate);
}

void AInventoryPrototypeCharacter::setMovingStaminaRate() {
    if (isSprinting) {
        staminaRate = -5;
    }
    else if (isRunning) {
        staminaRate = 1;
    }
    else if (isWalking) {
        staminaRate = 3;
    }
}

void AInventoryPrototypeCharacter::modifyStamina() {
    stamina += staminaRate;
}

void AInventoryPrototypeCharacter::sprint() {
    GetCharacterMovement()->MaxWalkSpeed = 1000;
    isSprinting = true;
    isRunning = false;
    isWalking = false;
}

void AInventoryPrototypeCharacter::run() {
    GetCharacterMovement()->MaxWalkSpeed = 600;
    isSprinting = false;
    isRunning = true;
    isWalking = false;
}

void AInventoryPrototypeCharacter::walk() {
    GetCharacterMovement()->MaxWalkSpeed = 300;
    isSprinting = false;
    isRunning = false;
    isWalking = true;
}

//Inventory
void AInventoryPrototypeCharacter::pickUpItem() {
    Inventory.Add(&testItem1);
}

/*
void AInventoryPrototypeCharacter::dropItem() {
    this->Inventory.Pop(&testItem1);
}*/

void AInventoryPrototypeCharacter::printInv() {
    UE_LOG(LogTemp, Warning, TEXT("Attempting to log inventory!"));

    if (Inventory.Num() == 0) {
        UE_LOG(LogTemp, Log, TEXT("Inventory is empty."));
        return;
    }

    for (int i = 0; i < Inventory.Num(); i++){
        UE_LOG(LogTemp, Log, TEXT("ITEM: %s"), *Inventory[i]->name);
        UE_LOG(LogTemp, Log, TEXT("Weight: %d"), Inventory[i]->weight);
        UE_LOG(LogTemp, Log, TEXT("ITEM: %d"), Inventory[i]->ID);
    }
}

void AInventoryPrototypeCharacter::Tick(float delta) {
    this->movingCheck();    
    this->staminaCheck();
}