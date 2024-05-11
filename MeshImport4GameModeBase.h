// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/StaticMesh.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "ProceduralMeshComponent.h"
#include "Importer.hpp"
#include <scene.h>
#include "postprocess.h"
#include "UObject/Object.h"
#include <mesh.h>
#include <material.h>
#include <texture.h>
#include <assimp/material.h>
#include "MeshImport4GameModeBase.generated.h"
 




UCLASS(BlueprintType)
class MESHIMPORT4_API UMeshNode : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
		FString NodeName;


	UPROPERTY(BlueprintReadOnly)
		UMeshNode* Parent;

	
	UFUNCTION(BlueprintCallable, Category = "FBX Node")
		const TArray<UMeshNode*>& GetChildren() const { return Children; }


	void AddChild(UMeshNode* Child);

protected:

	UPROPERTY(BlueprintReadOnly)
		TArray<UMeshNode*> Children;

};


UCLASS()
class MESHIMPORT4_API AMeshImport4GameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	AMeshImport4GameModeBase();
public:
	virtual void BeginPlay() override;
	
	UFUNCTION(BlueprintCallable, Category = "Mesh Import")
	void ImportSingleMeshFromFBX(const FString& BaseFilePath, int32 Index, const FVector& SpawnLocation, float DelayInSeconds);

	void ProcessNode(aiNode* Node, const aiScene* Scene, const FVector& SpawnLocation, UMeshNode* ParentNode);
	//FQuat ExtractRotation(const aiMatrix4x4& Matrix);
	FVector TransformVertex(const FVector& Vertex, const aiNode* Node);
	UMeshNode_Geometry* ImportMesh(aiMesh* ImportedMesh, aiNode* MeshNode, aiMaterial* Material, const FVector& SpawnLocation, const aiScene* Scene,AActor* NewActor);
	UTexture2D* LoadTexture2D(const FString& TexturePath, const aiTexture* EmbeddedTexture);
	//bool IsFBXLeftHanded(const aiScene* Scene);
	UFUNCTION(BlueprintCallable)
	      void Spawner(FString FilePath, float Delay, FVector SpawnLocation,int32 Index);

	UProceduralMeshComponent* NewMesh;

	static FName ConvertToTextureParameter(aiTextureType Type);

	EMaterialShadingModel GetUnrealShadingModel(aiShadingMode shadingMode);
	TArray<AActor*> PreviousSpawnedActor;


	UMaterialInstanceDynamic* ImportMaterial(const aiScene* Scene,int MaterialIndex,aiMesh* Mesh);

	//UTexture2D* LoadTexture2DFromEmbedded(const aiTexture* EmbeddedTexture);

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
         	TArray<AActor*>SpawnedActor;

	//void RetrieveEmbeddedTextureData(const aiTexture* EmbeddedTexture, uint32& OutWidth, uint32& OutHeight, uint8*& OutData);


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FMeshTextureTypeMapping> TextureTypeMappings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FMeshColorTypeMapping> ColorTypeMappings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FMeshScalarTypeMapping> ScalarTypeMappings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FBX Import Settings")
		TArray<FTexturePathMapping> TexturePathMappings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material Settings")
		TArray<FMaterialMapping> MaterialMappings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material Settings")
		UMaterialInterface* BaseMaterialOpaque;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material Settings")
		UMaterialInterface* BaseMaterialMasked;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material Settings")
		UMaterialInterface* BaseMaterialTranslucent;

	UPROPERTY(BlueprintReadOnly)
	TArray<UMeshNode*> RootNodes;


};

