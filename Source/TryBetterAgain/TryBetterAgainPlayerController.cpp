// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "TryBetterAgainPlayerController.h"

#include "MyAIController.h"
//#include "AI/Navigation/NavigationSystem.h"
#include "Runtime/Engine/Classes/Components/DecalComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "TryBetterAgainCharacter.h"
#include "MyProjectile.h"
#include "AI.h"
#include "CommonAncestor.h"
#include "GameFramework/Character.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Materials/Material.h"
#include "Runtime/Core/Public/Templates/SharedPointer.h"
#include "Runtime/UMG/Public/Blueprint/UserWidget.h"
#include "Effects.h"

#define MinSpeed 0.01f
#define mini(a,b) ((a)<(b)?(a):(b))
ATryBetterAgainPlayerController::ATryBetterAgainPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
	
	
	/*Dumb = CreateDefaultSubobject<UNavMovementComponent>(TEXT("Dobby"));
	Dumb->UpdateNavAgent(this);
	AIMovement= CreateDefaultSubobject<UPathFollowingComponent>(TEXT("MovementComp"));
	AIMovement->SetMovementComponent(Dumb);*/
}

void ATryBetterAgainPlayerController::BeginPlay()
{
	Super::BeginPlay();
	ZoomFactor = 0.5f;
	CameraSpeed = 4.f;
	CameraUp = 1500.f;
	CameraDown = 600.f;
	bZooming = 0;
	NPK = GetWorld()->SpawnActor<AMyAIController>(AIKClass, FVector(-490.f, -86.f, 392.f), FRotator::ZeroRotator);
	NPK->MyOwner = this;
	OursPawn=Cast<ATryBetterAgainCharacter>(NPK->GetPawn());
	if(OursPawn) 	OursPawn->RealController = this;
	SetViewTarget(OursPawn);
	if (OursPawn == NULL)
	{
		UE_LOG(LogTemp, Warning, TEXT("Korsun is absolute zero  iq"));
	}
	else
		UE_LOG(LogTemp, Warning, TEXT("Korsun is onehugredandforty  iq"));
	SetViewTargetWithBlend(OursPawn,1.0f);
	//OursPawn->InitStats();
	oldVictim = NULL;
	IsMoved = false;
	//skill TEST AREA DELETE THIS
	//OursPawn->Health = 50;
	OursPawn->SkillLevel[(int32)ESkill::FireBlink-(int32)ESkill::Fire_Start] = 1;
	OursPawn->SkillLevel[(int32)ESkill::FireBurn - (int32)ESkill::Fire_Start] = 7;
	OursPawn->SkillLevel[(int32)ESkill::FireFire - (int32)ESkill::Fire_Start] = 100;
	OursPawn->SkillLevel[(int32)ESkill::FireAfterBurn - (int32)ESkill::Fire_Start] = 5;
	OursPawn->SkillLevel[(int32)ESkill::FireMeteor - (int32)ESkill::Fire_Start] = 2;
	OursPawn->SkillLevel[(int32)ESkill::FireLance - (int32)ESkill::Fire_Start] = 3;
	OursPawn->SkillLevel[(int32)ESkill::FireAura - (int32)ESkill::Fire_Start] = 3;
	OursPawn->SkillLevel[(int32)ESkill::FireQueue - (int32)ESkill::Fire_Start] = 3;
	//UE_LOG(LogTemp, Warning, TEXT("Init SkillLevel"));
	
	State = ESkill::None;
	WaitTime = 0;
}


void ATryBetterAgainPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	
	if (OursPawn != nullptr)
	{
		if (State != ESkill::None)
		{
			if (State == ESkill::FireQueue)
			{
				GetHitResultUnderCursor(ECC_Visibility, false, Direct);
				OursPawn->FacedToEnemy(Direct.ImpactPoint);
			}
			FullTime += DeltaTime;
			WaitTime -= DeltaTime;
			if (WaitTime <= 0.0f)
			{
				WaitTime = DoSkill(State, FullTime);
				if(WaitTime<=0.0f)	State = ESkill::None;
			}
		}
		else
		{
			// get command from mouse
			if (bClicked) {
				FHitResult Hit;
				GetHitResultUnderCursor(ECC_Visibility, false, Hit);

				if (Hit.bBlockingHit) {
					victim = Cast<AAI>(Hit.GetActor());
					if (victim != nullptr) {
						if (victim != oldVictim)
						{
							DontAttack();
							oldVictim = victim;
						}
						is_gonna_attacking = true;
						//UE_LOG(LogTemp, Warning, TEXT("Set victim"));
					}
					else {
						is_gonna_attacking = false;
						DontAttack();
						//UE_LOG(LogTemp, Warning, TEXT("Move destination to HZ"));
						SetNewMoveDestination(Hit.ImpactPoint);
					}
				}
			}
			//check have victim died yet(3)
			if (is_gonna_attacking && !IsValid(victim))
			{
				is_gonna_attacking = false;
				bAttack = false;
			}
			//get distance from victim
			if (is_gonna_attacking)	Distance = FVector::Dist(victim->GetActorLocation(), OursPawn->GetActorLocation())- victim->GetCapsuleComponent()->GetScaledCapsuleRadius();
			//set path to victim
			if (is_gonna_attacking && !bAttack) 
			{
				UE_LOG(LogTemp, Warning, TEXT("Korsun is %f  iq i %f tuposti"), Distance - OursPawn->AttackRange,Distance);
				if (Distance > OursPawn->AttackRange)
				{
					//victim->SpawnMesh(destination);
					if (!IsMoved||OursPawn->GetVelocity().Size2D()<MinSpeed)
					{
						UE_LOG(LogTemp, Warning, TEXT("Korsun wants move"));
						NPK->MoveToActor(victim, OursPawn->AttackRange,false,true);
						IsMoved = true;
					}
				}
				else 
				{
					Attack();
					IsMoved = false;
				}
			}

			if (AttackAnimTime < 0)
			{
				AttackAnimTime += DeltaTime / OursPawn->AttackTime;
			}
			else
			{
				if (bAttack&&Distance < 2 * OursPawn->AttackRange)
				{
					if (OursPawn->FacedToEnemy(victim->GetActorLocation())) {
						OursPawn->OnePunch = true;
						AttackAnimTime += DeltaTime / (OursPawn->AttackTime);
						if (PrevAttackTick != 2)
						{
							PrevAttackTick = AtakAnim(AttackAnimTime);
							if (PrevAttackTick == 2 || PrevAttackTick == 3)
							{
								OursPawn->DoAttack(victim);
							}
						}
						else
						{
							PrevAttackTick = AtakAnim(AttackAnimTime);
						}
						if (PrevAttackTick == 3)
						{
							PrevAttackTick = 1;
							AttackAnimTime -= 1.0f;
						}
					}
				}
				else
				{

					bAttack = false;
					OursPawn->OnePunch = false;
					
					if (PrevAttackTick == 2)
					{
						AttackAnimTime -= 1.0f;
					}
					else
					{
						AttackAnimTime = 0.0f;
					}
					PrevAttackTick = 0;
				}
			}
		}
		//Zoom

		if (bZooming == 1)
		{
			ZoomFactor += CameraSpeed * DeltaTime;         //Zoom in over half a second
		}
		else if (bZooming == -1)
		{
			ZoomFactor -= CameraSpeed * DeltaTime;        //Zoom out over a quarter of a second
		}
		ZoomFactor = FMath::Clamp<float>(ZoomFactor, 0.0f, 1.0f);
		//Blend our camera's FOV and our SpringArm's length based on ZoomFactor
		if (OursPawn != NULL)
		{
			OursPawn->TopDownCameraComponent->FieldOfView = FMath::Lerp<float>(90.0f, 60.0f, ZoomFactor);
			OursPawn->CameraBoom->TargetArmLength = FMath::Lerp<float>(CameraUp, CameraDown, ZoomFactor);
			bZooming = 0;
		}

	}
}

void ATryBetterAgainPlayerController::CastSpell()
{
	FHitResult Hit;
	GetHitResultUnderCursor(ECC_Visibility, false, Hit);
	if (Hit.bBlockingHit) {
		
		OursPawn->FacedToEnemy(Hit.ImpactPoint);
		UWorld* const World = GetWorld();
		FRotator deltaRotate = (Hit.ImpactPoint - OursPawn->GetActorLocation() + FVector(0, 0, OursPawn->GetActorLocation().Z - Hit.ImpactPoint.Z)).Rotation();
		FVector location = OursPawn->GetActorLocation() + OursPawn->GetActorForwardVector() * 50;
		AMyProjectile* Projectile = World->SpawnActor<AMyProjectile>(MyProjectileBP, location, deltaRotate);
		Projectile->owner = OursPawn;
		Projectile->Damage = OursPawn->AttackDamage;
	}
}

void ATryBetterAgainPlayerController::Attack()
{
	bAttack = true;
}

void ATryBetterAgainPlayerController::DontAttack()
{
	bAttack = false; OursPawn->OnePunch = false;

	if (PrevAttackTick == 2)
	{
		AttackAnimTime -= 1.0f;
	}
	else
	{
		AttackAnimTime = 0.0f;
	}
	PrevAttackTick = 0;
}

