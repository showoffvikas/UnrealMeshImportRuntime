#include "MeshImport4GameModeBase.h"
#include "DrawDebugHelpers.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/Material.h"
#include <assimp/Importer.hpp>
#include <Runtime/Engine/Classes/Engine/Texture2D.h>
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "types.h"
#include "HAL/FileManager.h"
#include "AssetToolsModule.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ImageUtils.h"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include <DDSLoader.h>
#include "Engine/Texture2D.h"
#include "Engine/Texture2DDynamic.h"
#include "AssetRegistry/AssetRegistryModule.h"






using namespace Assimp;
using namespace std;

AMeshImport4GameModeBase::AMeshImport4GameModeBase()
{
}

void AMeshImport4GameModeBase::BeginPlay()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Hello, World!"));
	Super::BeginPlay();

	FString FilePath = TEXT("C:/Users/acer/Desktop/newfbxfiles/"); 
	int32 Index = 1;
	FVector MeshSpawnLocation(0.0f, 0.0f, 0.0f);

	const float DelayInSeconds = 1.f; 

	
//	GetWorld()->GetTimerManager().SetTimerForNextTick([this, FilePath, Index, MeshSpawnLocation, DelayInSeconds]() {
	//	ImportSingleMeshFromFBX(FilePath, Index, MeshSpawnLocation, DelayInSeconds);
		//});
}

void AMeshImport4GameModeBase::Spawner(FString FilePath, float Delay, FVector SpawnLocation,int32 Index)
{
    
    GetWorld()->GetTimerManager().SetTimerForNextTick([this, FilePath, Index, SpawnLocation, Delay]() {
        ImportSingleMeshFromFBX(FilePath, Index, SpawnLocation, Delay);
        });
    if (PreviousSpawnedActor.Num() > 0) {
        for (int i = 0; i < PreviousSpawnedActor.Num(); i++) {
            if (PreviousSpawnedActor[i])
                PreviousSpawnedActor[i]->SetActorHiddenInGame(true);
            PreviousSpawnedActor[i]->Destroy();
        }
    }
}



void AMeshImport4GameModeBase::ImportSingleMeshFromFBX(const FString& FilePath, int32 Index, const FVector& SpawnLocation, float DelayInSeconds)
{
    FString FileToImport = FString::Printf(TEXT("(%d).fbx"), Index);

    FString FullFilePath = FilePath + FileToImport;

   
    if (!FPaths::FileExists(FullFilePath))
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("File not found: %s"), *FullFilePath));
        return;
    }

   
    Assimp::Importer Importer;
    std::string FilePathStr(TCHAR_TO_UTF8(*FullFilePath));

    FString ErrorMessage;
    bool bIsSuccessful = false;

   
    const aiScene* Scene = Importer.ReadFile(FilePathStr, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_FlipWindingOrder);

    if (!Scene || Scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !Scene->mRootNode)
    {
        ErrorMessage = FString(Importer.GetErrorString());
        UE_LOG(LogTemp, Error, TEXT("Error importing FBX file: %s"), *ErrorMessage);
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("Error importing FBX file: %s"), *ErrorMessage));
    }
    else
    {
       
        RootNodes.Empty();
        ProcessNode(Scene->mRootNode, Scene, SpawnLocation, nullptr);

        bIsSuccessful = true;
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Imported Mesh Path: %s"), *FullFilePath));

        Index++;

       
        FString NextFileToImport = FString::Printf(TEXT("(%d).fbx"), Index);

        FString NextFullFilePath = FilePath + NextFileToImport;

        if (FPaths::FileExists(NextFullFilePath))
        {
            FTimerDelegate TimerDelegate;
            TimerDelegate.BindLambda([=]() {
                ImportSingleMeshFromFBX(FilePath, Index, SpawnLocation, DelayInSeconds);
                });

            FTimerHandle TimerHandle;
            GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, DelayInSeconds, false);
        }
    }
}


