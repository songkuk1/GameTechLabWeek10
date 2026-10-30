#pragma once
#include "EnginePCH.h"
#include <fbxsdk.h>

struct FStaticMeshData;

class FFBXImporter
{
public:
	FFBXImporter() = default;
	~FFBXImporter() = default;

	FFBXImporter(const FFBXImporter&) = delete;
	FFBXImporter& operator=(const FFBXImporter&) = delete;

	static TUniquePtr<FStaticMeshData> LoadStaticMeshData(const FString& Path);

private:
	// .fbx 파일 경로 -> FbxScene 변환
	static bool ParseFBX(FbxManager* Manager, const FString& Path, FbxScene*& OutScene);
	// FbxScene -> FStaticMeshData 변환 (축 변환이 씬을 수정하므로 const가 아니다)
	// Path는 텍스처 파일을 FBX 위치 기준으로 찾을 때 쓴다
	static bool Cook(const FString& Path, FbxScene* Scene, FStaticMeshData& Out);
};