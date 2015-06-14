
#include "../../Flare.h"
#include "FlareHUDMenu.h"
#include "../../Player/FlarePlayerController.h"


#define LOCTEXT_NAMESPACE "FlareHUDMenu"


/*----------------------------------------------------
	Construct
----------------------------------------------------*/

void SFlareHUDMenu::Construct(const FArguments& InArgs)
{
	// Data
	OwnerHUD = InArgs._OwnerHUD;
	TargetShip = NULL;
	Overheating = false;
	Burning = false;
	PowerOutage = false;
	PresentationFlashTime = 0.2f;
	TimeSinceOverheatChanged = PresentationFlashTime;
	TimeSinceOutageChanged = PresentationFlashTime;
	AFlarePlayerController* PC = Cast<AFlarePlayerController>(OwnerHUD->GetOwner());

	// Style
	const FFlareStyleCatalog& Theme = FFlareStyleSet::GetDefaultTheme();
	FLinearColor NormalColor = Theme.NeutralColor;
	NormalColor.A = Theme.DefaultAlpha;

	// Structure
	ChildSlot
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill)
	.Padding(FMargin(0))
	[
		SNew(SVerticalBox)

		// Text notification box
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		[
			SNew(SVerticalBox)

			// Info text
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Top)
			.Padding(Theme.SmallContentPadding)
			[
				SAssignNew(InfoText, STextBlock)
				.TextStyle(&Theme.SmallFont)
				.Text(this, &SFlareHUDMenu::GetInfoText)
				.ColorAndOpacity(NormalColor)
			]

			// Overheating progress bar
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Top)
			[
				SNew(SHorizontalBox)
				
				// Icon
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(Theme.ContentPadding)
				[
					SNew(SImage)
					.Image(FFlareStyleSet::GetIcon("Temperature"))
					.ColorAndOpacity(this, &SFlareHUDMenu::GetTemperatureColorNoAlpha)
				]

				// Bar
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox)
					.MinDesiredWidth(500)
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Center)
					[
						SNew(SProgressBar)
						.Style(&Theme.TemperatureBarStyle)
						.Percent(this, &SFlareHUDMenu::GetTemperatureProgress)
						.FillColorAndOpacity(this, &SFlareHUDMenu::GetTemperatureColorNoAlpha)
					]
				]

				// Text
				+ SHorizontalBox::Slot()
				.Padding(Theme.ContentPadding)
				[
					SNew(SBox)
					.MinDesiredWidth(100)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.TextStyle(&Theme.NameFont)
						.Text(this, &SFlareHUDMenu::GetTemperatureText)
						.ColorAndOpacity(this, &SFlareHUDMenu::GetTemperatureColor)
					]
				]
			]
	
			// Overheating box
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.TextStyle(&Theme.SubTitleFont)
				.Text(LOCTEXT("Overheating", "OVERHEATING !"))
				.ColorAndOpacity(this, &SFlareHUDMenu::GetOverheatColor, true)
			]

			// Outage box
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.TextStyle(&Theme.SubTitleFont)
					.Text(LOCTEXT("PowerOutage", "POWER OUTAGE !"))
					.ColorAndOpacity(this, &SFlareHUDMenu::GetOutageColor, true)
				]
			
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(this, &SFlareHUDMenu::GetOutageText)
					.TextStyle(&Theme.SubTitleFont)
					.ColorAndOpacity(this, &SFlareHUDMenu::GetOutageColor, true)
				]
			]
		]

		// Weapon panel
		+ SVerticalBox::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0, 0, 0, 100))
		[
			SAssignNew(WeaponContainer, SVerticalBox)
		]

		// Status panel
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		[
			SNew(SHorizontalBox)
			
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SAssignNew(TemperatureStatus, SFlareSubsystemStatus).Subsystem(EFlareSubsystem::SYS_Temperature)
			]

			+ SHorizontalBox::Slot().AutoWidth()
			[
				SAssignNew(PowerStatus, SFlareSubsystemStatus).Subsystem(EFlareSubsystem::SYS_Power)
			]

			+ SHorizontalBox::Slot().AutoWidth()
			[
				SAssignNew(PropulsionStatus, SFlareSubsystemStatus).Subsystem(EFlareSubsystem::SYS_Propulsion)
			]

			+ SHorizontalBox::Slot().AutoWidth()
			[
				SAssignNew(RCSStatus, SFlareSubsystemStatus).Subsystem(EFlareSubsystem::SYS_RCS)
			]

			+ SHorizontalBox::Slot().AutoWidth()
			[
				SAssignNew(LifeSupportStatus, SFlareSubsystemStatus).Subsystem(EFlareSubsystem::SYS_LifeSupport)
			]

			+ SHorizontalBox::Slot().AutoWidth()
			[
				SAssignNew(WeaponStatus, SFlareSubsystemStatus).Subsystem(EFlareSubsystem::SYS_Weapon)
			]
		]
	];
}


