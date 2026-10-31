#pragma once
#include "SkinWeightInfo.h"
#include "Render/StaticMeshData.h"

struct FMeshBoneInfo
{
	//본의 이름
	FName Name;

	//부모의 Index 만약 루트면 -1로 둔다
	int32 ParentIndex;

	FMeshBoneInfo(const FName& InName, int32 InParentIndex) : Name(InName), ParentIndex(InParentIndex)
	{};
};

class FSkeletalMeshData
{
public:
	FSkeletalMeshData() = default;
	~FSkeletalMeshData() = default;

	//StaticMeshData를 재활용하는게 이상해 보일 순있지만 애초에 언리얼의 SkeletalMeshData에서도
	//정점 정보는 StaticMesh 구조체를 쓴다. 다른건 섹션과 머테리얼이다.
	//섹션을 따로 둔 이유는 스켈레탈 섹션에 스키닝 정보가 더 있기 때문이다.
	// 예를 들어 BoneMap(이 섹션이 쓰는 뼈 목록), 정점당 최대 영향 수 같은 것이다.
	// GPU 스키닝은 한 번에 넘길 수 있는 뼈 행렬 수에 한계가 있어서, 섹션마다 쓰는 뼈를 따로 관리한다.
	//근데 지금 요구사항은 CPU 스키닝이기때문에 StaticMeshData + Skinweight등의 추가 정보의 형태가 가능한것이다.
	FStaticMeshData Mesh;
	// Mesh.Vertices와 길이가 같다. i번 정점의 스킨 웨이트
	TArray<FSkinWeightInfo> SkinWeights;

	// 부모가 항상 자식보다 앞에 온다. FSkinWeightInfo::BoneIndices가 이 배열의 인덱스다
	TArray<FMeshBoneInfo> Bones;
	// [본] = 바인드 포즈에서 본의 월드(컴포넌트) 행렬. 엔진 축 기준
	TArray<FMatrix> BindPoses;
	// [본] = BindPoses의 역행렬. 스키닝 행렬 = InverseBindPoses[i] * 현재 본 월드 행렬
	TArray<FMatrix> InverseBindPoses;

	void CalculateNormals(TArray<FVertexPNCT>& Vertices, const TArray<uint32>& Indices);
	void CalculateTangents(TArray<FVertexPNCT>& Vertices, const TArray<uint32>& Indices);
};
