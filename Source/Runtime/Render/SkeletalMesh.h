#pragma once
#include "EnginePCH.h"
#include "Asset/RenderAsset.h"
#include "Render/Buffer.h"
#include "SkeletalMeshData.h"
#include "SkinWeightInfo.h"

class UMaterial;

// FMeshBoneInfo는 임포터도 써야 해서 SkeletalMeshData.h로 옮겼다

struct FReferenceSkeleton
{
public:
	// 쿡된 본 목록과 바인드 포즈(월드 행렬)로 채운다. 포즈는 부모 기준 로컬 트랜스폼으로 바꿔 저장한다
	void Build(const TArray<FMeshBoneInfo>& Bones, const TArray<FMatrix>& BindPoses);

	int32 GetNum() const { return static_cast<int32>(RawRefBoneInfo.Num()); }
	const TArray<FMeshBoneInfo>& GetRawRefBoneInfo() const { return RawRefBoneInfo; }
	const TArray<FTransform>& GetRawRefBonePose() const { return RawRefBonePose; }
	// 없으면 INDEX_NONE(-1)
	int32 FindBoneIndex(const FName& BoneName) const;

private:

	//원본 에셋(FBX)에 실제로 있는 본
	TArray<FMeshBoneInfo> RawRefBoneInfo;
	TArray<FTransform>RawRefBonePose;

};


class USkeletalMesh : public URenderAsset
{
	DECLARE_CLASS(USkeletalMesh, URenderAsset)
public:
	USkeletalMesh() = default;
	~USkeletalMesh() = default;
	USkeletalMesh(const USkeletalMesh&) = delete;
	USkeletalMesh& operator=(const USkeletalMesh&) = delete;

	// 쿡된 데이터를 넘겨받고 ReferenceSkeleton을 만든다
	void SetSkeletalMeshData(TUniquePtr<FSkeletalMeshData> InData);

	const FSkeletalMeshData* GetSkeletalMeshData() const { return SkeletalMeshData.get(); }
	const FReferenceSkeleton* GetReferenceSkeleton() const { return ReferenceSkeleton.get(); }
	// 스킨 웨이트는 SkeletalMeshData가 정점과 같은 순서로 들고 있다
	const TArray<FSkinWeightInfo>& GetSkinWeights() const { return SkeletalMeshData->SkinWeights; }
	UMaterial* GetMaterial(uint32 SlotIndex) const;

	// Renderer가 실제로 Bind할 런타임 재질 객체. MeshData.MaterialSlots와 순서가 같다
	TArray<UMaterial*> Materials;

	// CPU 스키닝 결과를 매 프레임 덮어쓰므로 Dynamic이다. 처음에는 바인드 포즈 정점이 들어 있다
	TUniquePtr<FVertexBuffer> VertexBuffer;
	TUniquePtr<FIndexBuffer> IndexBuffer;

private:
	TUniquePtr<FSkeletalMeshData> SkeletalMeshData;
	//fbx상의 skeleton
	TUniquePtr<FReferenceSkeleton> ReferenceSkeleton;

};
