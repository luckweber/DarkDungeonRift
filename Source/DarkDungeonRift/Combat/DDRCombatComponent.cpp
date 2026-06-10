// Copyright Epic Games, Inc. All Rights Reserved.

#include "DDRCombatComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "DDRAttributeSet.h"
#include "DDRGameplayTags.h"
#include "DDRHitStopSubsystem.h"
#include "DDRLog.h"
#include "GE_DDRDamage.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

static TAutoConsoleVariable<int32> CVarCombatDebug(
	TEXT("ddr.CombatDebug"),
	0,
	TEXT("1 = log combat hits, combo windows, and damage."),
	ECVF_Default);

UDDRCombatComponent::UDDRCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	DamageEffectClass = UGE_DDRDamage::StaticClass();
}

void UDDRCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bBufferedAttack)
	{
		BufferedAttackTimeRemaining -= DeltaTime;
		if (BufferedAttackTimeRemaining <= 0.f)
		{
			ClearBufferedAttack();
		}
	}
}

void UDDRCombatComponent::PerformMeleeSweep(const FDDRMeleeSweepParams& Params)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	const FVector Start = OwnerActor->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	const FVector Forward = OwnerActor->GetActorForwardVector();
	const FVector End = Start + Forward * (Params.SweepForwardOffset + Params.SweepReach);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(DDRMeleeSweep), false, OwnerActor);
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FHitResult> Hits;
	const bool bHit = GetWorld()->SweepMultiByObjectType(
		Hits,
		Start,
		End,
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeSphere(Params.SweepRadius),
		QueryParams);

	if (!bHit)
	{
		return;
	}

	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || !CanHitActor(HitActor))
		{
			continue;
		}

		if (HitActorsThisSwing.Contains(HitActor))
		{
			continue;
		}

		HitActorsThisSwing.Add(HitActor);
		ApplyDamageToTarget(HitActor, Params);
		SendHitEvent(HitActor);

		if (CVarCombatDebug.GetValueOnGameThread() > 0)
		{
			UE_LOG(LogDDR, Log, TEXT("Combat hit %s section=%d damage=%.1f"),
				*GetNameSafe(HitActor), Params.ComboSectionIndex, Params.BaseDamage);
		}
	}
}

void UDDRCombatComponent::OpenComboWindow()
{
	bComboWindowOpen = true;

	if (CVarCombatDebug.GetValueOnGameThread() > 0)
	{
		UE_LOG(LogDDR, Log, TEXT("Combo window OPEN on %s"), *GetNameSafe(GetOwner()));
	}
}

void UDDRCombatComponent::CloseComboWindow()
{
	bComboWindowOpen = false;

	if (CVarCombatDebug.GetValueOnGameThread() > 0)
	{
		UE_LOG(LogDDR, Log, TEXT("Combo window CLOSE on %s"), *GetNameSafe(GetOwner()));
	}
}

void UDDRCombatComponent::BufferAttackInput(float BufferSeconds)
{
	bBufferedAttack = true;
	BufferedAttackTimeRemaining = BufferSeconds;
}

void UDDRCombatComponent::ClearBufferedAttack()
{
	bBufferedAttack = false;
	BufferedAttackTimeRemaining = 0.f;
}

bool UDDRCombatComponent::HasBufferedAttack() const
{
	return bBufferedAttack && BufferedAttackTimeRemaining > 0.f;
}

void UDDRCombatComponent::ResetHitTracking()
{
	HitActorsThisSwing.Reset();
}

void UDDRCombatComponent::SetActiveComboSection(int32 SectionIndex)
{
	ActiveComboSection = SectionIndex;
}

float UDDRCombatComponent::GetHealthPercent() const
{
	if (UAbilitySystemComponent* ASC = GetOwnerASC())
	{
		const float Health = ASC->GetNumericAttribute(UDDRAttributeSet::GetHealthAttribute());
		const float MaxHealth = ASC->GetNumericAttribute(UDDRAttributeSet::GetMaxHealthAttribute());
		if (MaxHealth > KINDA_SMALL_NUMBER)
		{
			return Health / MaxHealth;
		}
	}

	return 1.f;
}

bool UDDRCombatComponent::CanHitActor(AActor* OtherActor) const
{
	if (!OtherActor || OtherActor == GetOwner())
	{
		return false;
	}

	if (UAbilitySystemComponent* TargetASC = OtherActor->FindComponentByClass<UAbilitySystemComponent>())
	{
		if (TargetASC->HasMatchingGameplayTag(DDRTags::State_Dead))
		{
			return false;
		}
	}

	return true;
}

void UDDRCombatComponent::ApplyDamageToTarget(AActor* TargetActor, const FDDRMeleeSweepParams& Params)
{
	UAbilitySystemComponent* SourceASC = GetOwnerASC();
	UAbilitySystemComponent* TargetASC = TargetActor ? TargetActor->FindComponentByClass<UAbilitySystemComponent>() : nullptr;
	if (!SourceASC || !TargetASC || !DamageEffectClass)
	{
		return;
	}

	float Damage = Params.BaseDamage;
	if (const float AttackPower = SourceASC->GetNumericAttribute(UDDRAttributeSet::GetAttackPowerAttribute()))
	{
		Damage *= (1.f + AttackPower * DamageAttackPowerScale);
	}

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddSourceObject(GetOwner());
	Context.AddInstigator(GetOwner(), GetOwner());

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.f, Context);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(DDRTags::Data_Damage, Damage);
	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

	if (UWorld* World = GetWorld())
	{
		if (UDDRHitStopSubsystem* HitStop = World->GetSubsystem<UDDRHitStopSubsystem>())
		{
			HitStop->RequestHitStop(Params.HitStopFrames);
		}
	}

	if (SourceASC->IsValidLowLevel())
	{
		FGameplayCueParameters CueParams;
		CueParams.Instigator = GetOwner();
		CueParams.EffectCauser = GetOwner();
		CueParams.Location = TargetActor->GetActorLocation();
		SourceASC->ExecuteGameplayCue(DDRTags::Cue_Hit_Light, CueParams);
	}
}

void UDDRCombatComponent::SendHitEvent(AActor* HitActor) const
{
	FGameplayEventData EventData;
	EventData.Instigator = GetOwner();
	EventData.Target = HitActor;
	EventData.EventTag = DDRTags::Event_Combat_Hit;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetOwner(), DDRTags::Event_Combat_Hit, EventData);
}

UAbilitySystemComponent* UDDRCombatComponent::GetOwnerASC() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UAbilitySystemComponent>() : nullptr;
}
