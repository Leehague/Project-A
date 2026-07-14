using UnityEngine;
using UnityEditor;
using UnityEditor.SceneManagement;
using System.IO;
using System.Text;
using System.Collections.Generic;
using UnityEngine.AI;

#region JSON Data Models
[System.Serializable]
public class MapInfo
{
    public int id;
    public string name;
    public string MapPath;
    public string NavMeshPath;
    public float MinX;
    public float MinZ;
    public float CellSize;
    public int Width;
    public int Height;
}

[System.Serializable]
public class MapDataList
{
    public List<MapInfo> Maps = new List<MapInfo>();
}

[System.Serializable]
public class SceneSelection
{
    public string name;
    public string path;
    public bool isSelected = true;
}

// [추가] 클라이언트용 SceneStructure JSON 모델 정의
[System.Serializable]
public class SceneStructureInfo
{
    public int id;
    public string name;
    public string type;
}
[System.Serializable]
public class SceneStructureDataList
{
    public List<SceneStructureInfo> sceneStructures = new List<SceneStructureInfo>();
}
#endregion

public static class MapDataJsonUpdater
{
    private static string GetJsonPath()
    {
        return Path.GetFullPath(Path.Combine(Application.dataPath, "../../Common/Data/OnlyForServer/MapData.json"));
    }

    // [추가] Client용 SceneStructureData.json 경로
    private static string GetSceneStructureJsonPath()
    {
        return Path.GetFullPath(Path.Combine(Application.dataPath, "../../Common/Data/OnlyForClient/SceneStructureData.json"));
    }

    public static void UpdateMapData(string mapName, float minX, float minZ, float cellSize, int width, int height)
    {
        string jsonPath = GetJsonPath();
        MapDataList mapDataList = new MapDataList();

        if (File.Exists(jsonPath))
        {
            try
            {
                string jsonText = File.ReadAllText(jsonPath);
                mapDataList = JsonUtility.FromJson<MapDataList>(jsonText);
            }
            catch (System.Exception ex)
            {
                Debug.LogError($"[JSON Load Error] {ex.Message}");
                mapDataList = new MapDataList();
            }
        }

        if (mapDataList.Maps == null) mapDataList.Maps = new List<MapInfo>();

        // 1. 해당 맵 이름으로 검색하여 값 할당 혹은 추가
        MapInfo targetMap = mapDataList.Maps.Find(m => m.name == mapName);
        if (targetMap == null)
        {
            targetMap = new MapInfo { name = mapName };
            mapDataList.Maps.Add(targetMap);
        }

        targetMap.MapPath = $"Resource/Maps/{mapName}.txt";
        targetMap.NavMeshPath = $"Resource/Maps/{mapName}_NavMesh.bin";
        targetMap.MinX = minX;
        targetMap.MinZ = minZ;
        targetMap.CellSize = cellSize;
        targetMap.Width = width;
        targetMap.Height = height;

        // ----------------------------------------------------
        // 2. ID 자동 동기화 처리 (시스템 씬과 정렬 연동)
        // ----------------------------------------------------

        // 기본이 되는 클라이언트 전용 기본 씬 고정 정의 (ID 1, 2 선점)
        List<SceneStructureInfo> systemScenes = new List<SceneStructureInfo>()
        {
            new SceneStructureInfo { id = 1, name = "Login", type = "Login" },
            new SceneStructureInfo { id = 2, name = "Loading", type = "Loading" }
        };
        // 맵 툴로 스캔된 게임 씬들은 시스템 씬 개수 다음 번호인 (3번)부터 ID를 새로 부여합니다.
        int startId = systemScenes.Count + 1; // 3
        for (int i = 0; i < mapDataList.Maps.Count; i++)
        {
            mapDataList.Maps[i].id = startId + i;
        }
        // SceneStructureData를 위한 새 리스트 인스턴스 준비
        SceneStructureDataList sceneStructureList = new SceneStructureDataList();
        sceneStructureList.sceneStructures.AddRange(systemScenes);
        // 서버 맵 리스트의 ID와 매핑 정보를 그대로 클라이언트 씬 데이터에 입력
        foreach (MapInfo map in mapDataList.Maps)
        {
            sceneStructureList.sceneStructures.Add(new SceneStructureInfo
            {
                id = map.id,
                name = map.name,
                type = "Game" // 맵 툴로 스캔되는 씬들은 항상 플레이 영역인 "Game" 타입으로 분류
            });
        }
        // ----------------------------------------------------
        // 3. 각각 지정된 경로로 JSON 파일들 저장
        // ----------------------------------------------------

        // 3-A. MapData.json 저장 (Common/Data/OnlyForServer)
        try
        {
            string dir = Path.GetDirectoryName(jsonPath);
            if (!Directory.Exists(dir)) Directory.CreateDirectory(dir);
            string updatedJson = JsonUtility.ToJson(mapDataList, true);
            File.WriteAllText(jsonPath, updatedJson);
            Debug.Log($"[MapData.json 업데이트 완료] 총 {mapDataList.Maps.Count}개 맵 리스트가 정렬되었습니다.");
        }
        catch (System.Exception ex)
        {
            Debug.LogError($"[MapData.json Save Error] {ex.Message}");
        }
        // 3-B. SceneStructureData.json 저장 (Common/Data/OnlyForClient)
        string sceneJsonPath = GetSceneStructureJsonPath();
        try
        {
            string dir = Path.GetDirectoryName(sceneJsonPath);
            if (!Directory.Exists(dir)) Directory.CreateDirectory(dir);
            string updatedJson = JsonUtility.ToJson(sceneStructureList, true);
            File.WriteAllText(sceneJsonPath, updatedJson);
            Debug.Log($"[SceneStructureData.json 업데이트 완료] 기본 씬 {systemScenes.Count}개 및 게임 맵 씬 {mapDataList.Maps.Count}개 동기화가 완료되었습니다.");
        }
        catch (System.Exception ex)
        {
            Debug.LogError($"[SceneStructureData.json Save Error] {ex.Message}");
        }
    }
}