void AMeshImport4GameModeBase::ProcessNode(aiNode* Node, const aiScene* Scene, const FVector& SpawnLocation, UMeshNode* ParentNode)
{
   
    UMeshNode* CurrentNode = nullptr;
    AActor* NewActor = GetWorld()->SpawnActor<AActor>();
    
    for (uint32 MeshIndex = 0; MeshIndex < Node->mNumMeshes; MeshIndex++)
    {

        aiMesh* Mesh = Scene->mMeshes[Node->mMeshes[MeshIndex]];
        aiMaterial* Material = Scene->mMaterials[Mesh->mMaterialIndex];

        UMeshNode_Geometry* NewMeshNode = ImportMesh(Mesh, Node, Material, SpawnLocation, Scene,NewActor);

        NewMeshNode->NodeName = FString(Mesh->mName.C_Str());
        if (ParentNode)
        {
            ParentNode->AddChild(NewMeshNode);
        }
        else
        {
            RootNodes.Add(NewMeshNode);
        }



        CurrentNode = NewMeshNode;
    }

    if (CurrentNode == nullptr)
    {
        CurrentNode = NewObject<UMeshNode>();
        CurrentNode->NodeName = FString(Node->mName.C_Str());
        CurrentNode->Parent = ParentNode;
       
        if (ParentNode)
        {
            ParentNode->AddChild(CurrentNode);
        }
        else
        {
            RootNodes.Add(CurrentNode);
        }

    }

    
    for (uint32 ChildIndex = 0; ChildIndex < Node->mNumChildren; ChildIndex++)
    {
        ProcessNode(Node->mChildren[ChildIndex], Scene, SpawnLocation, CurrentNode);
    }
  

}
FVector AMeshImport4GameModeBase::TransformVertex(const FVector& Vertex, const aiNode* Node)
{
    aiMatrix4x4 Transform = Node->mTransformation;
    FMatrix UnrealTransform(
        FVector(Transform.a1, Transform.b1, Transform.c1),
        FVector(Transform.a2, Transform.b2, Transform.c2),
        FVector(Transform.a3, Transform.b3, Transform.c3),
        FVector(Transform.a4, Transform.b4, Transform.c4)
    );

    return UnrealTransform.TransformPosition(Vertex);
    

}

