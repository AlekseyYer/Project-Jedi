// Fill out your copyright notice in the Description page of Project Settings.


#include "DamageExecutionCalculation.h"
#include "AbilitySystemComponent.h"
#include "AttributeSets/BasicAttributeSet.h"


// Declare attributes we'll capture
struct FDamageStatics
{
    DECLARE_ATTRIBUTE_CAPTUREDEF(Break);
    DECLARE_ATTRIBUTE_CAPTUREDEF(Health);

    FDamageStatics()
    {
        // Capture Break from target
        DEFINE_ATTRIBUTE_CAPTUREDEF(UBasicAttributeSet, Break, Target, false);
        // Capture Health from target
        DEFINE_ATTRIBUTE_CAPTUREDEF(UBasicAttributeSet, Health, Target, false);
    }
};

static const FDamageStatics& DamageStatics()
{
    static FDamageStatics Statics;
    return Statics;
}

UDamageExecutionCalculation::UDamageExecutionCalculation()
{
    RelevantAttributesToCapture.Add(DamageStatics().BreakDef);
    RelevantAttributesToCapture.Add(DamageStatics().HealthDef);
}

void UDamageExecutionCalculation::Execute_Implementation(
    const FGameplayEffectCustomExecutionParameters& ExecutionParams,
    FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
    const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
    
    UE_LOG(LogTemp, Warning, TEXT("DamageExecutionCalculation: Execute called!"));
    
    // Get damage values from SetByCaller
    float BreakDamage = Spec.GetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag("Data.BreakDamage"), false, 0.f);
    float HealthDamage = Spec.GetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag("Data.HealthDamage"), false, 0.f);

    UE_LOG(LogTemp, Warning, TEXT("BreakDamage: %f, HealthDamage: %f"), BreakDamage, HealthDamage);

    // Get current Break value
    float CurrentBreak = 0.f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().BreakDef, FAggregatorEvaluateParameters(), CurrentBreak);

    
    // If break bar has value, damage it
    if (CurrentBreak > 0.f && BreakDamage != 0.f)
    {
        OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
            DamageStatics().BreakProperty, EGameplayModOp::Additive, BreakDamage));
    
        UE_LOG(LogTemp, Warning, TEXT("Damaging Break: %f"), BreakDamage);
    }
    // If break bar is already 0, damage health instead
    else if (CurrentBreak <= 0.f && HealthDamage != 0.f)
    {
        OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
            DamageStatics().HealthProperty, EGameplayModOp::Additive, HealthDamage));
    
        UE_LOG(LogTemp, Warning, TEXT("Break depleted. Damaging Health: %f"), HealthDamage);
    }
}