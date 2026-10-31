#include "EnginePCH.h"
#include "SkeletalMeshObject.h"
#include "Render/SkeletalMesh.h"
#include "Render/RenderCommand.h"

void FSkeletalMeshObject::Init(const USkeletalMesh* InMesh)
{
	SourceMesh = InMesh;
	SkinnedVertices.Reset();
	VertexBuffer.reset();

	const FSkeletalMeshData* Data = InMesh ? InMesh->GetSkeletalMeshData() : nullptr;
	if (!Data || Data->Mesh.Vertices.IsEmpty())
		return;

	// 스키닝 전에는 바인드 포즈 그대로 그린다
	SkinnedVertices = Data->Mesh.Vertices;
	const uint32 Bytes = sizeof(FVertexPNCT) * static_cast<uint32>(SkinnedVertices.Num());
	VertexBuffer = RenderCommand::CreateDynamicVertexBuffer(Bytes, sizeof(FVertexPNCT));
	UploadSkinnedVertices();
}

void FSkeletalMeshObject::UploadSkinnedVertices()
{
	if (!VertexBuffer || SkinnedVertices.IsEmpty())
		return;

	const uint32 Bytes = sizeof(FVertexPNCT) * static_cast<uint32>(SkinnedVertices.Num());
	RenderCommand::UpdateBufferData(VertexBuffer.get(), SkinnedVertices.GetData(), Bytes);
}