UMeshNode_Geometry* AMeshImport4GameModeBase::ImportMesh(aiMesh* ImportedMesh, aiNode* MeshNode, aiMaterial* Material, const FVector& SpawnLocation, const aiScene* Scene, AActor* NewActor)
{
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UV0;
    TArray<FProcMeshTangent> Tangents;
  
    FProcMeshSection FBXNewSection = FProcMeshSection();
    FBXNewSection.ProcVertexBuffer.Reset();
    FBXNewSection.ProcVertexBuffer.AddUninitialized(ImportedMesh->mNumVertices);
   

    for (uint32 VertexIndex = 0; VertexIndex < ImportedMesh->mNumVertices; VertexIndex++)
    {
        FProcMeshVertex& ProcMeshVertex = FBXNewSection.ProcVertexBuffer[VertexIndex];
        aiVector3D Vertex = ImportedMesh->mVertices[VertexIndex];
        FVector UnrealVertex(Vertex.x, Vertex.y, Vertex.z);
        UnrealVertex = TransformVertex(UnrealVertex, MeshNode);
        Vertices.Add(UnrealVertex);
        ProcMeshVertex.Position = UnrealVertex;

        if (ImportedMesh->HasNormals())
        {
            aiVector3D Normal = ImportedMesh->mNormals[VertexIndex];
            Normals.Add(FVector(Normal.x, Normal.y, Normal.z));
            ProcMeshVertex.Normal = FVector(Normal.x, Normal.y, Normal.z);
        }
        else
        {
            ProcMeshVertex.Normal = FVector(0.f, 0.f, 1.f);
        }

        if (ImportedMesh->HasTextureCoords(0))
        {
            aiVector3D UV = ImportedMesh->mTextureCoords[0][VertexIndex];
            UV0.Add(FVector2D(UV.x, UV.y));
            ProcMeshVertex.UV0 = FVector2D(UV.x, UV.y);
        }
        else
        {
            ProcMeshVertex.UV0 = FVector2D(0.f, 0.f);
        }

        if (ImportedMesh->HasTextureCoords(1))
        {
            aiVector3D LightmapUV = ImportedMesh->mTextureCoords[1][VertexIndex];
            ProcMeshVertex.UV1 = FVector2D(LightmapUV.x, LightmapUV.y);
        }

        FProcMeshTangent MeshTangent;
        if (ImportedMesh->HasTangentsAndBitangents() && ImportedMesh->mTangents)
        {
            MeshTangent.TangentX = FVector(ImportedMesh->mTangents[VertexIndex].x, ImportedMesh->mTangents[VertexIndex].y, ImportedMesh->mTangents[VertexIndex].z);
            MeshTangent.bFlipTangentY = true;
            Tangents.Add(MeshTangent);
        }
        ProcMeshVertex.Tangent = MeshTangent;
       



        FBXNewSection.SectionLocalBox += ProcMeshVertex.Position;
    }

    for (uint32 FaceIndex = 0; FaceIndex < ImportedMesh->mNumFaces; FaceIndex++)
    {
        aiFace Face = ImportedMesh->mFaces[FaceIndex];
        for (uint32 IndexIndex = 0; IndexIndex < Face.mNumIndices; IndexIndex++)
        {
            Triangles.Add(Face.mIndices[IndexIndex]);
            FBXNewSection.ProcIndexBuffer.Add(Face.mIndices[IndexIndex]);
        }
    }
    
  
    UProceduralMeshComponent* ProceduralMeshComponent = NewObject<UProceduralMeshComponent>(NewActor);
   
    ProceduralMeshComponent->RegisterComponentWithWorld(GetWorld());

    ProceduralMeshComponent->SetProcMeshSection(0, FBXNewSection);
    
    ProceduralMeshComponent->bCastDynamicShadow = true;
    ProceduralMeshComponent->bAffectDynamicIndirectLighting = true;

    NewActor->AddInstanceComponent(ProceduralMeshComponent);
    NewActor->SetRootComponent(ProceduralMeshComponent);
    
    ProceduralMeshComponent->bEnableAutoLODGeneration = false;
    int MaterialIndex = ImportedMesh->mMaterialIndex;

   
    if (MaterialIndex >= 0 && MaterialIndex < static_cast<int>(Scene->mNumMaterials))
    {
       
        

        // Set the same material for each LOD section
      
        UMaterialInstanceDynamic* Mat = ImportMaterial(Scene, MaterialIndex, ImportedMesh);

       if (Mat)
       {
            
               ProceduralMeshComponent->SetMaterial(0, Mat);
            
        }
       
    }


    UProceduralMeshData* MeshData = NewObject<UProceduralMeshData>();
    MeshData->Vertices = Vertices;
    MeshData->Triangles = Triangles;
    MeshData->Normals = Normals;
    MeshData->UV0 = UV0;
    MeshData->VertexColors = TArray<FLinearColor>();
    MeshData->Tangents = Tangents;

    FString NodeName = FString(ImportedMesh->mName.C_Str());
    UMeshNode_Geometry* NewMeshNode = NewObject<UMeshNode_Geometry>(this, UMeshNode_Geometry::StaticClass(), NAME_None);
    NewMeshNode->NodeName = NodeName;
    NewMeshNode->MeshComponent = NewMesh;
    NewMeshNode->ProceduralMeshData = MeshData;

    aiMatrix4x4 Transform = MeshNode->mTransformation;

  
    aiVector3D Scaling;
    aiQuaternion Rotation;
    aiVector3D Translation;
    Transform.Decompose(Scaling, Rotation, Translation);

    FTransform UnrealTransform;

   
    bool IsLeftHanded = (Transform.a1 * Transform.b2 * Transform.c3 < 0);

    
    FQuat QuatRotation(Rotation.x, Rotation.z, Rotation.y, -Rotation.w);

   
    FRotator EulerRotation = QuatRotation.Rotator();
    EulerRotation.Roll += (IsLeftHanded ? -270.0f : 270.0f); 
    UnrealTransform.SetRotation(EulerRotation.Quaternion());

   
    UnrealTransform.SetTranslation(FVector(Translation.x, Translation.y, Translation.z));

    FVector ScaleFactor = FVector(1.0f, 1.0f, 1.0f);
    UnrealTransform.SetScale3D(UnrealTransform.GetScale3D() * ScaleFactor);
    ProceduralMeshComponent->SetRelativeTransform(UnrealTransform);
    if (IsLeftHanded) ProceduralMeshComponent->SetWorldRotation(FRotator(0, 0, -90));
    else ProceduralMeshComponent->SetWorldRotation(FRotator(0, 0, -90));
  
    NewActor->SetActorLocation(SpawnLocation);
    
    PreviousSpawnedActor.Add(NewActor);

    return NewMeshNode;







}


