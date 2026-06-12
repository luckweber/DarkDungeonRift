// Copyright Epic Games, Inc. All Rights Reserved.

#include "DDRProjectileBase.h"

#include "DDRCombatComponent.h"
#include "DDRGameplayTags.h"
#include "DDRLog.h"
#include "DDRProjectileData.h"
#include "DDRProjectilePoolSubsystem.h"
#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GE_DDRDamage.h"
#include "GE_DDRPoiseDamage.h"
#include "Kismet/GameplayStatics.h"

ADDRProjectileBase::ADDRProjectileBase()
{
	PrimaryActorTick.bCanEverTick = false;

	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	SetRootComponent(Sphere);
	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Sphere->SetCollisionProfileName(TEXT("Projectile"));
	Sphere->SetGenerateOverlapEvents(true);

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->UpdatedComponent = Sphere;
	Movement->bRotationFollowsVelocity = true;
	Movement->ProjectileGravityScale = 0.f;
	Movement->bShouldBounce = false;

	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

void ADDRProjectileBase::BeginPlay()
{
	Super::BeginPlay();
	Sphere->OnComponentBeginOverlap.AddDynamic(this, &ADDRProjectileBase::OnSphereOverlap);
}

void ADDRProjectileBase::InitProjectile(UDDRProjectileData* InData, AActor* InInstigator, const FVector& LaunchDirection)
{
	ProjectileData = InData;
	SourceActor = InInstigator;
	PierceCount = 0;
	bActive = true;

	SetOwner(InInstigator);
	SetInstigator(Cast<APawn>(InInstigator));

	const float Radius = InData ? InData->HitRadius : 12.f;
	const float Speed = InData ? InData->Speed : 2400.f;

	Sphere->SetSphereRadius(Radius);
	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);

	Movement->ProjectileGravityScale = 0.f;
	Movement->Velocity = LaunchDirection.GetSafeNormal() * Speed;
	Movement->Activate(true);

	if (InData && InData->Lifetime > 0.f)
	{
		GetWorldTimerManager().SetTimer(LifetimeTimerHandle, this, &ADDRProjectileBase::ExpireProjectile, InData->Lifetime, false);
	}

	if (InData && InData->FireSFX)
	{
		UGameplayStatics::PlaySoundAtLocation(this, InData->FireSFX, GetActorLocation());
	}
}

void ADDRProjectileBase::DeactivateProjectile()
{
	bActive = false;
	GetWorldTimerManager().ClearTimer(LifetimeTimerHandle);
	Movement->StopMovementImmediately();
	Movement->Deactivate();
	Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	ProjectileData = nullptr;
	SourceActor.Reset();

	if (UDDRProjectilePoolSubsystem* Pool = GetWorld()->GetSubsystem<UDDRProjectilePoolSubsystem>())
	{
		Pool->Release(this);
	}
	else
	{
		Destroy();
	}
}

void ADDRProjectileBase::ExpireProjectile()
{
	DeactivateProjectile();
}

void ADDRProjectileBase::OnSphereOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!bActive || !OtherActor || OtherActor == GetOwner() || OtherActor == GetInstigator())
	{
		return;
	}

	if (SourceActor.IsValid())
	{
		if (const UDDRCombatComponent* SourceCombat = SourceActor->FindComponentByClass<UDDRCombatComponent>())
		{
			if (!SourceCombat->CanHitActor(OtherActor))
			{
				return;
			}
		}
		else
		{
			const UAbilitySystemComponent* SourceASC = SourceActor->FindComponentByClass<UAbilitySystemComponent>();
			const UAbilitySystemComponent* TargetASC = OtherActor->FindComponentByClass<UAbilitySystemComponent>();
			if (SourceASC && TargetASC
				&& SourceASC->HasMatchingGameplayTag(DDRTags::Faction_Enemy)
				&& TargetASC->HasMatchingGameplayTag(DDRTags::Faction_Enemy))
			{
				return;
			}
		}
	}

	ApplyHit(OtherActor);
}

void ADDRProjectileBase::ApplyHit(AActor* HitActor)
{
	if (!ProjectileData || !HitActor)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = SourceActor.IsValid()
		? SourceActor->FindComponentByClass<UAbilitySystemComponent>()
		: nullptr;
	UAbilitySystemComponent* TargetASC = HitActor->FindComponentByClass<UAbilitySystemComponent>();

	if (SourceASC && TargetASC)
	{
		TSubclassOf<UGameplayEffect> DamageClass = ProjectileData->DamageEffect;
		if (!DamageClass)
		{
			DamageClass = UGE_DDRDamage::StaticClass();
		}

		FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
		Context.AddSourceObject(SourceActor.Get());
		Context.AddInstigator(SourceActor.Get(), SourceActor.Get());

		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageClass, 1.f, Context);
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(DDRTags::Data_Damage, ProjectileData->BaseDamage);
			SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		}

		if (ProjectileData->PoiseDamage > 0.f)
		{
			FGameplayEffectSpecHandle PoiseSpec = SourceASC->MakeOutgoingSpec(UGE_DDRPoiseDamage::StaticClass(), 1.f, Context);
			if (PoiseSpec.IsValid())
			{
				PoiseSpec.Data->SetSetByCallerMagnitude(DDRTags::Data_PoiseDamage, ProjectileData->PoiseDamage);
				SourceASC->ApplyGameplayEffectSpecToTarget(*PoiseSpec.Data.Get(), TargetASC);
			}
		}

		FGameplayCueParameters CueParams;
		CueParams.Instigator = SourceActor.Get();
		CueParams.Location = HitActor->GetActorLocation();
		if (ProjectileData->ImpactCue.IsValid())
		{
			SourceASC->ExecuteGameplayCue(ProjectileData->ImpactCue, CueParams);
		}
		else
		{
			SourceASC->ExecuteGameplayCue(DDRTags::Cue_Proj_Impact, CueParams);
		}
	}

	if (ProjectileData->ImpactSFX)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ProjectileData->ImpactSFX, HitActor->GetActorLocation());
	}

	++PierceCount;
	if (!ProjectileData->bPierce || PierceCount > ProjectileData->MaxPierceCount)
	{
		DeactivateProjectile();
	}
}
