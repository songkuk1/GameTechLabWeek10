#include "EnginePCH.h"
#include "StaticMeshSceneProxy.h"
#include "Component/StaticMeshComponent.h"
#include "Render/Mesh.h"
#include "Engine/Scene.h"

FStaticMeshSceneProxy::FStaticMeshSceneProxy(UStaticMeshComponent* InComponent)
	: FPrimitiveSceneProxy(InComponent)
	, StaticMeshComponent(InComponent)
{
}

FStaticMeshSceneProxy::~FStaticMeshSceneProxy()
{
	if (Mesh) Mesh->UnregisterProxy(this);
}

void FStaticMeshSceneProxy::OnMeshRenderDataChanged()
{
	if (FScene* Scene = GetScene())
	{
		Scene->MarkRenderStateDirty(this);
		Scene->MarkDirty(this);
	}
}

void FStaticMeshSceneProxy::OnMeshDestroyed()
{
	Mesh = nullptr;
	RenderState.reset();
}

void FStaticMeshSceneProxy::UpdateTransform()
{
	FPrimitiveSceneProxy::UpdateTransform();
	LODSphere = Mesh ? Mesh->GetLocalLODSphere().Transform(GetLocalToWorld()) : FLODSphere{};
}

void FStaticMeshSceneProxy::UpdateRenderState()
{
	FPrimitiveSceneProxy::UpdateRenderState();

	UStaticMesh* PreviousMesh = Mesh;
	Mesh = StaticMeshComponent->GetStaticMesh();
	RenderState.reset();
	if (Mesh != PreviousMesh)
	{
		if (PreviousMesh) PreviousMesh->UnregisterProxy(this);
		if (Mesh) Mesh->RegisterProxy(this);
		// 메시가 바뀌면 바운드도 바뀌므로 Transform 갱신을 예약한다
		if (FScene* Scene = GetScene()) Scene->MarkDirty(this);
	}
	if (!Mesh) return;

	std::vector<UMaterial*> Materials;
	Materials.reserve(Mesh->MeshData.MaterialSlots.Num());
	for (uint32 Slot = 0; Slot < static_cast<uint32>(Mesh->MeshData.MaterialSlots.Num()); ++Slot)
		Materials.push_back(StaticMeshComponent->GetMaterial(Slot));
	RenderState = Mesh->GetRenderState(Materials);
}

FBox FStaticMeshSceneProxy::GetLocalBounds() const
{
	return Mesh ? Mesh->GetRenderBounds() : FPrimitiveSceneProxy::GetLocalBounds();
}