void UMeshNode::AddChild(UMeshNode* Child)
{
    Children.Add(Child);
    Child->Parent = this;
}





UMaterialInstanceDynamic* AMeshImport4GameModeBase::ImportMaterial(const aiScene* Scene, int MaterialIndex, aiMesh* Mesh)
{
    aiString TexturePath;
    aiTextureMapping TextureMapping;
    uint32 UVIndex;
    float BlendFactor;
   
    aiBlendMode aiBlend;
    aiGetMaterialIntegerArray(Scene->mMaterials[MaterialIndex], AI_MATKEY_BLEND_FUNC, reinterpret_cast<int*>(&aiBlend), nullptr);

    aiShadingMode aiShading;
    aiGetMaterialIntegerArray(Scene->mMaterials[MaterialIndex], AI_MATKEY_SHADING_MODEL, reinterpret_cast<int*>(&aiShading), nullptr);

    
    
    const aiTextureType textureTypes[] = {
        aiTextureType_DIFFUSE,
        aiTextureType_SPECULAR,
        aiTextureType_NORMALS,
        aiTextureType_EMISSIVE,
        aiTextureType_METALNESS,
        aiTextureType_DIFFUSE_ROUGHNESS,
        aiTextureType_AMBIENT_OCCLUSION,
        aiTextureType_OPACITY,
        aiTextureType_SHININESS
    };

    bool hasEmbeddedTextures = false; 
    if ((MaterialIndex >= 0) && (static_cast<unsigned int>(MaterialIndex) < Scene->mNumMaterials))
    {
        aiMaterial* Material = Scene->mMaterials[MaterialIndex];
        if (Material)
        {
            aiString MaterialName;
            Material->Get(AI_MATKEY_NAME, MaterialName);

            FString MaterialNameString(MaterialName.C_Str());
            FText MaterialNameText = FText::FromString(MaterialNameString);
            GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Blue, MaterialNameText.ToString());
        }
        FString MaterialPath = "/Game/LitOpa";
       
      

        float Opacity = 1.0f;
        Scene->mMaterials[MaterialIndex]->Get(AI_MATKEY_OPACITY, Opacity);

        // Checking for blend mode
        aiBlendMode BlendMode;
        Scene->mMaterials[MaterialIndex]->Get(AI_MATKEY_BLEND_FUNC, BlendMode);

        if (Opacity < 1.0f)
        {
           MaterialPath = "/Game/LitTran";
        }
        else if (BlendMode == aiBlendMode_Default)
        {
            MaterialPath = "/Game/LitMas";
        }
        else
        {
            MaterialPath = "/Game/LitOpa";
        }
        UMaterialInterface* BaseMaterial = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, *MaterialPath));
        UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, nullptr);
       
        for (const aiTextureType& textureType : textureTypes)
        {
            
            if (Material->GetTexture(textureType, 0, &TexturePath, &TextureMapping, &UVIndex, &BlendFactor) == AI_SUCCESS)
            {
                FString TexturePathString(TexturePath.C_Str());
                FString FullTexturePath = FPaths::Combine(FPaths::ProjectContentDir(), TexturePathString);
                UE_LOG(LogTemp, Warning, TEXT("TexturePath: %s"), *TexturePathString);
                const aiTexture* EmbeddedTexture = Scene->GetEmbeddedTexture(TexturePath.C_Str());

                if (EmbeddedTexture)
                {
                    hasEmbeddedTextures = true;
                    UTexture2D* Texture = LoadTexture2D(FullTexturePath, EmbeddedTexture);

                    if (Texture)
                    {                  
                            if (DynamicMaterial)
                            {
                                FString TextureTypeString;
                                switch (textureType)
                                {
                                case aiTextureType_DIFFUSE:
                                    TextureTypeString = "DiffuseTexture";
                                    DynamicMaterial->SetScalarParameterValue(TEXT("ContainDiffuse"), 1);
                                    DynamicMaterial->SetTextureParameterValue(FName(*TextureTypeString), Texture);
                                    break;
                                case aiTextureType_SPECULAR:
                                    TextureTypeString = "SpecularTexture";
                                    DynamicMaterial->SetScalarParameterValue(TEXT("ContainSpecular"), 1);
                                    DynamicMaterial->SetTextureParameterValue(FName(*TextureTypeString), Texture);
                                    break;
                                case aiTextureType_DIFFUSE_ROUGHNESS:
                                    TextureTypeString = "RoughnessTexture";
                                    DynamicMaterial->SetScalarParameterValue(TEXT("ContainRoughness"), 1);
                                    DynamicMaterial->SetTextureParameterValue(FName(*TextureTypeString), Texture);
                                    break;
                                case aiTextureType_EMISSIVE:
                                    TextureTypeString = "EmissiveTexture";
                                    DynamicMaterial->SetScalarParameterValue(TEXT("ContainEmissive"), 1);
                                    DynamicMaterial->SetTextureParameterValue(FName(*TextureTypeString), Texture);
                                    break;
                                case aiTextureType_METALNESS:
                                    TextureTypeString = "MetallicTexture";
                                    DynamicMaterial->SetScalarParameterValue(TEXT("ContainMetallic"), 1);
                                    DynamicMaterial->SetTextureParameterValue(FName(*TextureTypeString), Texture);
                                    break;

                                case aiTextureType_NORMALS:
                                    TextureTypeString = "NormalTexture";
                                    DynamicMaterial->SetScalarParameterValue(TEXT("ContainNormal"), 1);
                                    DynamicMaterial->SetTextureParameterValue(FName(*TextureTypeString), Texture);
                                    break;
                                
                                case aiTextureType_OPACITY:
                                    TextureTypeString = "OpacityTexture";
                                    DynamicMaterial->SetScalarParameterValue(TEXT("ContainOpacity"), 1);
                                    DynamicMaterial->SetTextureParameterValue(FName(*TextureTypeString), Texture);
                                    break;
                                
                                case aiTextureType_AMBIENT_OCCLUSION:
                                    TextureTypeString = "AOTexture";
                                    DynamicMaterial->SetScalarParameterValue(TEXT("ContainAO"), 1);
                                    DynamicMaterial->SetTextureParameterValue(FName(*TextureTypeString), Texture);
                                    break;
                                
                                case aiTextureType_SHININESS:
                                    TextureTypeString = "RoughnessTexture";
                                    DynamicMaterial->SetScalarParameterValue(TEXT("ContainRoughness"), 1);
                                    DynamicMaterial->SetTextureParameterValue(FName(*TextureTypeString), Texture);
                                    break;
                                 
                                default:
                                    TextureTypeString = "UnknownTextureType";
                                    break;
                                }

                               

                                FString WidthHeightString = FString::Printf(TEXT("Embedded Texture Size: Width=%d, Height=%d"), Texture->GetSizeX(), Texture->GetSizeY());
                                GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::White, WidthHeightString);

                               
                                UE_LOG(LogTemp, Warning, TEXT("Texture Type: %s"), *TextureTypeString);
                                UE_LOG(LogTemp, Warning, TEXT("Embedded Texture Size: Width=%d, Height=%d"), Texture->GetSizeX(), Texture->GetSizeY());

                                continue;
                            }
                        
                    }
                    
                   
                }
               
                
            }

            else {
                
                FString TextureTypeString;
                aiColor4D BaseColor;
                float value;
                switch (textureType)
                {
                case aiTextureType_DIFFUSE:

                    if (Material->Get(AI_MATKEY_COLOR_DIFFUSE, BaseColor) == AI_SUCCESS) {
                        DynamicMaterial->SetScalarParameterValue(TEXT("ContainDiffuse"), -1);
                        float R = BaseColor.r;
                        float G = BaseColor.g;
                        float B = BaseColor.b;
                        float A = BaseColor.a;
                        DynamicMaterial->SetVectorParameterValue("DiffuseColor", FLinearColor(R, G, B,A));

                    }
                    else {
                        DynamicMaterial->SetScalarParameterValue(TEXT("ContainDiffuse"), 0);
                    }
                    break;
                case aiTextureType_SPECULAR:

                    if (Material->Get(AI_MATKEY_COLOR_SPECULAR, BaseColor) == AI_SUCCESS) {
                        DynamicMaterial->SetScalarParameterValue(TEXT("ContainSpecular"), -1);
                        float R = BaseColor.r;
                        float G = BaseColor.g;
                        float B = BaseColor.b;
                        float A = BaseColor.a;
                        DynamicMaterial->SetVectorParameterValue("SpecularColor", FLinearColor(R, G, B,A));

                    }
                    else {
                        DynamicMaterial->SetScalarParameterValue(TEXT("ContainSpecular"), 0);
                    }
                    break;
                case aiTextureType_DIFFUSE_ROUGHNESS:
                    value = 0.f;
                    if (Material->Get(AI_MATKEY_ROUGHNESS_FACTOR, value) == AI_SUCCESS) {
                        DynamicMaterial->SetScalarParameterValue(TEXT("ContainRoughness"), -1);
                        DynamicMaterial->SetScalarParameterValue(TEXT("RoughnessColor"), value);

                    }
                    else {
                        DynamicMaterial->SetScalarParameterValue(TEXT("ContainRoughness"), 0);
                    }
                    break;
                case aiTextureType_EMISSIVE:

                    if (Material->Get(AI_MATKEY_COLOR_EMISSIVE, BaseColor) == AI_SUCCESS) {
                        DynamicMaterial->SetScalarParameterValue(TEXT("ContainEmissive"), -1);
                        float R = BaseColor.r;
                        float G = BaseColor.g;
                        float B = BaseColor.b;
                        float A = BaseColor.a;
                        DynamicMaterial->SetVectorParameterValue("EmissiveColor", FLinearColor(R, G, B,A));

                    }
                    else {
                        DynamicMaterial->SetScalarParameterValue(TEXT("ContainEmissive"), 0);
                    }
                    break;
                case aiTextureType_METALNESS:
                    value = 0.f;
                    if (Material->Get(AI_MATKEY_METALLIC_FACTOR, value) == AI_SUCCESS) {
                        DynamicMaterial->SetScalarParameterValue(TEXT("ContainMetallic"), -1);
                        DynamicMaterial->SetScalarParameterValue(TEXT("MetallicColor"), value);

                    }
                    else {
                        DynamicMaterial->SetScalarParameterValue(TEXT("ContainMetallic"), 0);
                    }
                    break;

                case aiTextureType_NORMALS:
                    DynamicMaterial->SetScalarParameterValue(TEXT("ContainNormal"), 0);
                    break;

                case aiTextureType_OPACITY:
                    value = 0.f;
                    if (Material->Get(AI_MATKEY_OPACITY, value) == AI_SUCCESS) {
                        DynamicMaterial->SetScalarParameterValue(TEXT("ContainOpacity"), -1);
                        DynamicMaterial->SetScalarParameterValue(TEXT("OpacityColor"), value);

                    }
                    else {
                        DynamicMaterial->SetScalarParameterValue(TEXT("ContainOpacity"), 0);
                    }
                    break;

                case aiTextureType_AMBIENT_OCCLUSION:                  
                    DynamicMaterial->SetScalarParameterValue(TEXT("ContainAO"), 0);
                    break;
     
                case aiTextureType_SHININESS:
                    value = 0.f;
                    if (Material->Get(AI_MATKEY_SHININESS_STRENGTH, value) == AI_SUCCESS) {
                        DynamicMaterial->SetScalarParameterValue(TEXT("ContainRoughness"), -1);
                        DynamicMaterial->SetScalarParameterValue(TEXT("RoughnessColor"), value);

                    }
                    else {
                        DynamicMaterial->SetScalarParameterValue(TEXT("ContainRoughness"), 0);
                    }
                    break;
                default:

                    break;
                }
                
            }
        }
        return DynamicMaterial;
        
       
    }
    return nullptr;
}



