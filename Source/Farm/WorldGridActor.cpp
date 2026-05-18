// Copyright (c) 2026 CREEK & RIVER Co., Ltd. - Game Foundry Studio. All rights reserved.

#include "WorldGridActor.h"
#include "Engine/Texture2D.h"
#include "DrawDebugHelpers.h"

//*UEnum::GetDisplayValueAsText(EEnum::Element).ToString()

AWorldGridActor::AWorldGridActor()
{
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Static);
	SetRootComponent(Root);

    for (ETileType TileType : TEnumRange<ETileType>())
	{
		// Skip Empty type: it should not have an HISM and represents a hole
		if (TileType == ETileType::Empty)
		{
			continue;
		}

		CreateInstancedStaticMesh(TileType);
	}

	// Ensure there is at least a default mapping for Empty -> white so simple RGB maps (e.g., MS Paint)
	if (TileTypeColorMap.Num() == 0)
	{
		FTileColorPair EmptyPair;
		EmptyPair.Type = ETileType::Empty;
		EmptyPair.Color = FLinearColor::White;
		TileTypeColorMap.Add(EmptyPair);
	}
	PrimaryActorTick.bCanEverTick = true;
}

ETileType AWorldGridActor::GetTileTypeFromColor(const FColor& Pixel) const
{
	if (TileTypeColorMap.Num() == 0)
	{
		return ETileType::Ground;
	}

	const FLinearColor PixelL = Pixel.ReinterpretAsLinear();

	float BestDist = TNumericLimits<float>::Max();
	ETileType BestType = TileTypeColorMap[0].Type;

	for (const FTileColorPair& Pair : TileTypeColorMap)
	{
		const FLinearColor& C = Pair.Color;
		const float dR = PixelL.R - C.R;
		const float dG = PixelL.G - C.G;
		const float dB = PixelL.B - C.B;
		const float Dist = dR * dR + dG * dG + dB * dB;
		if (Dist < BestDist)
		{
			BestDist = Dist;
			BestType = Pair.Type;
		}
	}

	return BestType;
}

void AWorldGridActor::LoadFromTexture(UTexture2D* Texture)
{
	if (!Texture)
	{
		return;
	}

	// Only perform editor-side loading here
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Use the texture source to get raw pixel data (works in editor for various formats)
	FTextureSource& Source = Texture->Source;
	const int32 Width = Source.GetSizeX();
	const int32 Height = Source.GetSizeY();

	if (Width <= 0 || Height <= 0)
	{
		return;
	}

	// Lock the top mip from the source
	void* DataPtr = Source.LockMip(0);
	if (!DataPtr)
	{
		return;
	}

	// Assume the source is in a 4-byte per pixel format compatible with FColor (BGRA/RGBA)
	const FColor* FormattedImageData = reinterpret_cast<const FColor*>(DataPtr);
	if (!FormattedImageData)
	{
		Source.UnlockMip(0);
		return;
	}

	// Rebuild grid based on texture size and pixel colors
	Tiles.Empty();
	GridSize2D = FIntVector2(Width, Height);

	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			const int32 Index = X + Y * Width;
			const FColor Pixel = FormattedImageData[Index];
			const ETileType Type = GetTileTypeFromColor(Pixel);
			CreateTileAtGridLocation(Type, FIntVector2(X, Y));
		}
	}

	Source.UnlockMip(0);

	// Mark chunks dirty and build instances so preview/editor sees the HISM instances
	SetAllChunkDirty();
}

void AWorldGridActor::BeginPlay()
{
	Super::BeginPlay();

	BuildGrid(true);
}

void AWorldGridActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

#if WITH_EDITOR
	BuildGrid();
#endif
}

void AWorldGridActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if(HasDirtyChunk())
	{
		BuildChunks();
	}

#if WITH_EDITOR
	if(bShowDebug)
	{
		 DebugDraw();
	}
#endif
}

#if WITH_EDITOR
void AWorldGridActor::RegeneratePreview()
{
	BuildGrid(true);
}

void AWorldGridActor::DebugDraw()
{
	UWorld* World = GetWorld();

	if (!World || GridSize2D.X <= 0 || GridSize2D.Y <= 0)
	{
		return;
	}

	if (World->IsGameWorld())
	{
		return;
	}

	const FVector2D TotalSize2D = FVector2D(GridSize2D.X * TileBlockSize.X, GridSize2D.Y * TileBlockSize.Y);
	const FVector Extent = FVector(TotalSize2D.X * 0.5f, TotalSize2D.Y * 0.5f, TileBlockSize.Z * 0.5f);
	const FVector Center = GetActorLocation() + Extent - FVector(TileBlockSize.X * 0.5f, TileBlockSize.Y * 0.5f, 0.0f);

	DrawDebugBox(World, Center, Extent, FQuat::Identity, FColor::Yellow, false, 0.0f, 0, 4.0f);
}
#endif

void AWorldGridActor::CreateInstancedStaticMesh(ETileType Type)
{
	const FString Name = StaticEnum<ETileType>()->GetNameStringByValue((int64)Type);

	UHierarchicalInstancedStaticMeshComponent* HISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(*Name);
	HISM->SetupAttachment(Root);

	HISM->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HISM->SetCollisionResponseToAllChannels(ECR_Block);

	HISM->SetMobility(EComponentMobility::Static);
	HISM->SetCastShadow(false);
	HISM->SetEnableGravity(false);
	HISM->bApplyImpulseOnDamage = false;

	TileInsancedStaticMeshComponents.Add(Type, HISM);

	ChunkDirtyState.Add(Type, true);
}

