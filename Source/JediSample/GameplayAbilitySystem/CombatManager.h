#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Characters/JediCharacterBase.h"
#include "CombatManager.generated.h"

UCLASS()
class JEDISAMPLE_API ACombatManager : public AActor
{
	GENERATED_BODY()

public:
	ACombatManager();

	virtual void BeginPlay() override;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	AActor* PlayerCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	TArray<AJediCharacterBase*> Enemies;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	AJediCharacterBase* ActiveAttacker;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Spawning")
	TSubclassOf<AJediCharacterBase> EnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Spawning")
	float SpawnRadiusMin = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Spawning")
	float SpawnRadiusMax = 700.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float VitalsShowRange = 600.f;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void RegisterEnemy(AJediCharacterBase* Enemy);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void UnregisterEnemy(AJediCharacterBase* Enemy);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void OnEnemyDied(AJediCharacterBase* Enemy);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ActivateNextAttacker();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetEnemyDisabled(AJediCharacterBase* Enemy, bool bDisabled);

private:
	UPROPERTY()
	TSet<AJediCharacterBase*> DisabledEnemies;

	TSet<AJediCharacterBase*> EnemiesWithVitalsVisible;

	virtual void Tick(float DeltaTime) override;

	void AssignAttacker(AJediCharacterBase* NewAttacker);
	AJediCharacterBase* PickNextAttacker() const;
	void SpawnNextEnemy();
	FVector FindSpawnLocation() const;
};