void ATryBetterAgainPlayerController::SetupInputComponent()
{
	// set up gameplay key bindings
	Super::SetupInputComponent();
	struct FInputActionBinding* Consume;

	Consume = &InputComponent->BindAction("SetDestination", IE_Pressed, this, &ATryBetterAgainPlayerController::OnSetDestinationPressed);
	Consume->bConsumeInput = false;
	InputComponent->BindAction("SetDestination", IE_Released, this, &ATryBetterAgainPlayerController::OnSetDestinationReleased).bConsumeInput=false;

	Consume = &InputComponent->BindAction("CastSpell", IE_Pressed, this, &ATryBetterAgainPlayerController::OnSpellCastPressed);
	Consume->bConsumeInput = false;
	InputComponent->BindAction("CastSpell", IE_Released, this, &ATryBetterAgainPlayerController::OnSpellCastReleased).bConsumeInput = false;
	InputComponent->BindAction("ZoomIn", IE_Pressed, this, &ATryBetterAgainPlayerController::ZoomIn).bConsumeInput=false;
	InputComponent->BindAction("ZoomOut", IE_Pressed, this, &ATryBetterAgainPlayerController::ZoomOut).bConsumeInput=false;
	InputComponent->BindAction("PauseMenu", IE_Pressed, this, &ATryBetterAgainPlayerController::SetPauseMenu).bConsumeInput = false;
	InputComponent->BindAction("FirstSkill", IE_Pressed, this, &ATryBetterAgainPlayerController::FireBlink).bConsumeInput = false;
	InputComponent->BindAction("Skill2", IE_Pressed, this, &ATryBetterAgainPlayerController::FireLance).bConsumeInput = false;
	InputComponent->BindAction("Skill3", IE_Pressed, this, &ATryBetterAgainPlayerController::FireMeteor).bConsumeInput = false;
	InputComponent->BindAction("Skill4", IE_Pressed, this, &ATryBetterAgainPlayerController::FireAura).bConsumeInput = false;
	InputComponent->BindAction("CancelSkill", IE_Pressed, this, &ATryBetterAgainPlayerController::CancelSkill).bConsumeInput = false;
	InputComponent->BindAction("Skill5", IE_Pressed, this, &ATryBetterAgainPlayerController::FireQueue).bConsumeInput = false;
}




void ATryBetterAgainPlayerController::SetNewMoveDestination(const FVector DestLocation)
{
	
	
	if (OursPawn)
	{
		UNavigationSystemBase* const NavSys = GetWorld()->GetNavigationSystem();
		float const Distance = FVector::Dist(DestLocation, OursPawn->GetActorLocation());
		
		// We need to issue move command only if far enough in order for walk animation to play correctly
		if (NavSys )
		{
			//UE_LOG(LogTemp, Warning, TEXT("distance is %f"), Distance);
			//NavSys->SimpleMoveToLocation(this, DestLocation);
			//UE_LOG(LogTemp, Warning, TEXT("Korsun is onehungredandforty  iq"));
			NPK->MoveToLocation(DestLocation, 1.0f);
		}
	}
	else
	{
		//UE_LOG(LogTemp, Warning, TEXT("Korsun is absolute zero  iq"));
	}
}

void ATryBetterAgainPlayerController::OnSetDestinationPressed()
{
	// set flag to keep updating destination until released
	bClicked = true;
}

void ATryBetterAgainPlayerController::OnSetDestinationReleased()
{
	// clear flag to indicate we should stop updating the destination
	bClicked = false;
}

void ATryBetterAgainPlayerController::OnSpellCastPressed()
{
	// set flag to keep updating destination until released
	leftClicked = true;
}

void ATryBetterAgainPlayerController::OnSpellCastReleased()
{
	// set flag to keep updating destination until released
	CastSpell();
	leftClicked = false;
}

void ATryBetterAgainPlayerController::ZoomIn()
{
	bZooming = 1;
}

