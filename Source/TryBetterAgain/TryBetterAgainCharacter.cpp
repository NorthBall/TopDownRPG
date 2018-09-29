// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "TryBetterAgainCharacter.h"
#include "Components/SphereComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Materials/Material.h"
#include "MyAIController.h"
#include "AI.h"
#include "Effects.h"
#include "FireLance.h"
#include "FirePrimitive.h"
#include "FireMeteor.h"
#include "MyFireAura.h"
#include "TryBetterAgainPlayerController.h"
#define maxi(a,b) ((a)<(b)?(b):(a))
#define maxskills 10
#define mini(a,b) ((a)<(b)?(a):(b))

ATryBetterAgainCharacter::ATryBetterAgainCharacter():ACommonAncestor()
{
	// Set size for player capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Rotate character to moving direction
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	// Create a camera boom...
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->bAbsoluteRotation = true; // Don't want arm to rotate when character does
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->RelativeRotation = FRotator(-60.f, 0.f, 0.f);
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level

	// Create a camera...
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm


	// Create a decal in the world to show the cursor's location
	CursorToWorld = CreateDefaultSubobject<UDecalComponent>("CursorToWorld");
	CursorToWorld->SetupAttachment(RootComponent);
	static ConstructorHelpers::FObjectFinder<UMaterial> DecalMaterialAsset(TEXT("Material'/Game/TopDownCPP/Blueprints/M_Cursor_Decal.M_Cursor_Decal'"));
	if (DecalMaterialAsset.Succeeded())
	{
		CursorToWorld->SetDecalMaterial(DecalMaterialAsset.Object);
	}
	CursorToWorld->DecalSize = FVector(16.0f, 32.0f, 32.0f);
	CursorToWorld->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f).Quaternion());

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	CameraSpeed = 4;
	CameraUp = 1500;
	CameraDown = 600;


	FireEffectAura = nullptr;
}
void ATryBetterAgainCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TickExample(DeltaSeconds);
	if (CursorToWorld != nullptr)
	{
		if (UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled())
		{
			if (UWorld* World = GetWorld())
			{
				FHitResult HitResult;
				FCollisionQueryParams Params(NAME_None, FCollisionQueryParams::GetUnknownStatId());
				FVector StartLocation = TopDownCameraComponent->GetComponentLocation();
				FVector EndLocation = TopDownCameraComponent->GetComponentRotation().Vector() * 2000.0f;
				Params.AddIgnoredActor(this);
				World->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, Params);
				FQuat SurfaceRotation = HitResult.ImpactNormal.ToOrientationRotator().Quaternion();
				CursorToWorld->SetWorldLocationAndRotation(HitResult.Location, SurfaceRotation);
			}
		}
		else if (RealController)
		{
			FHitResult TraceHitResult;
			RealController->GetHitResultUnderCursor(ECC_Visibility, true, TraceHitResult);
			FVector CursorFV = TraceHitResult.ImpactNormal;
			FRotator CursorR = CursorFV.Rotation();
			CursorToWorld->SetWorldLocation(TraceHitResult.Location);
			CursorToWorld->SetWorldRotation(CursorR);
		}
	}
	
}

bool ATryBetterAgainCharacter::FacedToEnemy(FVector enemyLocation)
{
	FRotator deltaRotate = (enemyLocation - GetActorLocation()).Rotation();
	deltaRotate.Pitch = 0;
	SetActorRotation(deltaRotate);
	return true;
}



/*void ATryBetterAgainCharacter::NoZoom()
{
	bZooming = 0;
}*/


