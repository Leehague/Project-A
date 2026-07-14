using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;

public class MapGeneratorWindow : EditorWindow
{
    // 입력받을 변수들
    private GameObject targetPrefab;
    private int gridWidth = 5;
    private int gridHeight = 5;
    private float spacing = 10f;

    // 상단 메뉴바에 툴 등록
    [MenuItem("Tools/Map Generator")]
    public static void ShowWindow()
    {
        // 윈도우 생성 및 타이틀 지정
        GetWindow<MapGeneratorWindow>("Map Generator");
    }

    // 에디터 윈도우의 GUI를 그리는 함수
    private void OnGUI()
    {
        GUILayout.Label("Statically Generate Grid Map", EditorStyles.boldLabel);
        GUILayout.Space(10);

        // 입력 UI 그리기
        targetPrefab = (GameObject)EditorGUILayout.ObjectField("Target Prefab", targetPrefab, typeof(GameObject), false);
        gridWidth = EditorGUILayout.IntField("Grid Width", gridWidth);
        gridHeight = EditorGUILayout.IntField("Grid Height", gridHeight);
        spacing = EditorGUILayout.FloatField("Spacing (m)", spacing);

        GUILayout.Space(20);

        // 생성 버튼
        if (GUILayout.Button("Generate Map", GUILayout.Height(30)))
        {
            GenerateMap();
        }

        // 전체 삭제 버튼 (꾸며주기)
        GUI.backgroundColor = Color.red;
        if (GUILayout.Button("Clear Generated Map", GUILayout.Height(20)))
        {
            ClearMap();
        }
        GUI.backgroundColor = Color.white;
    }

    private void GenerateMap()
    {
        if (targetPrefab == null)
        {
            EditorUtility.DisplayDialog("Error", "배치할 targetPrefab이 지정되지 않았습니다!", "OK");
            return;
        }

        // 부모 오브젝트 탐색 혹은 생성
        GameObject mapRoot = GameObject.Find("Static_Map_Root");
        if (mapRoot == null)
        {
            mapRoot = new GameObject("Static_Map_Root");
        }

        int count = 0;
        float xstart= gridWidth/2; float zstart= gridHeight/2;
        

        for (int x = 0; x < gridWidth; x++)
        {
            for (int z = 0; z < gridHeight; z++)
            {
                Vector3 spawnPos = new Vector3(x * spacing - xstart*spacing, 0.5f, z * spacing - zstart*spacing);

                // 프리팹 링크 유지하며 정적 인스턴스화
                GameObject spawnedObj = (GameObject)PrefabUtility.InstantiatePrefab(targetPrefab);
                if (spawnedObj != null)
                {
                    spawnedObj.transform.position = spawnPos;
                    spawnedObj.transform.SetParent(mapRoot.transform);

                    // 되돌리기(Ctrl+Z) 등록
                    Undo.RegisterCreatedObjectUndo(spawnedObj, "Generate Static Map Object");
                    count++;
                }
            }
        }

        // 변경사항 저장 대기 상태로 지정
        EditorSceneManager.MarkSceneDirty(EditorSceneManager.GetActiveScene());

        // 팝업 알림
        EditorUtility.DisplayDialog("Success", $"{count}개의 오브젝트가 정적으로 배치되었습니다!", "OK");
    }

    private void ClearMap()
    {
        GameObject mapRoot = GameObject.Find("Static_Map_Root");
        if (mapRoot != null)
        {
            // 되돌리기 가능하도록 안전하게 영구 삭제
            Undo.DestroyObjectImmediate(mapRoot);
            EditorSceneManager.MarkSceneDirty(EditorSceneManager.GetActiveScene());
            Debug.Log("[MapGenerator] 배치된 정적 맵이 삭제되었습니다.");
        }
        else
        {
            EditorUtility.DisplayDialog("Notice", "삭제할 Static_Map_Root를 찾을 수 없습니다.", "OK");
        }
    }
}
