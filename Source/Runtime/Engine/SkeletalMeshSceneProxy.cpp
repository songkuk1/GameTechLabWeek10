#include "EnginePCH.h"
#include "SkeletalMeshSceneProxy.h"
#include "Component/SkeletalMeshComponent.h"
#include "Engine/Scene.h"

FSkeletalMeshSceneProxy::FSkeletalMeshSceneProxy(USkeletalMeshComponent* InComponent)
	: FPrimitiveSceneProxy(InComponent)
	, SkeletalMeshComponent(InComponent)
{
}

void FSkeletalMeshSceneProxy::UpdateRenderState()
{
	FPrimitiveSceneProxy::UpdateRenderState();

	USkeletalMesh* PreviousMesh = SkeletalMesh;
	SkeletalMesh = SkeletalMeshComponent->GetSkeletalMesh();
	MeshObject = SkeletalMeshComponent->GetMeshObject();
	Sections.Reset();

	// 메시가 바뀌면 바운드도 바뀌므로 Transform 갱신을 예약한다
	if (SkeletalMesh != PreviousMesh)
	{
		if (FScene* Scene = GetScene()) Scene->MarkDirty(this);
	}

	const FSkeletalMeshData* Data = SkeletalMesh ? SkeletalMesh->GetSkeletalMeshData() : nullptr;
	if (!Data) return;

	// 스태틱의 FMeshRenderState와 같은 형태로 섹션을 캐시한다. 스켈레탈은 LOD가 없어 섹션 목록 하나뿐이다
	for (const FStaticMeshSection& Section : Data->Mesh.Sections)
	{
		UMaterial* Material = SkeletalMeshComponent->GetMaterial(static_cast<int32>(Section.MaterialSlotIndex));
		if (Material) Sections.Add({ Material, Section.StartIndex, Section.IndexCount });
	}
}

FVertexBuffer* FSkeletalMeshSceneProxy::GetVertexBuffer() const
{
	return MeshObject ? MeshObject->GetVertexBuffer() : nullptr;
}

FIndexBuffer* FSkeletalMeshSceneProxy::GetIndexBuffer() const
{
	return SkeletalMesh ? SkeletalMesh->IndexBuffer.get() : nullptr;
}

FBox FSkeletalMeshSceneProxy::GetLocalBounds() const
{
	const FSkeletalMeshData* Data = SkeletalMesh ? SkeletalMesh->GetSkeletalMeshData() : nullptr;
	return Data ? Data->Mesh.AABB : FPrimitiveSceneProxy::GetLocalBounds();
}
