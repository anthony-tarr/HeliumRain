
#include "FlareMeteorite.h"
#include "../Flare.h"
#include "../Game/FlareGame.h"
#include "../Data/FlareMeteoriteCatalog.h"
#include "Components/DestructibleComponent.h"
#include "../Player/FlarePlayerController.h"

#define LOCTEXT_NAMESPACE "FlareMeteorite"


/*----------------------------------------------------
	Constructor
----------------------------------------------------*/

AFlareMeteorite::AFlareMeteorite(const class FObjectInitializer& PCIP) : Super(PCIP)
{
	// Mesh
	Meteorite = PCIP.CreateDefaultSubobject<UDestructibleComponent>(this, TEXT("Meteorite"));
	Meteorite->bTraceComplexOnMove = true;
	Meteorite->SetLinearDamping(0);
	Meteorite->SetAngularDamping(0);
	Meteorite->SetSimulatePhysics(true);
	RootComponent = Meteorite;
	SetActorEnableCollision(true);

	// Physics
	Meteorite->SetMobility(EComponentMobility::Movable);
	Meteorite->SetCollisionProfileName("Destructible");
	Meteorite->GetBodyInstance()->SetUseAsyncScene(false);
	Meteorite->GetBodyInstance()->SetInstanceSimulatePhysics(true);
	Meteorite->SetNotifyRigidBodyCollision(true);

	// Settings
	Meteorite->PrimaryComponentTick.bCanEverTick = true;
	PrimaryActorTick.bCanEverTick = true;
	Paused = false;
}

void AFlareMeteorite::Load(const FFlareMeteoriteSave& Data, UFlareSector* ParentSector)
{
	FLOGV("AFlareMeteorite::Load vel=%s", *Data.LinearVelocity.ToString());

	Parent = ParentSector;

	MeteoriteData = Data;
	SetupMeteoriteMesh();
	Meteorite->SetPhysicsLinearVelocity(Data.LinearVelocity);
	Meteorite->SetPhysicsAngularVelocity(Data.AngularVelocity);

	Target = ParentSector->FindSpacecraft(MeteoriteData.TargetStation);
}

FFlareMeteoriteSave* AFlareMeteorite::Save()
{
	// Physical data
	MeteoriteData.Location = GetActorLocation();
	MeteoriteData.Rotation = GetActorRotation();
	if (!Paused)
	{
		MeteoriteData.LinearVelocity = Meteorite->GetPhysicsLinearVelocity();
		MeteoriteData.AngularVelocity = Meteorite->GetPhysicsAngularVelocity();
	}

	return &MeteoriteData;
}

void AFlareMeteorite::BeginPlay()
{
	Super::BeginPlay();
}

void AFlareMeteorite::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	/*FLOGV("Meteorite %s vel=%s", *GetName(), *Meteorite->GetPhysicsLinearVelocity().ToString());
	FLOGV(" - IsPhysicsCollisionEnabled %d", Meteorite->IsPhysicsCollisionEnabled());
	FLOGV(" - IsPhysicsStateCreated %d", Meteorite->IsPhysicsStateCreated());
	FLOGV(" - IsAnySimulatingPhysics %d", Meteorite->IsAnySimulatingPhysics());
	FLOGV(" - IsAnyRigidBodyAwake %d", Meteorite->IsAnyRigidBodyAwake());
	FLOGV(" - IsCollisionEnabled %d", Meteorite->IsCollisionEnabled());
	FLOGV(" - IsSimulatingPhysics %d", Meteorite->IsSimulatingPhysics());*/



	/*float CollisionSize = Asteroid->GetCollisionShape().GetExtent().Size();
	if (SpawnLocation.Size() <= 0.1)
	{
		SpawnLocation = GetActorLocation();
		DrawDebugSphere(GetWorld(), SpawnLocation, CollisionSize / 2, 16, FColor::Red, true);
	}
	else
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(), CollisionSize / 2, 16, FColor::Blue, false);
		DrawDebugLine(GetWorld(), GetActorLocation(), SpawnLocation, FColor::Green, false);
	}*/

	if(!IsBroken() && Target)
	{
		float Velocity = FMath::Min(10.0f, Meteorite->GetPhysicsLinearVelocity().Size());

		FVector TargetDirection = (Target->GetActorLocation() - GetActorLocation()).GetUnsafeNormal();
		Meteorite->SetPhysicsLinearVelocity(TargetDirection * Velocity);
	}
}


