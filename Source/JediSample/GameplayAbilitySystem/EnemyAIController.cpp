// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"

AEnemyAIController::AEnemyAIController()
{
	Blackboard = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));
	BrainComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComponent"));
	SetPerceptionComponent(*CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent")));
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (Blackboard && BlackboardData)
	{
		Blackboard->InitializeBlackboard(*BlackboardData);
	}

	if (BehaviorTree)
	{
		RunBehaviorTree(BehaviorTree);
	}
}

void AEnemyAIController::OnUnPossess()
{
	Super::OnUnPossess();
	if (BrainComponent)
	{
		BrainComponent->StopLogic(TEXT("Unpossessed"));
	}
}

