// CombatManager.cpp
#include "CombatManager.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"

ACombatManager::ACombatManager()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ACombatManager::BeginPlay()
{
    Super::BeginPlay();
    PlayerCharacter = UGameplayStatics::GetPlayerPawn(this, 0);
    SpawnNextEnemy();
}

void ACombatManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (!PlayerCharacter) return;

    for (AJediCharacterBase* Enemy : Enemies)
    {
        if (!Enemy || DisabledEnemies.Contains(Enemy)) continue;

        // Rotate all enemies toward the player
        FVector DirToPlayer = PlayerCharacter->GetActorLocation() - Enemy->GetActorLocation();
        DirToPlayer.Z = 0.f;
        if (!DirToPlayer.IsNearlyZero())
        {
            FRotator LookRot = DirToPlayer.Rotation();
            Enemy->SetActorRotation(FMath::RInterpTo(Enemy->GetActorRotation(), LookRot, DeltaTime, 5.f));
        }

        // Vitals: only active attacker in range shows vitals
        if (Enemy == ActiveAttacker)
        {
            float Dist = FVector::Dist(Enemy->GetActorLocation(), PlayerCharacter->GetActorLocation());
            bool bInRange = Dist <= VitalsShowRange;
            bool bCurrentlyVisible = EnemiesWithVitalsVisible.Contains(Enemy);

            if (bInRange && !bCurrentlyVisible)
            {
                Enemy->FadeInVitals();
                EnemiesWithVitalsVisible.Add(Enemy);
            }
            else if (!bInRange && bCurrentlyVisible)
            {
                Enemy->FadeOutVitals();
                EnemiesWithVitalsVisible.Remove(Enemy);
            }
        }
        else if (EnemiesWithVitalsVisible.Contains(Enemy))
        {
            // Not the active attacker but vitals still showing — fade out
            Enemy->FadeOutVitals();
            EnemiesWithVitalsVisible.Remove(Enemy);
        }
    }
}

void ACombatManager::SpawnNextEnemy()
{
    if (!EnemyClass || !PlayerCharacter) return;

    FVector SpawnLoc = FindSpawnLocation();
    FRotator SpawnRot = (PlayerCharacter->GetActorLocation() - SpawnLoc).Rotation();
    SpawnRot.Pitch = 0.f;
    SpawnRot.Roll = 0.f;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AJediCharacterBase* NewEnemy = GetWorld()->SpawnActor<AJediCharacterBase>(EnemyClass, SpawnLoc, SpawnRot, Params);
    if (NewEnemy)
    {
        RegisterEnemy(NewEnemy);
        UE_LOG(LogTemp, Warning, TEXT("CombatManager: Spawned %s at %s"), *NewEnemy->GetName(), *SpawnLoc.ToString());
    }
}

FVector ACombatManager::FindSpawnLocation() const
{
    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
    if (NavSys && PlayerCharacter)
    {
        for (int32 Attempt = 0; Attempt < 10; ++Attempt)
        {
            float Angle = FMath::RandRange(0.f, 360.f);
            float Dist = FMath::RandRange(SpawnRadiusMin, SpawnRadiusMax);
            FVector Candidate = PlayerCharacter->GetActorLocation()
                + FVector(FMath::Cos(FMath::DegreesToRadians(Angle)) * Dist,
                          FMath::Sin(FMath::DegreesToRadians(Angle)) * Dist,
                          0.f);

            FNavLocation NavLoc;
            if (NavSys->ProjectPointToNavigation(Candidate, NavLoc, FVector(100.f, 100.f, 200.f)))
                return NavLoc.Location;
        }
    }

    float Angle = FMath::RandRange(0.f, 360.f);
    float Dist = FMath::RandRange(SpawnRadiusMin, SpawnRadiusMax);
    return PlayerCharacter->GetActorLocation()
        + FVector(FMath::Cos(FMath::DegreesToRadians(Angle)) * Dist,
                  FMath::Sin(FMath::DegreesToRadians(Angle)) * Dist,
                  0.f);
}

void ACombatManager::RegisterEnemy(AJediCharacterBase* Enemy)
{
    if (!Enemy || Enemies.Contains(Enemy)) return;
    Enemies.Add(Enemy);

    if (!ActiveAttacker)
        ActivateNextAttacker();
}

void ACombatManager::UnregisterEnemy(AJediCharacterBase* Enemy)
{
    Enemies.Remove(Enemy);
    DisabledEnemies.Remove(Enemy);
    EnemiesWithVitalsVisible.Remove(Enemy);

    if (ActiveAttacker == Enemy)
    {
        ActiveAttacker = nullptr;
        ActivateNextAttacker();
    }
}

void ACombatManager::OnEnemyDied(AJediCharacterBase* Enemy)
{
    UnregisterEnemy(Enemy);
    SpawnNextEnemy();
}

void ACombatManager::ActivateNextAttacker()
{
    AssignAttacker(PickNextAttacker());
}

AJediCharacterBase* ACombatManager::PickNextAttacker() const
{
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
        if (EnemiesWithVitalsVisible.Contains(ActiveAttacker))
        {
            ActiveAttacker->FadeOutVitals();
            EnemiesWithVitalsVisible.Remove(ActiveAttacker);
        }

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
            UE_LOG(LogTemp, Warning, TEXT("CombatManager: New active attacker: %s"), *ActiveAttacker->GetName());
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("CombatManager: No valid attacker found"));
    }
}

void ACombatManager::SetEnemyDisabled(AJediCharacterBase* Enemy, bool bDisabled)
{
    if (!Enemy) return;

    if (bDisabled)
    {
        DisabledEnemies.Add(Enemy);
        if (ActiveAttacker == Enemy)
        {
            ActiveAttacker = nullptr;
            ActivateNextAttacker();
        }
    }
    else
    {
        DisabledEnemies.Remove(Enemy);
        if (!ActiveAttacker)
            ActivateNextAttacker();
    }
}