
#include "FireFireS.h"
#include "CommonAncestor.h"
UFireFireS::UFireFireS()
{
	BuffCount = 0;
	EffectTime = 2.0f;
	IsPermanent = false;
	IsPositive = true;
	Dispellable = true;
	Name = NameEffects::FireFireS;
}

void UFireFireS::Apply(float Delta)
{
	Target->RealA["AttackSpeed"] += BuffCount * 10;
	Target->RealM["CastTime"] /= (1.0f + 0.05*BuffCount);
	Target->RealM["CoolDownTime"] /= 1.0 + 0.04 * BuffCount;
	EffectTime -= Delta;
}
void UFireFireS::IncrementEffect(int i)
{
	BuffCount = (Target->SkillLevel[(int32)Skill::FireFire - (int32)Skill::Fire_Start] > BuffCount + i) ? BuffCount + i : Target->SkillLevel[(int32)Skill::FireFire - (int32)Skill::Fire_Start];
	EffectTime = 2.0f;
	if (Model == nullptr)
	{
		Model = Target->GetWorld()->SpawnActor<AActor>(VRepr, FVector(0.0f), FRotator::ZeroRotator);
		if (Model != nullptr)
			Model->AttachToComponent(Target->GetMesh(), FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), FName(TEXT("Hand_R_001")));	
	}
	if (Model != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("%d this scale %f"), BuffCount, BuffCount / 5.0f);
		Model->SetActorRelativeScale3D(FVector(BuffCount / 5.0f));
	}
}