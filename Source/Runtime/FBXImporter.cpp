#include "EnginePCH.h"
#include "FBXImporter.h"

#include "Core/EngineLog.h"
#include "Render/StaticMeshData.h"
#include "Render/Vertex.h"

#include <algorithm>

namespace
{
	// OpenGL 축(오른손, Y-Up)으로 맞춘 FBX → 엔진 (왼손, X-Forward, Y-Right, Z-Up)
	// ObjImporter의 ToEngine과 같은 규칙이다. 삼각형 winding도 함께 뒤집어야 한다
	FVector ToEngine(const FbxVector4& V)
	{
		return FVector(static_cast<float>(-V[2]), static_cast<float>(V[0]), static_cast<float>(V[1]));
	}

	// 노드 트리를 돌면서 메시마다 월드 변환과 엔진 축 변환을 적용해 Out에 쌓는다
	void CookNode(FbxNode* Node, FStaticMeshData& Out)
	{
		if (FbxMesh* Mesh = Node->GetMesh())
		{
			// ConvertScene은 정점이 아니라 노드 트랜스폼을 바꾼다. 월드 행렬을 곱해야 축 변환이 정점에 반영된다
			const FbxAMatrix World = Node->EvaluateGlobalTransform();

			// 노멀은 이동을 뺀 행렬의 역전치로 변환한다 (비균등 스케일에서도 면에 수직 유지)
			FbxAMatrix NormalMatrix = World;
			NormalMatrix.SetT(FbxVector4(0.0, 0.0, 0.0, 0.0));
			NormalMatrix = NormalMatrix.Inverse().Transpose();

			//ControlPoint : 메시의 고유한 위치 목록. 위치만 있다(로컬 위치임)
			const FbxVector4* ControlPoints = Mesh->GetControlPoints();

			FbxStringList UVSetNames;
			Mesh->GetUVSetNames(UVSetNames);
			const char* UVSetName = UVSetNames.GetCount() > 0 ? UVSetNames[0] : nullptr;

			TArray<uint32> Corners; // 현재 폴리곤의 꼭짓점별 정점 인덱스
			for (int32 Poly = 0; Poly < Mesh->GetPolygonCount(); ++Poly)
			{
				Corners.Reset();
				//이 면의 꼭짓점 개수
				const int32 PolySize = Mesh->GetPolygonSize(Poly);
				for (int32 Corner = 0; Corner < PolySize; ++Corner)
				{
					const int32 ControlPointIndex = Mesh->GetPolygonVertex(Poly, Corner);

					FVertexPNCT Vertex;
					//MultT : 행렬로 점 하나를 변환하는 함수 로컬 -> 월드로 변환에서 버텍스에 넣어준다.
					Vertex.Position = ToEngine(World.MultT(ControlPoints[ControlPointIndex]));

					FbxVector4 Normal;
					if (Mesh->GetPolygonVertexNormal(Poly, Corner, Normal))
					{
						FbxVector4 WorldNormal = NormalMatrix.MultT(Normal);
						WorldNormal.Normalize();
						Vertex.Normal = ToEngine(WorldNormal);
					}

					FbxVector2 UV;
					bool bUnmapped = true;
					if (UVSetName && Mesh->GetPolygonVertexUV(Poly, Corner, UVSetName, UV, bUnmapped) && !bUnmapped)
					{
						// OBJ와 같이 V를 뒤집는다 (FBX는 아래가 0, 엔진은 위가 0)
						Vertex.UV = FVector2(static_cast<float>(UV[0]), 1.0f - static_cast<float>(UV[1]));
					}
					Vertex.Color = FVector4(1.0f, 1.0f, 1.0f, 1.0f);

					Corners.Add(static_cast<uint32>(Out.Vertices.Num()));
					Out.Vertices.Add(std::move(Vertex));
				}

				// 삼각형 분할 (fan). FBX는 CCW, 엔진은 CW가 앞면이라 (0, k, k+1) → (0, k+1, k)
				for (int32 k = 1; k + 1 < Corners.Num(); ++k)
				{
					Out.Indices.Add(Corners[0]);
					Out.Indices.Add(Corners[k + 1]);
					Out.Indices.Add(Corners[k]);
				}
			}
		}

		for (int32 i = 0; i < Node->GetChildCount(); ++i)
		{
			CookNode(Node->GetChild(i), Out);
		}
	}
}