void AWorldGridActor::CreateTileAtGridLocation(ETileType Type, FIntVector2 GridLocation)
{
	FTileData Data;
	Data.Type = Type;
	Data.GridLocation = GridLocation;
	Data.Location =  FVector(GridLocation.X * TileBlockSize.X, GridLocation.Y * TileBlockSize.Y, 0.0f);
	Data.Transform = FTransform(FRotator::ZeroRotator, Data.Location, FVector::OneVector);
	Tiles.Add(MoveTemp(Data));
}

const FTileData& AWorldGridActor::GetTileAtHitLocation(const FHitResult& HitResult) const
{
	if(!HitResult.bBlockingHit)
	{
		return DummyTile;
	}

	ETileType Type = ETileType::Count;

	for(auto& Pair : TileInsancedStaticMeshComponents)
	{
		if(Pair.Value == HitResult.Component)
		{
			Type = Pair.Key;
			break;
		}
	}

	if(Type == ETileType::Count)
	{
		return DummyTile;
	}

	for(const FTileData& TileData : Tiles)
	{
		if(TileData.Type == Type && TileData.VisualInstanceIndex == HitResult.Item)
		{
			return TileData;
		}
	}

	return DummyTile;
}

const FTileData& AWorldGridActor::GetTileAtGridLocation(FIntVector2 GridLocation) const
{
	for(const FTileData& TileData : Tiles)
	{
		if(TileData.GridLocation == GridLocation)
		{
			return TileData;
		}
	}

	return DummyTile;
}

void AWorldGridActor::BuildGrid(bool bForced/* = false*/)
{
	if(bGridInitialized && !bForced)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("BuildGrid..."));

	SetAllChunkDirty();

	Tiles.Empty();

	for (int32 Y = 0; Y < GridSize2D.Y; ++Y)
	{
		for (int32 X = 0; X < GridSize2D.X; ++X)
		{
			ETileType Type = X % 2 == 0 ? ETileType::Grass : ETileType::Ground; 
			CreateTileAtGridLocation(Type, FIntVector2(X, Y));
		}
	}

	bGridInitialized = true;
}

TWeakObjectPtr<UHierarchicalInstancedStaticMeshComponent> AWorldGridActor::GetHISMForType(ETileType Type)
{
	for(auto& Pair : TileInsancedStaticMeshComponents)
	{
		if(Pair.Key == Type)
		{
			return Pair.Value;
		}
	}

	return nullptr;
}

void AWorldGridActor::BuildChunks()
{
	UE_LOG(LogTemp, Warning, TEXT("BuildChunks..."));

	for(ETileType TileType : TEnumRange<ETileType>())
	{
		PrepareChunk(TileType);
	}

	for(FTileData& TileData : Tiles)
	{
		if(TileData.HISM.IsValid() && TileData.VisualInstanceIndex == -1)
		{
			TileData.VisualInstanceIndex = 	TileData.HISM->AddInstance(TileData.Transform);
		}
	}
}

void AWorldGridActor::PrepareChunk(ETileType Type)
{
	if(!IsChunkDirty(Type))
	{
		return;
	}

	TWeakObjectPtr<UHierarchicalInstancedStaticMeshComponent> HISM = GetHISMForType(Type);
	if(HISM.IsValid())
	{
		HISM->ClearInstances();

		for(FTileData& TileData : Tiles)
		{
			if(TileData.Type != Type)
			{
				continue;
			}

			TileData.HISM = HISM;
			TileData.VisualInstanceIndex = -1;
		}
	}

	SetChunkDirty(Type, false);
}

bool AWorldGridActor::HasDirtyChunk() const
{
	for(const TPair<ETileType, bool>& Pair : ChunkDirtyState)
	{
		if(Pair.Value)
		{
			return true;
		}
	}

	return false;
}

bool AWorldGridActor::IsChunkDirty(ETileType Type) const
{
	for(const TPair<ETileType, bool>& Pair : ChunkDirtyState)
	{
		if(Pair.Key == Type)
		{
			return Pair.Value;
		}
	}

	return false;
}

void AWorldGridActor::SetChunkDirty(ETileType Type, bool bIsDirty)
{
	for(TPair<ETileType, bool>& Pair : ChunkDirtyState)
	{
		if(Pair.Key == Type)
		{
			Pair.Value = bIsDirty;
			return;
		}
	}
}

void AWorldGridActor::SetAllChunkDirty()
{
	for(TPair<ETileType, bool>& Pair : ChunkDirtyState)
	{
		Pair.Value = true;
	}
}

#if WITH_EDITOR
void AWorldGridActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(AWorldGridActor, GridSize2D) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(AWorldGridActor, TileBlockSize))
	{
		BuildGrid();
	}
}
#endif

void AWorldGridActor::PrintTileDebugInfo(const FTileData& TileData) const
{
#if WITH_EDITOR
	if(GEngine)
	{
		const FString Name = StaticEnum<ETileType>()->GetNameStringByValue((int64)TileData.Type);
		const FString Msg = FString::Printf(TEXT("%s tile at grid location {%d ; %d}"), *Name, TileData.GridLocation.X, TileData.GridLocation.Y);

		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, *Msg);
	}
#endif
}
