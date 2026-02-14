// Fill out your copyright notice in the Description page of Project Settings.


#include "JediCharacterBase.h"

#include "JediSample/GameplayAbilitySystem/SliceHelper.h"
#include "JediSample/GameplayAbilitySystem/AttributeSets/BasicAttributeSet.h"


// Sets default values
AJediCharacterBase::AJediCharacterBase()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Add the ability system component
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(AscReplicationMode);

	// Character defaults
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	//Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.f;

	// Add the attribute set
	BasicAttributeSet = CreateDefaultSubobject<UBasicAttributeSet>(TEXT("BasicAttributeSet"));

	ProceduralMeshCopy = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMeshCopy"));
	ProceduralMeshCopy->SetupAttachment(GetMesh());
	ProceduralMeshCopy->SetVisibility(false);
}

// Called when the game starts or when spawned
void AJediCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystemComponent->RegisterGameplayTagEvent(FGameplayTag::RequestGameplayTag("State.Dead"))
	.AddUObject(this, &AJediCharacterBase::OnDeadTagChanged);

	if (GetMesh() && ProceduralMeshCopy)
	{
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, [this]()
		{
			if (GetMesh() && ProceduralMeshCopy)
			{
				USliceHelper::CopySkeletalMeshToProceduralMesh(GetMesh(), ProceduralMeshCopy);
				ProceduralMeshCopy->SetVisibility(false);
				UE_LOG(LogTemp, Warning, TEXT("Pre-copy complete. Sections: %d"), ProceduralMeshCopy->GetNumSections());
			}
		}, 0.5f, false);
	}
}

// Called every frame
void AJediCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AJediCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

UAbilitySystemComponent* AJediCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AJediCharacterBase::SliceAtPoint_Implementation(FVector HitLocation, FVector SliceNormal)
{
    if (!GetMesh() || !ProceduralMeshCopy) return;

	// If pre-copy hasn't happened, do full copy (fallback)
	if (ProceduralMeshCopy->GetNumSections() == 0)
	{
		USliceHelper::CopySkeletalMeshToProceduralMesh(GetMesh(), ProceduralMeshCopy);
	}
	// Otherwise just use the pre-copied mesh as-is

	ProceduralMeshCopy->SetWorldTransform(GetMesh()->GetComponentTransform());
	GetMesh()->SetVisibility(false);
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ProceduralMeshCopy->SetVisibility(true);

    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetCharacterMovement()->DisableMovement();

    // SliceNormal is now the actual swing direction from the Blueprint
    UProceduralMeshComponent* OtherHalf = nullptr;

    UKismetProceduralMeshLibrary::SliceProceduralMesh(
        ProceduralMeshCopy,
        HitLocation,
        SliceNormal,
        true,
        OtherHalf,
        EProcMeshSliceCapOption::CreateNewSectionForCap,
        SliceCapMaterial
    );

    if (OtherHalf)
    {
        ProceduralMeshCopy->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
        OtherHalf->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

    	auto SetupHalf = [](UProceduralMeshComponent* Half)
    	{
    		Half->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    		Half->SetCollisionResponseToAllChannels(ECR_Block);
    		Half->SetCollisionObjectType(ECC_PhysicsBody);
    		Half->bUseComplexAsSimpleCollision = false;

    		FBox Bounds(ForceInit);
    		for (int32 i = 0; i < Half->GetNumSections(); i++)
    		{
    			FProcMeshSection* Section = Half->GetProcMeshSection(i);
    			if (Section)
    			{
    				for (const FProcMeshVertex& Vert : Section->ProcVertexBuffer)
    				{
    					Bounds += Vert.Position;
    				}
    			}
    		}

    		if (Bounds.IsValid)
    		{
    			TArray<FVector> BoxVerts;
    			BoxVerts.Add(Bounds.Min);
    			BoxVerts.Add(Bounds.Max);
    			BoxVerts.Add(FVector(Bounds.Min.X, Bounds.Min.Y, Bounds.Max.Z));
    			BoxVerts.Add(FVector(Bounds.Min.X, Bounds.Max.Y, Bounds.Min.Z));
    			BoxVerts.Add(FVector(Bounds.Max.X, Bounds.Min.Y, Bounds.Min.Z));
    			BoxVerts.Add(FVector(Bounds.Max.X, Bounds.Max.Y, Bounds.Min.Z));
    			BoxVerts.Add(FVector(Bounds.Max.X, Bounds.Min.Y, Bounds.Max.Z));
    			BoxVerts.Add(FVector(Bounds.Min.X, Bounds.Max.Y, Bounds.Max.Z));

    			Half->ClearCollisionConvexMeshes();
    			Half->AddCollisionConvexMesh(BoxVerts);
    		}

    		Half->SetSimulatePhysics(true);
    	};

        SetupHalf(ProceduralMeshCopy);
        SetupHalf(OtherHalf);

    	float TumbleStrength = 150.f;
    	FVector RandomTumble = FMath::VRand() * TumbleStrength;

    	ProceduralMeshCopy->AddAngularImpulseInDegrees(RandomTumble, NAME_None, true);
    	OtherHalf->AddAngularImpulseInDegrees(-RandomTumble, NAME_None, true);
    }
}
void AJediCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

void AJediCharacterBase::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

// Handle death 
void AJediCharacterBase::OnDeadTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	//if one count of the tag has been added
	if (NewCount > 0)
	{
		HandleDeath();
	}
}

void AJediCharacterBase::HandleDeath_Implementation()
{
	GetCharacterMovement()->DisableMovement();
}