bool FFBXImporter::ParseFBX(const FString& Path, FbxScene* Out)
{
    //FBX매니저 생성 모든 SDK 객체는 Manager로부터 만들어진다
    FbxManager* Manager = FbxManager::Create();
    //임포트/익스포트 옵션 객체
    FbxIOSettings* IOS = FbxIOSettings::Create(Manager, IOSROOT);
    //만든 설정을 매니저에 등록
    Manager->SetIOSettings(IOS);
    //임포터 생성
    FbxImporter* Importer = FbxImporter::Create(Manager, "");
    //파일을 읽고 포맷을 판별,실패하면 false를 반환
    if (!Importer->Initialize(Path.c_str(), -1, Manager->GetIOSettings()))
    {
        HTR_LOG(Warning, "[FBX] {}: {}", Path, Importer->GetStatus().GetErrorString());
        return false;
    }
    //읽을 데이터를 담을 빈 씬을 생성
	Out = FbxScene::Create(Manager, "Scene");
    //담기
    Importer->Import(Out);
    //임포트 파괴
    Importer->Destroy();

    return true;
}

bool FFBXImporter::Cook(FbxScene* Scene, FStaticMeshData& Out)
{
    FStaticMeshData Cooked;

    // 파일 축이 무엇이든 OBJ와 같은 OpenGL 축(오른손, Y-Up)으로 맞춘다
    // 왼손으로의 변환은 거울 반전이라 노드 트랜스폼에 넣으면 노멀,winding이 꼬인다. 그래서 ToEngine에서 한다 (언리얼도 SDK로는 오른손까지만 바꾼다)
    if (Scene->GetGlobalSettings().GetAxisSystem() != FbxAxisSystem::OpenGL)
    {
        FbxAxisSystem::OpenGL.ConvertScene(Scene);
    }

	// 오목 다각형도 올바르게 삼각형으로 바꾼다. 이후 모든 폴리곤은 꼭짓점이 3개다
	FbxGeometryConverter Converter(Scene->GetFbxManager());
	if (!Converter.Triangulate(Scene, true))
	{
		HTR_LOG(Warning, "[FBX] triangulate failed, using fan triangulation");
	}


    CookNode(Scene->GetRootNode(), Cooked);

    if (Cooked.Vertices.IsEmpty())
    {
        HTR_LOG(Error, "[FBX] no mesh");
        return false;
    }

    Cooked.CalculateTangents(Cooked.Vertices, Cooked.Indices);

    // AABB (변환이 끝난 Position 기준)
    FVector Min = Cooked.Vertices[0].Position;
    FVector Max = Min;
    for (const FVertexPNCT& Vertex : Cooked.Vertices)
    {
        Min.X = std::min(Min.X, Vertex.Position.X);
        Min.Y = std::min(Min.Y, Vertex.Position.Y);
        Min.Z = std::min(Min.Z, Vertex.Position.Z);
        Max.X = std::max(Max.X, Vertex.Position.X);
        Max.Y = std::max(Max.Y, Vertex.Position.Y);
        Max.Z = std::max(Max.Z, Vertex.Position.Z);
    }
    Cooked.AABB.Min = Min;
    Cooked.AABB.Max = Max;

    FString Error;
    if (!Cooked.Validate(Error))
    {
        HTR_LOG(Error, "[FBX] {}", Error);
        return false;
    }

    Out = std::move(Cooked);
    return true;
}