/*----------------------------------------------------
	Interaction
----------------------------------------------------*/

void SFlareHUDMenu::SetTargetShip(IFlareSpacecraftInterface* Target)
{
	// Set targets
	TargetShip = Target;
	TemperatureStatus->SetTargetShip(Target);
	PowerStatus->SetTargetShip(Target);
	PropulsionStatus->SetTargetShip(Target);
	RCSStatus->SetTargetShip(Target);
	LifeSupportStatus->SetTargetShip(Target);
	WeaponStatus->SetTargetShip(Target);
	AFlareSpacecraft* PlayerShip = Cast<AFlareSpacecraft>(Target);
	const FFlareStyleCatalog& Theme = FFlareStyleSet::GetDefaultTheme();

	// Is this a civilian ship ?
	WeaponStatus->SetVisibility(Target->IsMilitary() ? EVisibility::Visible : EVisibility::Hidden);

	// Update weapon list
	if (PlayerShip)
	{
		TArray<FFlareWeaponGroup*>& WeaponGroupList = PlayerShip->GetWeaponsSystem()->GetWeaponGroupList();
		TSharedPtr<SFlareSubsystemStatus> Temp;
		WeaponContainer->ClearChildren();

		// Add weapon indicators
		for (int32 i = WeaponGroupList.Num() - 1; i >= 0; i--)
		{
			WeaponContainer->AddSlot()
			.AutoHeight()
			[
				SNew(SFlareWeaponStatus)
				.PlayerShip(PlayerShip)
				.TargetWeaponGroupIndex(i)
			];
		}

		// No weapon
		WeaponContainer->AddSlot()
		.AutoHeight()
		[
			SNew(SFlareWeaponStatus)
			.PlayerShip(PlayerShip)
			.TargetWeaponGroupIndex(-1)
		];

		WeaponContainer->SetVisibility(EVisibility::Visible);
	}
}


/*----------------------------------------------------
	Callbacks
----------------------------------------------------*/

void SFlareHUDMenu::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (TargetShip)
	{
		// Is alive ?
		SetVisibility(TargetShip->GetDamageSystem()->IsAlive() ? EVisibility::Visible : EVisibility::Collapsed);

		// Overheating status
		TimeSinceOverheatChanged += InDeltaTime;
		Temperature = TargetShip->GetDamageSystem()->GetTemperature();
		OverheatTemperature = TargetShip->GetDamageSystem()->GetOverheatTemperature();

		// Alert the player if the ship is near the overheat temperature
		bool NewOverheating = (TargetShip->GetDamageSystem()->GetTemperature() > TargetShip->GetDamageSystem()->GetOverheatTemperature() * 0.95);
		if (NewOverheating != Overheating)
		{
			TimeSinceOverheatChanged = 0;
		}
		Overheating = NewOverheating;
		Burning = (TargetShip->GetDamageSystem()->GetTemperature() > TargetShip->GetDamageSystem()->GetBurnTemperature());

		// Outage status
		TimeSinceOutageChanged += InDeltaTime;
		bool NewPowerOutage = TargetShip->GetDamageSystem()->HasPowerOutage();
		if (NewPowerOutage != PowerOutage)
		{
			TimeSinceOutageChanged = 0;
		}
		PowerOutage = NewPowerOutage;
	}
}