void ATryBetterAgainCharacter::DoAttack(ACommonAncestor* Victim)
{
	Victim->Health -= (AttackDamage*(int)(100 * Victim->PhysicMultiplier)) / 100;
	Victim->UpdateHealthBar();//may be deleted
	if (Victim->Health <= 0) Victim->Dead();
	else {
		FireBurn(Victim);
	}
	FireFire();
	
}
Effects* ATryBetterAgainCharacter::FireFire(int32 i)
{
	Effects* BuffEffect = FindName(NameEffects::FireFireS);
	/*if (BuffEffect == nullptr)
	{
		BuffEffect = AddNewEffect(false, false, true, NameEffects::FireFireS, 2.0f);
		BuffEffect->IsSingle = true;
		BuffEffect->SpecInt = mini(i, SkillLevel[(int32)Skill::FireFire - (int32)Skill::Fire_Start]);
		BuffEffect->AttackSpeedA = 10 * BuffEffect->SpecInt;
		BuffEffect->CastTimeM = 100.0 / (100.0 + 5 * BuffEffect->SpecInt);
		BuffEffect->CoolDownTimeM = 100.0 / (100.0 + 4 * BuffEffect->SpecInt);
	}
	else
	{
		BuffEffect->SpecInt = mini(BuffEffect->SpecInt + i, SkillLevel[(int32)Skill::FireFire - (int32)Skill::Fire_Start]);
		BuffEffect->AttackSpeedA = 10 * BuffEffect->SpecInt;
		BuffEffect->CastTimeM = 100.0 / (100.0 + 5 * BuffEffect->SpecInt);
		BuffEffect->CoolDownTimeM = 100.0 / (100.0 + 4 * BuffEffect->SpecInt);
		BuffEffect->EffectTime = 2.0f;
	}
	*/
	return BuffEffect;
}
Effects* ATryBetterAgainCharacter::FireBurn(ACommonAncestor * Victim)
{

	Effects* BurnEffect = Victim->FindName(NameEffects::FireBurnE);
	/*if (BurnEffect == nullptr)
	{
		BurnEffect = Victim->AddNewEffect(false, false, false, NameEffects::FireBurnE, 2.0f);
		BurnEffect->IsSingle = false;
		BurnEffect->SpecInt = mini(1, SkillLevel[(int32)Skill::FireBurn - (int32)Skill::Fire_Start]);
		BurnEffect->TickMHealthA = -BurnEffect->SpecInt;
	}
	else
	{
		BurnEffect->SpecInt = mini(BurnEffect->SpecInt + 1, SkillLevel[(int32)Skill::FireBurn - (int32)Skill::Fire_Start]);
		BurnEffect->TickMHealthA = -BurnEffect->SpecInt;
		BurnEffect->EffectTime = 2.0f;
	}*/
	return BurnEffect;
}
Effects* ATryBetterAgainCharacter::FireAfterBurn(ACommonAncestor *Victim, int32 Damage)
{
	int32 time = 2;	
	Effects* BurnEffect = Victim->AddNewEffect(false, false, false, NameEffects::FireAfterBurnE, (float)time);
	if (BurnEffect != nullptr) {
		BurnEffect->IsSingle = false;
//		BurnEffect->TickMHealthA = -(Damage*SkillLevel[(int32)Skill::FireAfterBurn - (int32)Skill::Fire_Start]) / (10 * time * 4);
	}
 return BurnEffect;
}
void ATryBetterAgainCharacter::FireBlink(FHitResult Hit)
{
	int32 SkillNum = (int32)Skill::FireBlink - (int32)Skill::Fire_Start;
	
	if (SkillCDTimes[SkillNum] == 0.0f&&SkillLevel[SkillNum] != 0)
	{

		UE_LOG(LogTemp, Warning, TEXT("vizov"));
		if (FVector::Dist2D(Hit.ImpactPoint, GetActorLocation())<(500 + MagicRangeA)*MagicRangeM)
		{
			bool IsLegal = false;
			int32 i, n;
			float CollisionRange = (150 + MagicRangeA)*MagicRangeM;
			TArray<FOverlapResult> All;
			AAI *Target;
			Effects *Legalization;
			GetWorld()->OverlapMultiByObjectType(All, Hit.ImpactPoint, FQuat(), ECollisionChannel::ECC_Pawn, FCollisionShape::MakeSphere(CollisionRange));
			n = All.Num();
			for (i = 0; i < n; i++)
			{
				Target = Cast<AAI>(All[i].GetActor());
				if (Target != nullptr)
				{
					Legalization=Target->FindName(NameEffects::FireBurnE);
					if (Legalization != nullptr)
					{
						IsLegal = true;
						break;
					}
				}

			}
			All.Empty();
			if (IsLegal)
			{
				FacedToEnemy(Hit.ImpactPoint);
				SetActorLocation(Hit.ImpactPoint + FVector(0, 0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
				FireFire();
				Effects* OursEffect;
				Effects* BurnEffect;
				int32 Damage;
				float Range = (300 + MagicRangeA)*MagicRangeM;

				GetWorld()->OverlapMultiByObjectType(All, GetActorLocation(), FQuat(), ECollisionChannel::ECC_Pawn, FCollisionShape::MakeSphere(Range));
				n = All.Num();
				UE_LOG(LogTemp, Warning, TEXT("SpellCasted range= %f, actors= %d"), Range, n);
				for (i = 0; i < n; i++)
				{
					Target = Cast<AAI>(All[i].GetActor());
					if (Target != nullptr)
					{

						Damage= (80*SkillLevel[(int32)Skill::FireBlink-(int32)Skill::Fire_Start] + MagicPowerA)*MagicPowerM*Target->MagicMultiplierM;
						Target->Health -= Damage;
						OursEffect=FireAfterBurn(Target, Damage);
						/*OursEffect = (Target->AddNewEffect(false, false, false, NameEffects::FireBlinkE, 4.0f));
						OursEffect->TickHealthA = -7;
						OursEffect->IsSingle = false;*/

						BurnEffect = Target->FindName(NameEffects::FireBurnE);
						/*if (BurnEffect != nullptr)
						{
							Health += BurnEffect->SpecInt * 10;
							FireFire(BurnEffect->SpecInt);
							Target->DeleteEffect(BurnEffect);
						}
						if(OursEffect!=nullptr) Target->CalcOneEffect(OursEffect, 0);*/
						Target->UpdateHealthBar();
						if (Target->Health <= 0) Target->Dead();
					}
				}

				UpdateAll();//this/self

			}
		}
		SkillCDTimes[SkillNum] = (5.0f - CoolDownTimeA) / CoolDownTimeM;
			}
			
}
void ATryBetterAgainCharacter::FireMeteor(FHitResult Hit)
{

	int32 SkillNum = (int32)Skill::FireMeteor - (int32)Skill::Fire_Start;

	UE_LOG(LogTemp, Warning, TEXT("vizov"));
	FacedToEnemy(Hit.ImpactPoint);
	FRotator deltaRotate = (Hit.ImpactPoint - GetActorLocation() + FVector(0, 0, GetActorLocation().Z - Hit.ImpactPoint.Z)).Rotation();
	FVector location = FVector(Hit.ImpactPoint.X,Hit.ImpactPoint.Y,GetActorLocation().Z);
	FActorSpawnParameters a;
	a.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AFireMeteor* Meteor = GetWorld()->SpawnActor<AFireMeteor>(AFireMeteor::StaticClass(), location, deltaRotate);
	//Meteor->SetActorLocation(location);
	if (Meteor == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Fing imposible"));
		return;
	}
	//SpawnMesh(location);
	Meteor->Hero = this;
	FireFire();
	SkillCDTimes[SkillNum] = (5.0f - CoolDownTimeA) / CoolDownTimeM;

}
void ATryBetterAgainCharacter::FireQueue(FHitResult Hit)
{
	int32 SkillNum = (int32)Skill::FireQueue - (int32)Skill::Fire_Start;

	UE_LOG(LogTemp, Warning, TEXT("vizov"));
	FacedToEnemy(Hit.ImpactPoint);
	FRotator deltaRotate = (Hit.ImpactPoint - GetActorLocation() + FVector(0, 0, GetActorLocation().Z - Hit.ImpactPoint.Z)).Rotation();
	FVector location = GetActorLocation() + GetActorForwardVector() * 50;
	AFirePrimitive* FireBall = GetWorld()->SpawnActor<AFirePrimitive>(FirePrimitiveBP, location, deltaRotate);
	if (FireBall == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Fing imposible"));
		return;
	}
	FireBall->owner = this;
	FireFire();
	SkillCDTimes[SkillNum] = (5.0f - CoolDownTimeA) / CoolDownTimeM;
}
void ATryBetterAgainCharacter::FireLance(FHitResult Hit)
{

	int32 SkillNum = (int32)Skill::FireLance - (int32)Skill::Fire_Start;

		UE_LOG(LogTemp, Warning, TEXT("vizov"));
		FacedToEnemy(Hit.ImpactPoint);
		FRotator deltaRotate = (Hit.ImpactPoint - GetActorLocation() + FVector(0, 0, GetActorLocation().Z - Hit.ImpactPoint.Z)).Rotation();
		FVector location = GetActorLocation() + GetActorForwardVector() * 50;
		AFireLance* Lance = GetWorld()->SpawnActor<AFireLance>(LanceBP, location, deltaRotate);
		if (Lance == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("Fing imposible"));
			return;
		}
		Lance->owner = this;
		FireFire();
		SkillCDTimes[SkillNum] = (0.5f - CoolDownTimeA )/ CoolDownTimeM;
	
}
void ATryBetterAgainCharacter::FireAura()
{

	AMyFireAura *FireCollisionAura=nullptr;
	int32 SkillNum = (int32)Skill::FireAura - (int32)Skill::Fire_Start;
	
	if (FireEffectAura == nullptr)
	{
		
		FireCollisionAura = GetWorld()->SpawnActor<AMyFireAura>(AMyFireAura::StaticClass(),GetActorLocation(),FRotator::ZeroRotator);
		if (FireCollisionAura == nullptr) return;
		FireFire();
		FireEffectAura = AddNewEffect(true, true, true, NameEffects::FireAuraS);
		FireCollisionAura->Aura->AttachToComponent(GetCapsuleComponent(),FAttachmentTransformRules::KeepWorldTransform);
		FireCollisionAura->Aura->SetSphereRadius(150.0f + MagicRangeA, true);
		FireCollisionAura->Owner = this;
		FireCollisionAura->Duration = 5.0f;
		SkillCDTimes[SkillNum] = (7.0f - CoolDownTimeA) / CoolDownTimeM;

	}
}