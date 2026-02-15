
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float RingRadius = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float OrbitSpeed = 30.f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	AActor* PlayerCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	TArray<AJediCharacterBase*> Enemies;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	AJediCharacterBase* ActiveAttacker;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void RegisterEnemy(AJediCharacterBase* Enemy);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void UnregisterEnemy(AJediCharacterBase* Enemy);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void OnEnemyDied(AJediCharacterBase* Enemy);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ActivateNextAttacker();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	FVector GetRingPosition(int32 EnemyIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetEnemyDisabled(AJediCharacterBase* Enemy, bool bDisabled);

private:
	UPROPERTY()
	TSet<AJediCharacterBase*> DisabledEnemies;

	float OrbitAngleOffset = 0.f;

	virtual void Tick(float DeltaTime) override;

	void AssignAttacker(AJediCharacterBase* NewAttacker);
	AJediCharacterBase* PickNextAttacker() const;
};