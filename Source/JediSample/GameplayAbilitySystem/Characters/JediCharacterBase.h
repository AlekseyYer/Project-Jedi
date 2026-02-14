// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "JediCharacterBase.generated.h"

UCLASS()
class JEDISAMPLE_API AJediCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public: 
	// Sets default values for this character's properties
	AJediCharacterBase();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem")
	UAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly, Category = "AbilitySystem")
	class UBasicAttributeSet* BasicAttributeSet;
protected:
	
	// Whether the character is currently blocking
	UPROPERTY(BlueprintReadWrite, Category = "Combat")
	bool bIsBlocking;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	TSubclassOf<AActor> WeaponClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities")
	TArray<TSubclassOf<UGameplayAbility>> AbilitiesToGrant;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Slice")
	UProceduralMeshComponent* ProceduralMeshCopy;

	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slice")
	UMaterialInterface* SliceCapMaterial;
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AbilitySystem")
	EGameplayEffectReplicationMode AscReplicationMode = EGameplayEffectReplicationMode::Mixed;

    virtual void PossessedBy(AController* NewController) override;

	virtual void OnDeadTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Damage")
	void HandleDeath();

	// Replication for PlayerState (ex. player joins a server or spawns)
	virtual void OnRep_PlayerState() override;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
	void SliceAtPoint(FVector HitLocation, FVector SliceNormal);
};
