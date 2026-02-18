// CombatManager.cpp
#include "CombatManager.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"

ACombatManager::ACombatManager()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ACombatManager::BeginPlay()
{
    Super::BeginPlay();

    PlayerCharacter = UGameplayStatics::GetPlayerPawn(this, 0);
}

void ACombatManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!PlayerCharacter) return;

    int32 WaitingIndex = 0;
    for (AJediCharacterBase* Enemy : Enemies)
    {
        if (!Enemy || DisabledEnemies.Contains(Enemy)) continue;

        // Rotate ALL enemies toward the player
        FVector DirToPlayer = PlayerCharacter->GetActorLocation() - Enemy->GetActorLocation();
        DirToPlayer.Z = 0.f;
        if (!DirToPlayer.IsNearlyZero())
        {
            FRotator LookRot = DirToPlayer.Rotation();
            Enemy->SetActorRotation(FMath::RInterpTo(Enemy->GetActorRotation(), LookRot, DeltaTime, 5.f));
        }
    }
}


void ACombatManager::RegisterEnemy(AJediCharacterBase* Enemy)
{
    if (!Enemy || Enemies.Contains(Enemy)) return;
    Enemies.Add(Enemy);

    // If no active attacker, assign this one
    if (!ActiveAttacker)
    {
        ActivateNextAttacker();
    }
}

void ACombatManager::UnregisterEnemy(AJediCharacterBase* Enemy)
{
    Enemies.Remove(Enemy);
    DisabledEnemies.Remove(Enemy);

    if (ActiveAttacker == Enemy)
    {
        ActiveAttacker = nullptr;
        ActivateNextAttacker();
    }
}

void ACombatManager::OnEnemyDied(AJediCharacterBase* Enemy)
{
    UnregisterEnemy(Enemy);
}

void ACombatManager::ActivateNextAttacker()
{
    AJediCharacterBase* Next = PickNextAttacker();
    AssignAttacker(Next);
}

AJediCharacterBase* ACombatManager::PickNextAttacker() const
{
    // Pick the closest non-disabled enemy
    float BestDist = TNumericLimits<float>::Max();
    AJediCharacterBase* Best = nullptr;

    for (AJediCharacterBase* Enemy : Enemies)
    {
        if (!Enemy || DisabledEnemies.Contains(Enemy)) continue;

        float Dist = FVector::Dist(Enemy->GetActorLocation(), PlayerCharacter->GetActorLocation());
        if (Dist < BestDist)
        {
            BestDist = Dist;
            Best = Enemy;
        }
    }

    return Best;
}

void ACombatManager::AssignAttacker(AJediCharacterBase* NewAttacker)
{
    if (ActiveAttacker)
    {
        AAIController* OldAI = Cast<AAIController>(ActiveAttacker->GetController());
        if (OldAI && OldAI->GetBlackboardComponent())
        {
            OldAI->GetBlackboardComponent()->SetValueAsBool(FName("IsActiveAttacker"), false);
            OldAI->GetBlackboardComponent()->ClearValue(FName("TargetPlayer"));
        }
    }

    ActiveAttacker = NewAttacker;

    if (ActiveAttacker)
    {
        AAIController* NewAI = Cast<AAIController>(ActiveAttacker->GetController());
        if (NewAI && NewAI->GetBlackboardComponent())
        {
            NewAI->GetBlackboardComponent()->SetValueAsBool(FName("IsActiveAttacker"), true);
            NewAI->GetBlackboardComponent()->SetValueAsObject(FName("TargetPlayer"), PlayerCharacter);
            UE_LOG(LogTemp, Warning, TEXT("New active attacker: %s"), *ActiveAttacker->GetName());

            ActiveAttacker->FadeInVitals();
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No valid attacker found"));
    }
}

void ACombatManager::SetEnemyDisabled(AJediCharacterBase* Enemy, bool bDisabled)
{
    if (!Enemy) return;

    if (bDisabled)
    {
        DisabledEnemies.Add(Enemy);

        // If this was the active attacker, pick a new one
        if (ActiveAttacker == Enemy)
        {
            ActiveAttacker = nullptr;
            ActivateNextAttacker();
        }
    }
    else
    {
        DisabledEnemies.Remove(Enemy);

        // If no active attacker, this enemy can take over
        if (!ActiveAttacker)
        {
            ActivateNextAttacker();
        }
    }
}