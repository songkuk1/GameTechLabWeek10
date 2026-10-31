#include "EnginePCH.h"
#include "SkeletalMeshComponent.h"

#include "Engine/SkeletalMeshSceneProxy.h"
#include "Render/RenderPacket.h"

USkeletalMeshComponent::USkeletalMeshComponent()
	: MeshObject(MakeUnique<FSkeletalMeshObject>())
{
}

FPrimitiveSceneProxy* USkeletalMeshComponent::CreateSceneProxy()
{
	// 레벨 로드처럼 SetSkeletalMesh를 거치지 않고 포인터만 채워진 경우를 맞춘다
	MeshObject->SyncMesh(SkeletalMesh);
	return new FSkeletalMeshSceneProxy(this);
}

void USkeletalMeshComponent::SubmitToRenderQueue(FRenderQueue& RenderQueue)
{
	// CreateSceneProxy가 항상 스켈레탈 프록시를 만든다
	const FSkeletalMeshSceneProxy* Proxy = static_cast<const FSkeletalMeshSceneProxy*>(GetSceneProxy());
	if (!Proxy || !Proxy->HasRenderMesh()) return;

	FVertexBuffer* VertexBuffer = Proxy->GetVertexBuffer();
	FIndexBuffer* IndexBuffer = Proxy->GetIndexBuffer();
	if (!IndexBuffer) return;

	for (const FCachedMeshSection& Section : Proxy->GetSections())
	{
		FRenderPacket& Packet = RenderQueue.AddDefaulted_GetRef();
		Packet.Proxy = Proxy;
		Packet.VertexBuffer = VertexBuffer;
		Packet.IndexBuffer = IndexBuffer;
		Packet.Material = Section.Material;
		Packet.StartIndex = Section.StartIndex;
		Packet.IndexCount = Section.IndexCount;
	}
}

void USkeletalMeshComponent::PostEditChangeProperty(const FProperty& Property)
{
	Super::PostEditChangeProperty(Property);
	if (Property.Name == "SkeletalMesh")
	{
		MeshObject->SyncMesh(SkeletalMesh);
		ClearOverrideMaterials();
		MarkRenderStateDirty();
		OnTransformDirty();
	}
}

void USkeletalMeshComponent::SetSkeletalMesh(USkeletalMesh* InSkeletalMesh)
{
	if (SkeletalMesh == InSkeletalMesh)
		return;

	SkeletalMesh = InSkeletalMesh;
	MeshObject->Init(SkeletalMesh);
	ClearOverrideMaterials();
	MarkRenderStateDirty();
	// 메시가 바뀌면 바운드도 바뀐다
	OnTransformDirty();
}

const FStaticMeshData* USkeletalMeshComponent::GetMeshData() const
{
	const FSkeletalMeshData* Data = SkeletalMesh ? SkeletalMesh->GetSkeletalMeshData() : nullptr;
	return Data ? &Data->Mesh : nullptr;
}

int32 USkeletalMeshComponent::GetNumMaterials() const
{
	const FStaticMeshData* Data = GetMeshData();
	return Data ? static_cast<int32>(Data->MaterialSlots.Num()) : 0;
}

FString USkeletalMeshComponent::GetMaterialSlotName(int32 SlotIndex) const
{
	const FStaticMeshData* Data = GetMeshData();
	if (!Data || SlotIndex < 0 || SlotIndex >= static_cast<int32>(Data->MaterialSlots.Num()))
		return FString();
	return Data->MaterialSlots[SlotIndex].Name;
}

UMaterial* USkeletalMeshComponent::GetDefaultMaterial(int32 SlotIndex) const
{
	return SkeletalMesh && SlotIndex >= 0 ? SkeletalMesh->GetMaterial(static_cast<uint32>(SlotIndex)) : nullptr;
}
