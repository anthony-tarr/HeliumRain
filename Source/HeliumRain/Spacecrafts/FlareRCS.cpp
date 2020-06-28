
#include "HeliumRain/Spacecrafts/FlareRCS.h"
#include "../Flare.h"
#include "HeliumRain/Spacecrafts/FlareSpacecraft.h"


/*----------------------------------------------------
	Constructor
----------------------------------------------------*/

UFlareRCS::UFlareRCS(const class FObjectInitializer& PCIP)
	: Super(PCIP)
{
	HasFlickeringLights = false;
}


/*----------------------------------------------------
	Gameplay
----------------------------------------------------*/

bool UFlareRCS::IsDestroyedEffectRelevant()
{
	// Smoke has a 40% chance
	return (FMath::FRand() > 0.4f);
}
