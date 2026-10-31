#pragma once
#include "EnginePCH.h"
#include <fbxsdk.h>
#include "Render/SkeletalMeshData.h"

struct FStaticMeshData;

// 스킨이 있으면 Skeletal, 없으면 Static만 채운다. 둘 다 비어 있으면 실패
struct FFbxMeshResult
{
	TUniquePtr<FStaticMeshData> Static;
	TUniquePtr<FSkeletalMeshData> Skeletal;
};

class FFBXImporter
{
public:
	FFBXImporter() = default;
	~FFBXImporter() = default;

	FFBXImporter(const FFBXImporter&) = delete;
	FFBXImporter& operator=(const FFBXImporter&) = delete;

	static FFbxMeshResult LoadMeshData(const FString& Path);

private:
	// .fbx 파일 경로 -> FbxScene 변환
	static bool ParseFBX(FbxManager* Manager, const FString& Path, FbxScene*& OutScene);
	// FbxScene -> FStaticMeshData 변환 (축 변환이 씬을 수정하므로 const가 아니다)
	// Path는 텍스처 파일을 FBX 위치 기준으로 찾을 때 쓴다
	static bool CookStatic(const FString& Path, FbxScene* Scene, FStaticMeshData& Out);
	static bool CookSkeletal(const FString& Path, FbxScene* Scene, FSkeletalMeshData& Out);
	static bool HasSkin(FbxScene* Scene);
};