UTexture2D* AMeshImport4GameModeBase::LoadTexture2D(const FString& TexturePath, const aiTexture* EmbeddedTexture)
{
    IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));

    TSharedPtr<IImageWrapper> ImageWrapper;

    FString FileExtension = FString(EmbeddedTexture->achFormatHint).ToLower();

    EImageFormat ImageFormat = EImageFormat::Invalid;
    if (FileExtension == TEXT("jpg"))
    {
        ImageFormat = EImageFormat::JPEG;
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Nice its Jpg"));
    }
    else if (FileExtension == TEXT("png"))
    {
        ImageFormat = EImageFormat::PNG;
    }

    if (ImageFormat == EImageFormat::Invalid)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("Unsupported image format for texture: %s"), *TexturePath));
        return nullptr;
    }

    ImageWrapper = ImageWrapperModule.CreateImageWrapper(ImageFormat);
    if (!ImageWrapper.IsValid())
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("Failed to create image wrapper for texture: %s"), *TexturePath));
        return nullptr;
    }


    if (ImageWrapper->SetCompressed(EmbeddedTexture->pcData, EmbeddedTexture->mWidth))
    {
        TArray<uint8> RawData;
        if (ImageWrapper->GetRaw(ERGBFormat::RGBA, 8, RawData))
        {

            for (int32 i = 0; i < FMath::Min(16, RawData.Num()); ++i)
            {
                UE_LOG(LogTemp, Warning, TEXT("RawData[%d]: %d"), i, RawData[i]);
            }

            UTexture2D* Texture = UTexture2D::CreateTransient(ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), PF_R8G8B8A8);
            if (Texture)
            {
                void* TextureDataPtr = Texture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
                FMemory::Memcpy(TextureDataPtr, RawData.GetData(), RawData.Num());
                Texture->PlatformData->Mips[0].BulkData.Unlock();
                Texture->UpdateResource();

                return Texture;
            }
            else
            {
                GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("Failed to create texture: %s"), *TexturePath));
            }
        }
        else
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("Failed to get raw data for texture: %s"), *TexturePath));
        }
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("Failed to set compressed data for texture: %s"), *TexturePath));
    }

    return nullptr;
}
