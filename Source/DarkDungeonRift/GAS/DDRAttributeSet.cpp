// Copyright Epic Games, Inc. All Rights Reserved.

#include "DDRAttributeSet.h"

#include "DDRGameplayTags.h"
#include "DDRLog.h"
#include "GameplayEffectExtension.h"
#include "TimerManager.h"

UDDRAttributeSet::UDDRAttributeSet()
{
	InitHealth(100.f);
	InitMaxHealth(100.f);
	InitStamina(100.f);
	InitMaxStamina(100.f);
	InitAttackPower(100.f);
	InitPoise(0.f);
	InitPoiseMax(0.f);
	InitIncomingDamage(0.f);
	InitIncomingPoiseDamage(0.f);
}

void UDDRAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxStamina());
	}
	else if (Attribute == GetPoiseAttribute())
	{
		const float MaxPoise = GetPoiseMax();
		if (MaxPoise > KINDA_SMALL_NUMBER)
		{
			NewValue = FMath::Clamp(NewValue, 0.f, MaxPoise);
		}
	}
}

void UDDRAttributeSet::NotifyPoiseHit()
{
	TimeSinceLastPoiseHit = 0.f;
}

void UDDRAttributeSet::TickPoiseRegen(const float DeltaSeconds)
{
	if (GetPoiseMax() <= KINDA_SMALL_NUMBER || GetPoise() >= GetPoiseMax())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	if (!ASC || ASC->HasMatchingGameplayTag(DDRTags::State_Combat_Stagger))
	{
		return;
	}

	TimeSinceLastPoiseHit += DeltaSeconds;
	if (TimeSinceLastPoiseHit < PoiseRegenDelaySeconds || PoiseRegenPerSecond <= 0.f)
	{
		return;
	}

	SetPoise(FMath::Min(GetPoiseMax(), GetPoise() + PoiseRegenPerSecond * DeltaSeconds));
}

void UDDRAttributeSet::HandlePoiseBreak(UAbilitySystemComponent* ASC)
{
	if (!ASC || GetPoiseMax() <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	ASC->AddLooseGameplayTag(DDRTags::State_Combat_Stagger);
	SetPoise(GetPoiseMax());
	NotifyPoiseHit();

	FGameplayEventData EventData;
	EventData.EventTag = DDRTags::Event_Combat_PoiseBreak;
	ASC->HandleGameplayEvent(DDRTags::Event_Combat_PoiseBreak, &EventData);

	UE_LOG(LogDDR, Log, TEXT("[POISE] quebra -> stagger %.2fs"), StaggerDurationSeconds);

	if (UWorld* World = ASC->GetWorld())
	{
		World->GetTimerManager().SetTimer(
			StaggerTimerHandle,
			[ASC]()
			{
				if (ASC)
				{
					ASC->RemoveLooseGameplayTag(DDRTags::State_Combat_Stagger);
				}
			},
			StaggerDurationSeconds,
			false);
	}
}

void UDDRAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float LocalDamage = GetIncomingDamage();
		SetIncomingDamage(0.f);

		if (LocalDamage > 0.f)
		{
			SetHealth(FMath::Clamp(GetHealth() - LocalDamage, 0.f, GetMaxHealth()));
		}

		if (GetHealth() <= 0.f)
		{
			if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
			{
				ASC->AddLooseGameplayTag(DDRTags::State_Dead);
			}
		}
	}
	else if (Data.EvaluatedData.Attribute == GetIncomingPoiseDamageAttribute())
	{
		const float LocalPoiseDamage = GetIncomingPoiseDamage();
		SetIncomingPoiseDamage(0.f);

		if (LocalPoiseDamage > 0.f && GetPoiseMax() > KINDA_SMALL_NUMBER)
		{
			NotifyPoiseHit();
			const float NewPoise = FMath::Max(0.f, GetPoise() - LocalPoiseDamage);
			SetPoise(NewPoise);

			if (NewPoise <= KINDA_SMALL_NUMBER)
			{
				if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
				{
					HandlePoiseBreak(ASC);
				}
			}
		}
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
	}
}
