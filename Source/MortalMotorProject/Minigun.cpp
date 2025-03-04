// Fill out your copyright notice in the Description page of Project Settings.


#include "Minigun.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Particles/ParticleSystemComponent.h"
#include "IDamageable.h"

// Sets default values
AMinigun::AMinigun():
	m_searchTimer(0.f),
	m_attackTimer(0.f)
{
	PrimaryActorTick.bCanEverTick = true;

	TurretBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MiniGunBody"));
	TurretBody->SetupAttachment(RootComponent);

	//SetUp traceparams
	TraceParams = FCollisionQueryParams();
	TraceParams.bTraceComplex = false;
	TraceParams.bReturnPhysicalMaterial = false;
	TraceParams.AddIgnoredActor(this);
	SweepSphere = FCollisionShape::MakeSphere(Radius);
	m_offset = FVector(0, 0, 0.1f);

	//VFX
	ShootVfxComponent = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ShootVFX"));
	ShootVfxComponent->bAutoActivate = false;
	ShootVfxComponent->SetupAttachment(TurretBody);
}

void AMinigun::BeginPlay()
{
	Super::BeginPlay();
}

void AMinigun::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//IF no target
	if (Target == nullptr)
	{
		//Scane for a target via sphere sweep
		ScanForTarget();
	}
	
	//IF has target
	if (Target != nullptr)
	{
		//Check every 2 seconds if the target is far from the shooting range or dead
		if (m_searchTimer >= SEARCH_TIME)
		{
			m_searchTimer = 0.f;
			ValidateTarget();
		}

		//Rotate towards target
		RotateTowardsTarget(DeltaTime);

		if (m_attackTimer >= FireRate && Target)
		{
			//Shoot Target
			Shoot();
			m_attackTimer = 0.f;
		}
		
		m_attackTimer += DeltaTime;
		m_searchTimer += DeltaTime;
	}

}

void AMinigun::ScanForTarget()
{

	bool hit = GetWorld()->SweepSingleByChannel
	(
		Outhit,
		TurretBody->GetComponentLocation(),
		TurretBody->GetComponentLocation() + m_offset,
		FQuat::Identity,
		ECollisionChannel::ECC_GameTraceChannel1,
		SweepSphere,
		TraceParams
	);
	

	if (hit)
	{
		Target = Outhit.GetActor();
	}
}

void AMinigun::ValidateTarget()
{
	//if the target becomes null or dead
	if (Target == nullptr) { return; }

	//calculate distance between gun and target
	float Distance = this->GetDistanceTo(Target);

	//if the target becomes too far from the gun, we clear it's ref
	if (Distance > Radius)
	{
		Target = nullptr;
		StopShoot();
	}
	
}

void AMinigun::RotateTowardsTarget(float deltaTime)
{
	if (Target == nullptr) { return; }

	// Get the direction to the target
	FVector Direction = (Target->GetActorLocation() - TurretBody->GetComponentLocation()).GetSafeNormal();

	// Get the rotation to face the target
	FRotator TargetRotation = FRotationMatrix::MakeFromX(Direction).Rotator();
	TargetRotation.Pitch = 0.f;
	TargetRotation.Roll = 0.f;


	//Lerp to new rotation instead of snapping to it
	FRotator NewRotation = FMath::Lerp(TurretBody->GetComponentRotation(), TargetRotation, TurretRotationSpeed * deltaTime);

	// Set the actor's rotation
	TurretBody->SetWorldRotation(NewRotation);
}

void AMinigun::Shoot()
{
	if (Target == nullptr) { return; }

	IIDamageable* Damageable = Cast<IIDamageable>(Target);
	ShootVfxComponent->Activate();

	if (Damageable && Damageable->IsAlive())
	{
		Damageable->TakeDamge(Damage);
	}

	if (!Damageable->IsAlive())
	{
		Target = nullptr;
		StopShoot();
	}
}

void AMinigun::StopShoot()
{
	ShootVfxComponent->Deactivate();
}

