#pragma once
#include "EnginePCH.h"
#include "Render/Buffer.h"
#include "Render/Vertex.h"

class USkeletalMesh;

// 컴포넌트 하나의 스키닝 결과 (언리얼의 FSkeletalMeshObject).
// 같은 USkeletalMesh를 쓰는 컴포넌트도 자세가 다르므로 정점 버퍼를 컴포넌트마다 따로 가진다
// CPU 스키닝이라 정점 레이아웃은 스태틱과 같은 FVertexPNCT이고, 매 프레임 Dynamic 버퍼를 덮어쓴다
class FSkeletalMeshObject
{
public:
	FSkeletalMeshObject() = default;
	~FSkeletalMeshObject() = default;

	FSkeletalMeshObject(const FSkeletalMeshObject&) = delete;
	FSkeletalMeshObject& operator=(const FSkeletalMeshObject&) = delete;

	// 메시가 바뀔 때 부른다. 버퍼를 메시 크기로 다시 만들고 바인드 포즈 정점을 올린다
	// 객체는 그대로 두고 내용만 바꾸므로 프록시가 들고 있는 포인터는 계속 유효하다
	void Init(const USkeletalMesh* InMesh);
	// 리플렉션(로드, 디테일 패널)으로 메시 포인터만 바뀐 경우를 위해, 메시가 다를 때만 Init한다
	void SyncMesh(const USkeletalMesh* InMesh) { if (InMesh != SourceMesh) Init(InMesh); }

	// 스키닝 결과를 CPU 배열에 쓴 뒤 부르면 GPU 버퍼에 올린다
	void UploadSkinnedVertices();

	TArray<FVertexPNCT>& GetSkinnedVertices() { return SkinnedVertices; }
	const TArray<FVertexPNCT>& GetSkinnedVertices() const { return SkinnedVertices; }
	FVertexBuffer* GetVertexBuffer() const { return VertexBuffer.get(); }

private:
	const USkeletalMesh* SourceMesh = nullptr;
	TArray<FVertexPNCT> SkinnedVertices;
	TUniquePtr<FVertexBuffer> VertexBuffer;
};