void ATryBetterAgainPlayerController::ZoomOut()
{
	bZooming = -1;
}
void ATryBetterAgainPlayerController::SetPauseMenu()
{
	if (IsPaused() == false)
	{
		if (PauseMenu == NULL)
		{
			PauseMenu = CreateWidget<UUserWidget>(GetWorld(), WidgetClass);
		}
		if (PauseMenu)
		{
			if(PauseMenu->IsInViewport()==false)
			PauseMenu->AddToViewport();
		}
	}

}
int ATryBetterAgainPlayerController::AtakAnim(float AtakAnim)
{
	if (AtakAnim < OursPawn->PreAttack) return 1;
	if (AtakAnim < 1.0f) return 2;
	return 3;
}
void ATryBetterAgainPlayerController::FireMeteor()
{
	if (OursPawn != NULL&&State != ESkill::FireMeteor)
	{
		int32 SkillNum = (int32)ESkill::FireMeteor - (int32)ESkill::Fire_Start;

		if (OursPawn->SkillCDTimes[SkillNum] == 0.0f&&OursPawn->SkillLevel[SkillNum] != 0)
		{
			FHitResult Hit;
			GetHitResultUnderCursor(ECC_Visibility, false, Hit);

			float Range = (700 + OursPawn->RealA["MagicRange"])*OursPawn->RealM["MagicRange"];
			if (FVector::Dist2D(Hit.ImpactPoint, OursPawn->GetActorLocation()) < Range)
			{
				OursPawn->FacedToEnemy(Hit.ImpactPoint);
				State = ESkill::FireMeteor;
				DoStop();
				WaitTime = (3.0f - OursPawn->RealA["CastTime"])*OursPawn->RealM["CastTime"];
				FullTime = 0.0f;
				Direct = Hit;
			}
		}
	}
}
void ATryBetterAgainPlayerController::FireQueue()
{
	if (OursPawn != NULL&&State != ESkill::FireQueue)
	{
		int32 SkillNum = (int32)ESkill::FireQueue - (int32)ESkill::Fire_Start;

		if (OursPawn->SkillCDTimes[SkillNum] == 0.0f&&OursPawn->SkillLevel[SkillNum] != 0)
		{
			FHitResult Hit;
			GetHitResultUnderCursor(ECC_Visibility, false, Hit);

			float Range = (700 + OursPawn->RealA["MagicRange"])*OursPawn->RealM["MagicRange"];
			if (FVector::Dist2D(Hit.ImpactPoint, OursPawn->GetActorLocation()) < Range)
			{
				OursPawn->FacedToEnemy(Hit.ImpactPoint);
				State = ESkill::FireQueue;
				DoStop();
				WaitTime = (2.0f - OursPawn->RealA["CastTime"])*OursPawn->RealM["CastTime"];
				FullTime = 0.0f;
				Direct = Hit;
			}
		}
	}
}
void ATryBetterAgainPlayerController::FireBlink()
{
	if (OursPawn != NULL) {
		FHitResult Hit;
		GetHitResultUnderCursor(ECC_Visibility, false, Hit);
		OursPawn->FireBlink(Hit);
	}
}
void ATryBetterAgainPlayerController::FireLance()
{
	if (OursPawn != NULL&&State!=ESkill::FireLance)
	{
		int32 SkillNum = (int32)ESkill::FireLance - (int32)ESkill::Fire_Start;

		if (OursPawn->SkillCDTimes[SkillNum] == 0.0f&&OursPawn->SkillLevel[SkillNum] != 0)
		{
			FHitResult Hit;
			GetHitResultUnderCursor(ECC_Visibility, false, Hit);

			float Range = (700 + OursPawn->RealA["MagicRange"])*OursPawn->RealM["MagicRange"];
			if (FVector::Dist2D(Hit.ImpactPoint, OursPawn->GetActorLocation()) < Range)
			{
				State = ESkill::FireLance;
				DoStop();
				WaitTime = (1.0f-OursPawn->RealA["CastTime"])*OursPawn->RealM["CastTime"];
				FullTime = 0.0f; 
				Direct = Hit;
			}
		}
	}
}

void ATryBetterAgainPlayerController::FireAura()
{
	if (OursPawn != NULL)
	{
		int32 SkillNum = (int32)ESkill::FireAura - (int32)ESkill::Fire_Start;

		if (OursPawn->SkillCDTimes[SkillNum] == 0.0f&&OursPawn->SkillLevel[SkillNum] != 0)
		{
			OursPawn->FireAura();
			//UE_LOG(LogTemp, Warning, TEXT("StartAura"));
		}
	}
}
float ATryBetterAgainPlayerController::DoSkill(ESkill State,float Time)
{

	switch (State)
	{	
	case ESkill::FireMeteor:
		OursPawn->FireMeteor(Direct);
		break;
	case ESkill::FireQueue:
		OursPawn->FireQueue(Direct);
		return mini((2.0f - OursPawn->RealA["CastTime"])*OursPawn->RealM["CastTime"], 5.0f - FullTime);
	case ESkill::FireLance:
		OursPawn->FireLance(Direct);
		break;
	}
	return 0;
}
void ATryBetterAgainPlayerController::DoStop()
{
	
		is_gonna_attacking = false;
		DontAttack();
		NPK->StopMovement();
}
void ATryBetterAgainPlayerController::CancelSkill()
{
	State = ESkill::None;

}