public class MapExporter : EditorWindow
{
    [MenuItem("Tools/Export Map Collision")]
    public static void ShowWindow()
    {
        GetWindow<MapExporter>("Map Exporter");
    }

    private Vector3 minBound;
    private Vector3 maxBound;
    private float cellSize = 0.5f;
    private string mapName = "Map_01";
    private bool autoCalculateBounds = true;

    // ★ 요구사항 2: 씬 선택 리스트 관리 변수
    private List<SceneSelection> sceneSelections = new List<SceneSelection>();
    private Vector2 scrollPos;

    void OnEnable()
    {
        mapName = EditorSceneManager.GetActiveScene().name;
        if (string.IsNullOrEmpty(mapName)) mapName = "Map_01";
        if (autoCalculateBounds) CalculateBoundsAutomatically();

        RefreshSceneList();
    }

    /// <summary>
    /// 프로젝트 폴더 내의 모든 씬 파일을 검색하여 리스트를 갱신합니다.
    /// </summary>
    void RefreshSceneList()
    {
        sceneSelections.Clear();

        // [수정] 스캔할 대상을 실제 맵 씬 폴더로 지정
        string targetDir = "Assets/Scenes/Maps";

        // 예외 대응: 폴더가 존재하지 않을 때만 전체 씬 폴더로 변경
        if (!Directory.Exists(targetDir))
            targetDir = "Assets/Scenes";
        string[] sceneGuids = AssetDatabase.FindAssets("t:Scene", new[] { targetDir });
        foreach (string guid in sceneGuids)
        {
            string path = AssetDatabase.GUIDToAssetPath(guid);
            string name = Path.GetFileNameWithoutExtension(path);

            // [방어 코드] 만약 폴더가 없어서 fallback 되었을 때를 대비해 시스템 씬은 제외
            if (name == "Login" || name == "Loading")
                continue;
            sceneSelections.Add(new SceneSelection { name = name, path = path, isSelected = true });
        }
    }

