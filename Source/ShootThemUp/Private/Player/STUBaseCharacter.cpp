// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/STUBaseCharacter.h"

#include "Components/STUCharacterMovementComponent.h"

DEFINE_LOG_CATEGORY_STATIC(BaseCharacterLog, All, All);

ASTUBaseCharacter::ASTUBaseCharacter(const FObjectInitializer& ObjInit):
    Super(ObjInit.SetDefaultSubobjectClass<USTUCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
    
    SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>("SpringArmComponent");
    SpringArmComponent->SetupAttachment(GetRootComponent());
    SpringArmComponent->bUsePawnControlRotation = true;
    
    CameraComponent = CreateDefaultSubobject<UCameraComponent>("CameraComponent");
    CameraComponent->SetupAttachment(SpringArmComponent);

    HealthComponent = CreateDefaultSubobject<USTUHealthComponent>("HealthComponent");
    HealthTextComponent = CreateDefaultSubobject<UTextRenderComponent>("HealthTextComponent");
    HealthTextComponent->SetupAttachment(GetRootComponent());

    
}

// Called when the game starts or when spawned

void ASTUBaseCharacter::BeginPlay()
{
	Super::BeginPlay();

    check(HealthComponent);
    check(HealthTextComponent);
    check(GetCharacterMovement());

    OnHealthChanged(HealthComponent->GetHealth());
    HealthComponent->OnDeath.AddUObject(this, &ASTUBaseCharacter::OnDeath);
    HealthComponent->OnHealthChanged.AddUObject(this, &ASTUBaseCharacter::OnHealthChanged);

    LandedDelegate.AddDynamic(this, &ASTUBaseCharacter::OnGroundLanded);
}

// Called every frame

void ASTUBaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ASTUBaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
    check(PlayerInputComponent);

    PlayerInputComponent->BindAxis("MoveForward", this, &ASTUBaseCharacter::MoveForward);
    PlayerInputComponent->BindAxis("MoveRight", this, &ASTUBaseCharacter::MoveRight);
    PlayerInputComponent->BindAxis("LookUp", this, &ASTUBaseCharacter::LookUp);
    PlayerInputComponent->BindAxis("TurnAround", this, &ASTUBaseCharacter::TurnAround);
    PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ASTUBaseCharacter::Jump);
    PlayerInputComponent->BindAction("Run", IE_Pressed, this, &ASTUBaseCharacter::OnStartRunning);
    PlayerInputComponent->BindAction("Run", IE_Released, this, &ASTUBaseCharacter::OnStopRunning);
}

bool ASTUBaseCharacter::IsRunning()
{
    return WantsToRun && IsMovingForward && !GetVelocity().IsZero();
}

float ASTUBaseCharacter::GetMovementDirection() const
{
    if (GetVelocity().IsZero()) return 0.0f;
    // ???????????????? ?????????????? ?????????????? ????????????????
    const auto VelocityNormal = GetVelocity().GetSafeNormal();
    // ???????????????? ???????? ?????????? ???????????????? ???????????????? ?? ???????????????? ?????????????????????? ??????????????????
    const auto AngelBetween = FMath::Acos(FVector::DotProduct(GetActorForwardVector(), VelocityNormal));
    // ?????????????????????? ?????????????????????????? ???????????? ?????? ?????????????????????? ?????????????????????? ???????? (?????????? ????????)
    const auto CrossProduct = FVector::CrossProduct(GetActorForwardVector(), VelocityNormal);
    const auto Degrees = FMath::RadiansToDegrees(AngelBetween);
    return CrossProduct.IsZero() ? Degrees : Degrees * FMath::Sign(CrossProduct.Z);
}

void ASTUBaseCharacter::MoveForward(float Amount)
{
    IsMovingForward = Amount > 0;
    if(Amount == 0.0f) return;
    AddMovementInput(GetActorForwardVector(), Amount);
}

void ASTUBaseCharacter::MoveRight(float Amount)
{
    if(Amount == 0.0f) return;
    AddMovementInput(GetActorRightVector(), Amount);
}

void ASTUBaseCharacter::LookUp(float Amount)
{
    AddControllerPitchInput(Amount);
}

void ASTUBaseCharacter::TurnAround(float Amount)
{
    AddControllerYawInput(Amount);
}

void ASTUBaseCharacter::OnStartRunning()
{
    WantsToRun = true;
}

void ASTUBaseCharacter::OnStopRunning()
{
    WantsToRun = false;
}

void ASTUBaseCharacter::OnDeath()
{
    UE_LOG(BaseCharacterLog, Display, TEXT("Player %s is dead"), *GetName());
    PlayAnimMontage(DeathAnimMontage);

    GetCharacterMovement()->DisableMovement();
    SetLifeSpan(LifeSpanOnDeath);

    if (Controller)
    {
        Controller->ChangeState(NAME_Spectating);
    }
}

void ASTUBaseCharacter::OnHealthChanged(float Health)
{
    HealthTextComponent->SetText(FText::FromString(FString::Printf(TEXT("%0.f"), Health)));
}

void ASTUBaseCharacter::OnGroundLanded(const FHitResult &Hit)
{
    const auto FallVelocityZ = -GetVelocity().Z;
    UE_LOG(BaseCharacterLog, Display, TEXT("On landed: %f"), FallVelocityZ);

    if (FallVelocityZ < LandedDamageVelocity.X) return;

    const auto FinalDamage = FMath::GetMappedRangeValueClamped(LandedDamageVelocity, LandedDamage, FallVelocityZ);
    UE_LOG(BaseCharacterLog, Display, TEXT("Final Damage: %f"), FinalDamage);
    TakeDamage(FinalDamage, FDamageEvent{}, nullptr, nullptr);
}
