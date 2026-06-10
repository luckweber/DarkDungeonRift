// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DDRGameplayAbility.h"
#include "GA_Dash.generated.h"

class UAnimMontage;

UCLASS()
class DARKDUNGEONRIFT_API UGA_Dash : public UDDRGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Dash();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	UFUNCTION()
	void OnRootMotionFinished();

	UFUNCTION()
	void OnDashMontageFinished();

	FVector GetDashDirection() const;
	FName ComputeDodgeSectionName(const FVector& DashDirection) const;
	void PlayVisualDodgeMontage(FName SectionName);
	static FName GetDirectionalSectionName(float AngleDegrees);

	// Montage de dodge direcional — 8 seções (F, FR, R, BR, B, BL, L, FL), SEM links entre
	// seções (Montage Sections -> Clear, doc 57). Opcional: sem montage o dash é só deslocamento.
	UPROPERTY(EditDefaultsOnly, Category = "DDR|Dash")
	TObjectPtr<UAnimMontage> DashMontage;

	// true = gira o personagem pra direção do dash e toca sempre a seção F ("modo Hades", 1 clip).
	// false (default) = mantém o facing e escolhe a seção pelo ângulo (dodge direcional de combate).
	UPROPERTY(EditDefaultsOnly, Category = "DDR|Dash")
	bool bFaceDashDirection = false;

	// Modo clip-dirigido (clips COM root motion): escala o deslocamento autorado.
	// Gancho p/ Eco "dash +X% de distância" sem reautorar clips.
	UPROPERTY(EditDefaultsOnly, Category = "DDR|Dash", meta = (ClampMin = "0.1"))
	float RootMotionTranslationScale = 1.f;

	// Família "dodge -> corrida": se o jogador está SEGURANDO movimento ao dashar e a
	// montage tem a seção "<dir><sufixo>" (ex.: "F_Run"), usa-a — o dodge termina
	// emendando na corrida (clips 06_Dodge_Combat_to_Run). Sem a seção, cai na base.
	UPROPERTY(EditDefaultsOnly, Category = "DDR|Dash")
	FName RunSectionSuffix = TEXT("_Run");

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Dash")
	float DashDistance = 550.f;

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Dash", meta = (ClampMin = "0.05"))
	float DashDuration = 0.22f;

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Dash")
	TSubclassOf<UGameplayEffect> IFrameEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "DDR|Dash")
	TSubclassOf<UGameplayEffect> CooldownEffectClass;

private:
	// Trava de rotação durante o dodge (restaurada no EndAbility).
	bool bSavedOrientRotationToMovement = false;
	bool bModifiedRotationFlag = false;
};