    void OnGUI()
    {
        // 1. 공통 속성 설정
        EditorGUILayout.LabelField("Export Settings", EditorStyles.boldLabel);
        cellSize = EditorGUILayout.FloatField("Cell Size", cellSize);
        autoCalculateBounds = EditorGUILayout.Toggle("Auto Calculate Bounds", autoCalculateBounds);

        using (new EditorGUI.DisabledGroupScope(autoCalculateBounds))
        {
            minBound = EditorGUILayout.Vector3Field("Min Bound", minBound);
            maxBound = EditorGUILayout.Vector3Field("Max Bound", maxBound);
        }
        EditorGUILayout.Space();

        // 2. ★ 요구사항 2: 씬 선택 인터페이스 그리기
        EditorGUILayout.LabelField("Select Scenes to Export", EditorStyles.boldLabel);

        EditorGUILayout.BeginHorizontal();
        if (GUILayout.Button("Select All"))
        {
            foreach (var s in sceneSelections) s.isSelected = true;
        }
        if (GUILayout.Button("Deselect All"))
        {
            foreach (var s in sceneSelections) s.isSelected = false;
        }
        if (GUILayout.Button("Refresh Scenes"))
        {
            RefreshSceneList();
        }
        EditorGUILayout.EndHorizontal();

        scrollPos = EditorGUILayout.BeginScrollView(scrollPos, GUILayout.Height(180));
        foreach (var selection in sceneSelections)
        {
            selection.isSelected = EditorGUILayout.ToggleLeft($" {selection.name}  ({selection.path})", selection.isSelected);
        }
        EditorGUILayout.EndScrollView();

        EditorGUILayout.Space();

        // 3. 실행 버튼
        if (GUILayout.Button("Export Selected Scenes & NavMesh (Batch)", GUILayout.Height(30)))
        {
            ExportSelectedScenesBatch();
        }
    }

    public void CalculateBoundsAutomatically()
    {
        NavMeshTriangulation tri = NavMesh.CalculateTriangulation();
        if (tri.vertices != null && tri.vertices.Length > 0)
        {
            Vector3 min = tri.vertices[0];
            Vector3 max = tri.vertices[0];
            foreach (Vector3 v in tri.vertices)
            {
                min = Vector3.Min(min, v);
                max = Vector3.Max(max, v);
            }
            minBound = new Vector3(Mathf.Floor(min.x) - 2f, 0f, Mathf.Floor(min.z) - 2f);
            maxBound = new Vector3(Mathf.Ceil(max.x) + 2f, 0f, Mathf.Ceil(max.z) + 2f);
            return;
        }

        Collider[] colliders = FindObjectsByType<Collider>(FindObjectsSortMode.None);
        if (colliders != null && colliders.Length > 0)
        {
            Bounds combinedBounds = new Bounds();
            bool hasFirst = false;
            int groundLayer = LayerMask.NameToLayer("Ground");
            int obstacleLayer = LayerMask.NameToLayer("Obstacle");

            foreach (var col in colliders)
            {
                if (col.gameObject.layer == groundLayer || col.gameObject.layer == obstacleLayer)
                {
                    if (!hasFirst) { combinedBounds = col.bounds; hasFirst = true; }
                    else combinedBounds.Encapsulate(col.bounds);
                }
            }

            if (hasFirst)
            {
                minBound = new Vector3(Mathf.Floor(combinedBounds.min.x) - 2f, 0f, Mathf.Floor(combinedBounds.min.z) - 2f);
                maxBound = new Vector3(Mathf.Ceil(combinedBounds.max.x) + 2f, 0f, Mathf.Ceil(combinedBounds.max.z) + 2f);
                return;
            }
        }

        minBound = new Vector3(-50, 0, -50);
        maxBound = new Vector3(50, 0, 50);
    }

