// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "TryBetterAgainPlayerController.h"
#include "AI/Navigation/NavigationSystem.h"
#include "Runtime/Engine/Classes/Components/DecalComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "TryBetterAgainCharacter.h"
#include "MyProjectile.h"
#include "AI.h"

ATryBetterAgainPlayerController::ATryBetterAgainPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
}

FVector ATryBetterAgainPlayerController::Tehnika100TochekKorsuna(FVector to, int range, FVector from)
{
	UNavigationSystem* const NavSys = GetWorld()->GetNavigationSystem();
	float path_len;
	FVector nearest_point = to;

	if (NavSys) {
		nearest_point = to + FVector(range, 0, 0);
		NavSys->GetPathLength(from, nearest_point, path_len);
		//UE_LOG(LogTemp, Warning, TEXT("path_len = %f"), path_len);
		for (int i = 1; i < 100; i++) {
			float radian = i * PI / 50;
			FVector current_point = to + FVector(range * cos(radian), range * sin(radian), 0);
			float current_path_len;
			NavSys->GetPathLength(from, current_point, current_path_len);
			//UE_LOG(LogTemp, Warning, TEXT("current_path_len = %f"), current_path_len);
			if (current_path_len < path_len) {
				nearest_point = current_point;
				path_len = current_path_len;
			}
		}
	} 
	//UE_LOG(LogTemp, Warning, TEXT("returning point = %f"), path_len);
	return nearest_point;
}

void RotateToFacedEnemy()
{
	

}

void ATryBetterAgainPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	static bool is_gonna_attacking = false;
	static AAI* victim;
	static float all_time = 0;
	static ATryBetterAgainCharacter* MyCharacter;

	// keep updating the destination every tick while desired
	if (bClicked) {
		FHitResult Hit;
		GetHitResultUnderCursor(ECC_Visibility, false, Hit);

		if (Hit.bBlockingHit) {
			victim = Cast<AAI>(Hit.GetActor());
			if (victim != nullptr) {
				is_gonna_attacking = true;
				UE_LOG(LogTemp, Warning, TEXT("Set victim"));
			}
			else {
				is_gonna_attacking = false;
				DontAttack();
				UE_LOG(LogTemp, Warning, TEXT("Move destination to HZ"));
				SetNewMoveDestination(Hit.ImpactPoint);
			}
		}
	}

	if (is_gonna_attacking && !bAttack) {
		//UE_LOG(LogTemp, Warning, TEXT("In is_gonna_attacking"));
		APawn* const MyPawn = GetPawn();
		MyCharacter = Cast<ATryBetterAgainCharacter>(MyPawn);
		if (MyCharacter != nullptr) {
			float const Distance = FVector::Dist(victim->GetActorLocation(), MyCharacter->GetActorLocation());
			//UE_LOG(LogTemp, Warning, TEXT("Distance is %f"), Distance);
			if (Distance >= MyCharacter->AttackRange) {
				FVector destination = Tehnika100TochekKorsuna(victim->GetActorLocation(), MyCharacter->AttackRange - 50.0f, MyCharacter->GetActorLocation());
				UE_LOG(LogTemp, Warning, TEXT("Move destination to AI"));
				SetNewMoveDestination(destination);
			}
			else {
				if (MyCharacter->FacedToEnemy(victim->GetActorLocation())){
					Attack();
					is_gonna_attacking = false;
				}
				//UE_LOG(LogTemp, Warning, TEXT("Unset is_gonna_attacking"));
			}
		}
		else
			is_gonna_attacking = false;
	}
	
	if (bAttack) {
		all_time += DeltaTime;
		if (all_time >= 1) {
			TArray<int32> flags;
			MyCharacter->CalculateAttack(victim, flags);
			all_time = 0;
		}
	}
}

void ATryBetterAgainPlayerController::CastSpell()
{
	FHitResult Hit;
	GetHitResultUnderCursor(ECC_Visibility, false, Hit);
	if (Hit.bBlockingHit) {
		APawn* const MyPawn = GetPawn();
		ATryBetterAgainCharacter* MyCharacter = Cast<ATryBetterAgainCharacter>(MyPawn);
		MyCharacter->FacedToEnemy(Hit.ImpactPoint);
		UWorld* const World = GetWorld();
		FRotator deltaRotate = (Hit.ImpactPoint - MyPawn->GetActorLocation()).Rotation();
		FVector location = MyCharacter->GetActorLocation() + MyCharacter->GetActorForwardVector() * 50;
		AMyProjectile* Projectile = World->SpawnActor<AMyProjectile>(MyProjectileBP, location, deltaRotate);
		Projectile->owner = MyCharacter;
	}
}

void ATryBetterAgainPlayerController::Attack()
{
	bAttack = true;
}

void ATryBetterAgainPlayerController::DontAttack()
{
	bAttack = false;
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
}

void ATryBetterAgainPlayerController::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}


void ATryBetterAgainPlayerController::MoveToTouchLocation(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	FVector2D ScreenSpaceLocation(Location);

	// Trace to see what is under the touch location
	FHitResult HitResult;
	GetHitResultAtScreenPosition(ScreenSpaceLocation, CurrentClickTraceChannel, true, HitResult);
	if (HitResult.bBlockingHit)
	{
		// We hit something, move there
		SetNewMoveDestination(HitResult.ImpactPoint);
	}
}

void ATryBetterAgainPlayerController::SetNewMoveDestination(const FVector DestLocation)
{
	APawn* const MyPawn = GetPawn();
	
	if (MyPawn)
	{
		UNavigationSystem* const NavSys = GetWorld()->GetNavigationSystem();
		float const Distance = FVector::Dist(DestLocation, MyPawn->GetActorLocation());

		// We need to issue move command only if far enough in order for walk animation to play correctly
		if (NavSys && (Distance > 10.0f))
		{
			UE_LOG(LogTemp, Warning, TEXT("distance is %f"), Distance);
			NavSys->SimpleMoveToLocation(this, DestLocation);
		}
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