FText SFlareHUDMenu::GetInfoText() const
{
	FText ShipText;
	FText InfoText = LOCTEXT("Controls", "mode");
	FText ModeText;
	FText SectorText = LOCTEXT("TODOTEXT", "Nema A19");

	if (TargetShip)
	{
		ShipText = Cast<AFlareSpacecraft>(TargetShip)->GetDescription()->Name;
		EFlareWeaponGroupType::Type WeaponType = TargetShip->GetWeaponsSystem()->GetActiveWeaponType();

		switch (WeaponType)
		{
			case EFlareWeaponGroupType::WG_NONE:    ModeText = LOCTEXT("Navigation", "Navigation");      break;
			case EFlareWeaponGroupType::WG_GUN:     ModeText = LOCTEXT("Fighter", "Fighter");            break;
			case EFlareWeaponGroupType::WG_BOMB:    ModeText = LOCTEXT("Bomber", "Bomber");              break;
			case EFlareWeaponGroupType::WG_TURRET:
			default:                                ModeText = LOCTEXT("CapitalShip", "Capital ship");   break;
		}
	}

	return FText::FromString(ShipText.ToString() + " - " + ModeText.ToString() + " " + InfoText.ToString() + " - " + SectorText.ToString());
}

TOptional<float> SFlareHUDMenu::GetTemperatureProgress() const
{
	float WidgetMin = 500.0f;
	float WidgetRange = OverheatTemperature - WidgetMin;
	return ((Temperature - WidgetMin) / WidgetRange);
}

FSlateColor SFlareHUDMenu::GetTemperatureColor() const
{
	const FFlareStyleCatalog& Theme = FFlareStyleSet::GetDefaultTheme();
	float Distance = Temperature - 0.9f * OverheatTemperature;
	float Ratio = FMath::Clamp(FMath::Abs(Distance) / 10.0f, 0.0f, 1.0f);

	if (Distance < 0)
	{
		Ratio = 0.0f;
	}

	FLinearColor Color = FMath::Lerp(Theme.NeutralColor, Theme.EnemyColor, Ratio);
	Color.A = Theme.DefaultAlpha;
	return Color;
}

FSlateColor SFlareHUDMenu::GetTemperatureColorNoAlpha() const
{
	const FFlareStyleCatalog& Theme = FFlareStyleSet::GetDefaultTheme();
	float Distance = Temperature - 0.9f * OverheatTemperature;
	float Ratio = FMath::Clamp(FMath::Abs(Distance) / 10.0f, 0.0f, 1.0f);

	if (Distance < 0)
	{
		Ratio = 0.0f;
	}

	return FMath::Lerp(Theme.NeutralColor, Theme.EnemyColor, Ratio);
}

FText SFlareHUDMenu::GetTemperatureText() const
{
	return FText::FromString(FString::Printf(TEXT("%4s K"), *FString::FromInt(Temperature)));
}

FSlateColor SFlareHUDMenu::GetOverheatColor(bool Text) const
{
	const FFlareStyleCatalog& Theme = FFlareStyleSet::GetDefaultTheme();
	
	FLinearColor Color = Theme.EnemyColor;
	float Ratio = FMath::Clamp(TimeSinceOverheatChanged / PresentationFlashTime, 0.0f, 1.0f);
	Color.A *= (Overheating ? Ratio : (1 - Ratio));

	if (Text)
	{
		Color.A *= Theme.DefaultAlpha;
	}

	return Color;
}

FSlateColor SFlareHUDMenu::GetOutageColor(bool Text) const
{
	const FFlareStyleCatalog& Theme = FFlareStyleSet::GetDefaultTheme();

	FLinearColor Color = Theme.NeutralColor;
	float Ratio = FMath::Clamp(TimeSinceOutageChanged / PresentationFlashTime, 0.0f, 1.0f);
	Color.A *= (PowerOutage ? Ratio : (1 - Ratio));

	if (Text)
	{
		Color.A *= Theme.DefaultAlpha;
	}

	return Color;
}

FText SFlareHUDMenu::GetOutageText() const
{
	if (TargetShip)
	{
		return FText::FromString(LOCTEXT("PwBackIn", "Power back in ").ToString() + FString::FromInt(TargetShip->GetDamageSystem()->GetPowerOutageDuration() + 1) + "...");
	}
	else
	{
		return FText::FromString("");
	}
}

#undef LOCTEXT_NAMESPACE
