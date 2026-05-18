// Copyright (c) 2026 CREEK & RIVER Co., Ltd. - Game Foundry Studio. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "WorldGridActor.generated.h"

class UTexture2D;

UENUM(BlueprintType)
enum class ETileType : uint8
{
    Empty,
	Grass,
	Ground,

	// Add new items BEFORE this
	Count UMETA(Hidden)
};

USTRUCT(BlueprintType)
struct FTileColorPair
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Grid")
	ETileType Type = ETileType::Grass;

	// Color picker in editor
	UPROPERTY(EditAnywhere, Category = "Grid")
	FLinearColor Color = FLinearColor::White;
};
ENUM_RANGE_BY_COUNT(ETileType, ETileType::Count)

USTRUCT(BlueprintType)
struct FTileData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FTransform Transform;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector Location;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FIntVector2 GridLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 VisualInstanceIndex = -1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	ETileType Type = ETileType::Grass;

	TWeakObjectPtr<UHierarchicalInstancedStaticMeshComponent> HISM;
};

UCLASS()
class FARM_API AWorldGridActor : public AActor
{
	GENERATED_BODY()

public:
	AWorldGridActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable)
	void CreateTileAtGridLocation(ETileType Type, FIntVector2 GridLocation);

	UFUNCTION(BlueprintCallable)
	const FTileData& GetTileAtGridLocation(FIntVector2 GridLocation) const;

	UFUNCTION(BlueprintCallable)
	const FTileData& GetTileAtHitLocation(const FHitResult& HitResult) const;

	UFUNCTION(BlueprintCallable, BlueprintPure = false)
	void PrintTileDebugInfo(const FTileData& TileData) const;

	// Load grid from a texture: texture size becomes GridSize2D, pixel colors map to tile types.
	UFUNCTION(CallInEditor, Category = "Grid")
	void LoadFromTexture(UTexture2D* Texture);

#if WITH_EDITOR
	// make Tick run in editor
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }

	// invoked when a property changes in editor
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Grid")
	void RegeneratePreview();

	void DebugDraw();
#endif

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void CreateInstancedStaticMesh(ETileType Type);

	void BuildGrid(bool bForced = false);
	void BuildChunks();
	void PrepareChunk(ETileType Type);

	TWeakObjectPtr<UHierarchicalInstancedStaticMeshComponent> GetHISMForType(ETileType Type);

	bool HasDirtyChunk() const;
	bool IsChunkDirty(ETileType Type) const;
	void SetChunkDirty(ETileType Type, bool bIsDirty);
	void SetAllChunkDirty();

	ETileType GetTileTypeFromColor(const FColor& Pixel) const;

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TMap<ETileType, TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> TileInsancedStaticMeshComponents;

	UPROPERTY(EditDefaultsOnly, Category = "Grid")
	FIntVector2 GridSize2D = FIntVector2(10);

	UPROPERTY(EditDefaultsOnly, Category = "Grid")
	FVector TileBlockSize = FVector(100.f);

	TArray<FTileData> Tiles;

	TMap<ETileType, bool> ChunkDirtyState;

	UPROPERTY(EditAnywhere, Category = "Grid")
	TArray<FTileColorPair> TileTypeColorMap;

	bool bGridInitialized = false;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Grid")
	bool bShowDebug = false;

	const FTileData DummyTile;
};
