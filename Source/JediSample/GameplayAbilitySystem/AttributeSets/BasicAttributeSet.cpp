// Fill out your copyright notice in the Description page of Project Settings.


#include "BasicAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UBasicAttributeSet::UBasicAttributeSet()
{
	Health = 100.f;
	MaxHealth = 100.f;
	Stamina = 100.f;
	MaxStamina = 100.f;
	Break = 100.f;      
	MaxBreak = 100.f;   
}

void UBasicAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	//Replicate to everyone and whenever the value is received
	DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, Break, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, MaxBreak, COND_None, REPNOTIFY_Always);
}

void UBasicAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue,0.f,GetMaxHealth());
	}
	else if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue,0.f,GetMaxStamina());
	}
	else if (Attribute == GetBreakAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxBreak());
	}
}

void UBasicAttributeSet::PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		//Clamp Health
		SetHealth(GetHealth());

		if (Data.EffectSpec.Def->GetAssetTags().HasTag(FGameplayTag::RequestGameplayTag("Effects.HitReaction")))
		{
			FGameplayTagContainer HitReactionTagContainer;
			HitReactionTagContainer.AddTag(FGameplayTag::RequestGameplayTag("GameplayAbility.HitReaction"));
			GetOwningAbilitySystemComponent()->TryActivateAbilitiesByTag(HitReactionTagContainer);
		}
		
		
	}
	else if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		//Clamp Stamina
		SetStamina(GetStamina());
	}

	else if (Data.EvaluatedData.Attribute == GetBreakAttribute())
	{
		float OverflowDamage = 0.f;
    
		// If break goes below 0, store the overflow
		if (GetBreak() < 0.f)
		{
			OverflowDamage = FMath::Abs(GetBreak());
			SetBreak(0.f); // Clamp to 0
		}
    
		// Apply overflow damage to health
		if (OverflowDamage > 0.f)
		{
			SetHealth(GetHealth() - OverflowDamage);
		}
	}
}

void UBasicAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetHealthAttribute() && NewValue <= 0.f)
	{
		FGameplayTagContainer DeathAbilityTagContainer;
		DeathAbilityTagContainer.AddTag(FGameplayTag::RequestGameplayTag("GameplayAbility.Death"));
		GetOwningAbilitySystemComponent()->TryActivateAbilitiesByTag(DeathAbilityTagContainer);
	}

	else if (Attribute == GetBreakAttribute() && NewValue <= 0.f && OldValue > 0.f)
	{
		// Break bar broken
		FGameplayTagContainer BreakAbilityTagContainer;
		BreakAbilityTagContainer.AddTag(FGameplayTag::RequestGameplayTag("GameplayAbility.Broken"));
		GetOwningAbilitySystemComponent()->TryActivateAbilitiesByTag(BreakAbilityTagContainer);
    
		// Grant broken state tag
		FGameplayTag BrokenTag = FGameplayTag::RequestGameplayTag("State.Broken");
		GetOwningAbilitySystemComponent()->AddLooseGameplayTag(BrokenTag);
	}
}