void AFlareMeteorite::NotifyHit(class UPrimitiveComponent* MyComp, class AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::ReceiveHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

	AFlareAsteroid* Asteroid = Cast<AFlareAsteroid>(Other);
	AFlareSpacecraft* Spacecraft = Cast<AFlareSpacecraft>(Other);

	if(Asteroid)
	{
		ApplyDamage(MeteoriteData.BrokenDamage, 1.f , HitLocation, EFlareDamage::DAM_Collision, NULL, FString());
	}
	else if (Spacecraft && (Spacecraft->IsStation() || Spacecraft->GetSize() == EFlarePartSize::L))
	{
		Spacecraft->GetDamageSystem()->ApplyDamage(MeteoriteData.BrokenDamage * 10, MeteoriteData.BrokenDamage, HitLocation, EFlareDamage::DAM_Collision, NULL, FString());

		if(Spacecraft->IsStation())
		{
			Parent->GetGame()->GetQuestManager()->OnEvent(FFlareBundle().PutTag("meteorite-hit-station").PutName("sector", Parent->GetSimulatedSector()->GetIdentifier()));
		}

		ApplyDamage(MeteoriteData.BrokenDamage, 1.f , HitLocation, EFlareDamage::DAM_Collision, NULL, FString());
	}
}

void AFlareMeteorite::SetPause(bool Pause)
{
	FLOGV("AFlareMeteorite::SetPause Pause=%d", Pause);
	
	if (Paused == Pause)
	{
		return;
	}

	CustomTimeDilation = (Pause ? 0.f : 1.0);
	if (Pause)
	{
		Save(); // Save must be performed with the old pause state
	}

	Meteorite->SetSimulatePhysics(!Pause);

	Paused = Pause;
	SetActorHiddenInGame(Pause);

	if (!Pause)
	{
		Meteorite->SetPhysicsLinearVelocity(MeteoriteData.LinearVelocity);
		Meteorite->SetPhysicsAngularVelocity(MeteoriteData.AngularVelocity);
	}
}

void AFlareMeteorite::SetupMeteoriteMesh()
{
	AFlareGame* Game = Cast<AFlareGame>(GetWorld()->GetAuthGameMode());
	
	if (Game->GetMeteoriteCatalog())
	{
		const TArray<UDestructibleMesh*>& MeteoriteList = MeteoriteData.IsMetal ? Game->GetMeteoriteCatalog()->MetalMeteorites : Game->GetMeteoriteCatalog()->RockMeteorites;
		FCHECK(MeteoriteData.MeteoriteMeshID >= 0 && MeteoriteData.MeteoriteMeshID < MeteoriteList.Num());

		Meteorite->SetDestructibleMesh(MeteoriteList[MeteoriteData.MeteoriteMeshID]);
		Meteorite->GetBodyInstance()->SetMassScale(10000);
		Meteorite->GetBodyInstance()->UpdateMassProperties();
	}
	else
	{
		return;
	}
}

bool AFlareMeteorite::IsBroken()
{
	return MeteoriteData.Damage > MeteoriteData.BrokenDamage;
}

void AFlareMeteorite::ApplyDamage(float Energy, float Radius, FVector Location, EFlareDamage::Type DamageType, UFlareSimulatedSpacecraft* DamageSource, FString DamageCauser)
{
	if(!IsBroken())
	{
		MeteoriteData.Damage+= Energy;

		if (IsBroken())
		{
			// Notify PC
			Parent->GetGame()->GetPC()->Notify(LOCTEXT("MeteoriteDestroyed", "Meteorite destroyed"),
									LOCTEXT("MeteoriteDestroyedFormat", "A meteorite has been destroyed"),
									FName("meteorite-destroyed"),
									EFlareNotification::NT_Military,
									false);

			Parent->GetGame()->GetQuestManager()->OnEvent(FFlareBundle().PutTag("meteorite-destroyed").PutName("sector", Parent->GetSimulatedSector()->GetIdentifier()));

			Meteorite->ApplyRadiusDamage(100, Location, 1.f, Energy * 2000 , false);
		}
	}
	else
	{
		Meteorite->ApplyRadiusDamage(Energy / 100, Location, 1.f, Energy * 500 , false);
	}
}

#undef LOCTEXT_NAMESPACE
