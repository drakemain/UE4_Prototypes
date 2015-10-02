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
    this->inventory = new Inventory(150);
    this->testItem = new Item(0, "ITEM", 5);
    this->testItem1 = new Item(1, "myItem", 10);
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
bool AInventoryPrototypeCharacter::staminaModifyCheck(int stamina, int staminaRate, int maxStamina) {
    bool canModifyStamina = false;
    
    if (staminaRate < 0 && stamina > 0) {
        canModifyStamina = true;
        if (stamina < 0) {
            this->stamina = 0;
        }
    }
    else if (staminaRate > 0 && stamina < maxStamina) {
        canModifyStamina = true;
        if (stamina > maxStamina) {
            this->stamina = this->maxStamina;
        }
    }
    
    return canModifyStamina;
}

bool AInventoryPrototypeCharacter::movingCheck() {
    if (GetVelocity().IsZero()) {
        return false;
    }
    else {
        return true;
    }
}

int AInventoryPrototypeCharacter::getStaminaRate(bool isMoving, bool isSprinting, bool isRunning, bool isWalking) {
    int stamRateToSet;
    
    if (!isMoving) {
        stamRateToSet = 3;
    }
    else {

        if (isSprinting) {
            stamRateToSet = -7;
        }
        else if (isRunning) {
            stamRateToSet = 1;
        }
        else if (isWalking) {
            stamRateToSet = 2;
        }
    }

    return stamRateToSet;
}

void AInventoryPrototypeCharacter::modifyStamina() {
    this->stamina += this->staminaRate;

    if (this->stamina > this->maxStamina) {
        this->stamina = this->maxStamina;
    }
    else if (this->stamina < 0) {
        this->stamina = 0;
    }
}

void AInventoryPrototypeCharacter::sprint() {
    GetCharacterMovement()->MaxWalkSpeed = 1100;
    this->isSprinting = true;
    this->isRunning = false;
    this->isWalking = false;
}

void AInventoryPrototypeCharacter::run() {
    GetCharacterMovement()->MaxWalkSpeed = 600;
    this->isSprinting = false;
    this->isRunning = true;
    this->isWalking = false;
}

void AInventoryPrototypeCharacter::walk() {
    GetCharacterMovement()->MaxWalkSpeed = 300;
    this->isSprinting = false;
    this->isRunning = false;
    this->isWalking = true;
}

void AInventoryPrototypeCharacter::staminaSystem() {
    this->staminaRate = getStaminaRate(movingCheck(), isSprinting, isRunning, isWalking);
    
    if (staminaModifyCheck(stamina, staminaRate, maxStamina)) {
        modifyStamina();

        if (this->stamina <= 0) {
            run();
        }

        UE_LOG(LogTemp, Warning, TEXT("Stamina Check is TRUE."));
        UE_LOG(LogTemp, Log, TEXT("Stamina Rate: %d"), this->staminaRate);
        UE_LOG(LogTemp, Log, TEXT("Stamina: %d"), this->stamina);
    }
}

//Inventory
void AInventoryPrototypeCharacter::pickUpItem(Item* item) {
    inventory->add(item);
}

void AInventoryPrototypeCharacter::printInv() {
    UE_LOG(LogTemp, Warning, TEXT("Attempting to log inventory!"));

    if (inventory->getStackCount() == 0) {
        UE_LOG(LogTemp, Log, TEXT("Inventory is empty."));
        return;
    }

    for (int i = 0; i < inventory->getStackCount(); i++){
        UE_LOG(LogTemp, Log, TEXT("ITEM: %s"), *inventory->getName(i));
        UE_LOG(LogTemp, Log, TEXT("Weight: %d"), inventory->getWeight(i));
        UE_LOG(LogTemp, Log, TEXT("ITEM: %d"), inventory->getID(i));
    }
}

void AInventoryPrototypeCharacter::Tick(float delta) {
    staminaSystem();
}