    /// <summary>
    /// 선택한 씬들을 순회하며 일괄적으로 데이터를 내보내고 JSON을 갱신합니다.
    /// </summary>
    private void ExportSelectedScenesBatch()
    {
        string originalScenePath = EditorSceneManager.GetActiveScene().path;

        if (!EditorSceneManager.SaveCurrentModifiedScenesIfUserWantsTo())
        {
            Debug.LogWarning("[Batch] 사용자가 저장 확인을 취소했습니다.");
            return;
        }

        int successCount = 0;
        string exportDir = Application.dataPath + "/../MapData/";
        if (!Directory.Exists(exportDir)) Directory.CreateDirectory(exportDir);

        foreach (var selection in sceneSelections)
        {
            if (!selection.isSelected) continue;

            // 1. 씬 열기
            EditorSceneManager.OpenScene(selection.path, OpenSceneMode.Single);

            // 2. 바운드 계산
            if (autoCalculateBounds) CalculateBoundsAutomatically();

            // 3. NavMesh (.bin) 저장
            NavMeshTriangulation tri = NavMesh.CalculateTriangulation();
            if (tri.vertices != null && tri.vertices.Length > 0)
            {
                string binPath = Path.Combine(exportDir, $"{selection.name}_NavMesh.bin");
                using (BinaryWriter writer = new BinaryWriter(File.Open(binPath, FileMode.Create)))
                {
                    writer.Write(tri.vertices.Length);
                    foreach (Vector3 v in tri.vertices)
                    {
                        writer.Write(v.x); writer.Write(v.y); writer.Write(v.z);
                    }
                    writer.Write(tri.indices.Length);
                    foreach (int index in tri.indices)
                    {
                        writer.Write(index);
                    }
                }
            }

            // 4. 충돌 정보 (.txt) 저장 및 JSON 갱신 자동 트리거
            ExportMapData(selection.name, minBound, maxBound, cellSize);
            successCount++;
        }

        // 원래 씬으로 복귀
        if (!string.IsNullOrEmpty(originalScenePath) && File.Exists(originalScenePath))
        {
            EditorSceneManager.OpenScene(originalScenePath, OpenSceneMode.Single);
        }

        Debug.Log($"[Batch 완료] 선택된 {successCount}개 씬의 맵 데이터와 NavMesh 데이터 내보내기가 성공적으로 끝났습니다.");
    }

    public static void ExportMapData(string name, Vector3 min, Vector3 max, float size)
    {
        string mapDataDir = Application.dataPath + "/../MapData/";
        if (!Directory.Exists(mapDataDir)) Directory.CreateDirectory(mapDataDir);

        string path = Path.Combine(mapDataDir, $"{name}.txt");
        StringBuilder sb = new StringBuilder();

        int xCount = Mathf.CeilToInt((max.x - min.x) / size);
        int zCount = Mathf.CeilToInt((max.z - min.z) / size);

        sb.AppendLine($"{xCount} {zCount}");
        sb.AppendLine($"{min.x} {min.z} {size}");

        int obstacleMask = 1 << LayerMask.NameToLayer("Obstacle");
        int groundMask = 1 << LayerMask.NameToLayer("Ground");

        for (int z = 0; z < zCount; z++)
        {
            for (int x = 0; x < xCount; x++)
            {
                float posX = min.x + (x * size);
                float posZ = min.z + (z * size);

                Vector3 rayStart = new Vector3(posX, 100.0f, posZ);
                RaycastHit hit;

                if (Physics.Raycast(rayStart, Vector3.down, out hit, 200.0f, obstacleMask))
                {
                    sb.Append($"1|{hit.point.y:F2} ");
                }
                else if (Physics.Raycast(rayStart, Vector3.down, out hit, 200.0f, groundMask))
                {
                    sb.Append($"0|{hit.point.y:F2} ");
                }
                else
                {
                    sb.Append($"1|-100.0 ");
                }
            }
            sb.AppendLine();
        }

        File.WriteAllText(path, sb.ToString());
        Debug.Log($"Map Export Complete: {path}");

        // 자동으로 JSON에 크기 정보 및 ID 동기화
        MapDataJsonUpdater.UpdateMapData(name, min.x, min.z, size, xCount, zCount);
    }
}
