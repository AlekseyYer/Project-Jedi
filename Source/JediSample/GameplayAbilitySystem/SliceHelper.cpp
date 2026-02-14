#include "SliceHelper.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "KismetProceduralMeshLibrary.h"

void USliceHelper::CopySkeletalMeshToProceduralMesh(
    USkeletalMeshComponent* SkeletalMesh,
    UProceduralMeshComponent* ProcMesh)
{
    if (!SkeletalMesh || !ProcMesh) return;

    USkinnedMeshComponent* SkinnedMesh = Cast<USkinnedMeshComponent>(SkeletalMesh);
    if (!SkinnedMesh) return;

    FSkeletalMeshRenderData* RenderData = SkeletalMesh->GetSkeletalMeshRenderData();
    if (!RenderData || RenderData->LODRenderData.Num() == 0) return;

    ProcMesh->ClearAllMeshSections();

    // Use the lowest LOD available for cheaper slicing
    int32 LODIndex = RenderData->LODRenderData.Num() - 1;
    FSkeletalMeshLODRenderData& LODData = RenderData->LODRenderData[LODIndex];

    UE_LOG(LogTemp, Warning, TEXT("Using LOD %d (of %d). "), LODIndex, RenderData->LODRenderData.Num());

    TArray<uint32> IndexBuffer;
    LODData.MultiSizeIndexContainer.GetIndexBuffer(IndexBuffer);

    int32 NumSections = LODData.RenderSections.Num();

    for (int32 SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
    {
        FSkelMeshRenderSection& Section = LODData.RenderSections[SectionIdx];

        TArray<FVector> Vertices;
        TArray<int32> Triangles;
        TArray<FVector> Normals;
        TArray<FVector2D> UVs;
        TArray<FProcMeshTangent> Tangents;

        uint32 NumVerts = Section.NumVertices;
        uint32 BaseVertIdx = Section.BaseVertexIndex;

        Vertices.SetNum(NumVerts);
        Normals.SetNum(NumVerts);
        UVs.SetNum(NumVerts);
        Tangents.SetNum(NumVerts);

        for (uint32 VertIdx = 0; VertIdx < NumVerts; VertIdx++)
        {
            uint32 GlobalIdx = BaseVertIdx + VertIdx;

            Vertices[VertIdx] = FVector(SkinnedMesh->GetSkinnedVertexPosition(
                SkinnedMesh,
                GlobalIdx,
                LODData,
                LODData.SkinWeightVertexBuffer
            ));

            FVector3f RawNormal = LODData.StaticVertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(GlobalIdx);
            Normals[VertIdx] = FVector(RawNormal);

            FVector2f RawUV = LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetVertexUV(GlobalIdx, 0);
            UVs[VertIdx] = FVector2D(RawUV);

            FVector3f RawTangent = LODData.StaticVertexBuffers.StaticMeshVertexBuffer.VertexTangentX(GlobalIdx);
            Tangents[VertIdx] = FProcMeshTangent(FVector(RawTangent), false);
        }

        uint32 NumTriangles = Section.NumTriangles;
        uint32 BaseIndex = Section.BaseIndex;

        for (uint32 TriIdx = 0; TriIdx < NumTriangles; TriIdx++)
        {
            uint32 I0 = IndexBuffer[BaseIndex + TriIdx * 3 + 0] - BaseVertIdx;
            uint32 I1 = IndexBuffer[BaseIndex + TriIdx * 3 + 1] - BaseVertIdx;
            uint32 I2 = IndexBuffer[BaseIndex + TriIdx * 3 + 2] - BaseVertIdx;

            Triangles.Add((int32)I0);
            Triangles.Add((int32)I1);
            Triangles.Add((int32)I2);
        }

        ProcMesh->CreateMeshSection(
            SectionIdx,
            Vertices,
            Triangles,
            Normals,
            UVs,
            TArray<FColor>(),
            Tangents,
            true
        );

        ProcMesh->SetMaterial(SectionIdx, SkeletalMesh->GetMaterial(SectionIdx));
    }

    
    ProcMesh->SetMobility(EComponentMobility::Movable);
    ProcMesh->bUseComplexAsSimpleCollision = false;
    ProcMesh->SetCollisionObjectType(ECC_PhysicsBody);
    ProcMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    ProcMesh->SetCollisionResponseToAllChannels(ECR_Block);
    ProcMesh->RecreatePhysicsState();
}

void USliceHelper::UpdateProceduralMeshPositions(
    USkeletalMeshComponent* SkeletalMesh,
    UProceduralMeshComponent* ProcMesh)
{
    if (!SkeletalMesh || !ProcMesh) return;

    USkinnedMeshComponent* SkinnedMesh = Cast<USkinnedMeshComponent>(SkeletalMesh);
    if (!SkinnedMesh) return;

    FSkeletalMeshRenderData* RenderData = SkeletalMesh->GetSkeletalMeshRenderData();
    if (!RenderData || RenderData->LODRenderData.Num() == 0) return;

    FSkeletalMeshLODRenderData& LODData = RenderData->LODRenderData[0];
    int32 NumSections = LODData.RenderSections.Num();

    for (int32 SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
    {
        FProcMeshSection* ProcSection = ProcMesh->GetProcMeshSection(SectionIdx);
        if (!ProcSection) continue;

        FSkelMeshRenderSection& Section = LODData.RenderSections[SectionIdx];
        uint32 NumVerts = Section.NumVertices;
        uint32 BaseVertIdx = Section.BaseVertexIndex;

        TArray<FVector> NewPositions;
        TArray<FVector> Normals;
        TArray<FVector2D> UVs;
        TArray<FColor> Colors;
        TArray<FProcMeshTangent> Tangents;

        NewPositions.SetNum(NumVerts);
        Normals.SetNum(NumVerts);
        UVs.SetNum(NumVerts);
        Tangents.SetNum(NumVerts);

        for (uint32 VertIdx = 0; VertIdx < NumVerts; VertIdx++)
        {
            uint32 GlobalIdx = BaseVertIdx + VertIdx;

            NewPositions[VertIdx] = FVector(
                SkinnedMesh->GetSkinnedVertexPosition(
                    SkinnedMesh,
                    GlobalIdx,
                    LODData,
                    LODData.SkinWeightVertexBuffer
                )
            );

            // Keep existing normals, UVs, tangents
            Normals[VertIdx] = FVector(ProcSection->ProcVertexBuffer[VertIdx].Normal);
            UVs[VertIdx] = ProcSection->ProcVertexBuffer[VertIdx].UV0;
            Tangents[VertIdx] = ProcSection->ProcVertexBuffer[VertIdx].Tangent;
        }

        ProcMesh->UpdateMeshSection(
            SectionIdx,
            NewPositions,
            Normals,
            UVs,
            Colors,
            Tangents
        );
    }
}





