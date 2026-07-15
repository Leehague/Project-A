#if UNITY_EDITOR
using System.Collections.Generic;
using System.IO;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.SceneManagement;

public class MapSceneSplitter : EditorWindow
{
    // JSON 파싱용 직렬화 구조체 정의
    [System.Serializable]
    public class SceneStructure
    {
        public int id;
        public string name;
        public string type;
    }

    [System.Serializable]
    public class SceneStructureData
    {
        public List<SceneStructure> sceneStructures;
    }

    private float _sectorSize = 50f;
    private string _outputFolder = "Assets/Scenes/SplitMap";

    // 드롭다운 관리를 위한 변수들
    private List<SceneStructure> _validScenesInJson = new List<SceneStructure>();
    private List<string> _dropdownNames = new List<string>();
    private int _selectedSceneIndex = 0;

    [MenuItem("Tools/Map/Scene Splitter")]
    public static void ShowWindow()
    {
        GetWindow<MapSceneSplitter>("Scene Splitter");
    }

    private void OnEnable()
    {
        // 툴 창이 열리거나 활성화될 때 JSON 파일 분석 및 씬 탐색 수행
        LoadValidScenesFromJSON();
    }

    /// <summary>
    /// Resources 내의 JSON 데이터를 읽고, 실제 Assets 폴더에 씬 파일이 존재하는 대상만 필터링합니다.
    /// </summary>
    private void LoadValidScenesFromJSON()
    {
        _validScenesInJson.Clear();
        _dropdownNames.Clear();

        // DataManager가 Load하는 실제 JSON 경로
        string jsonPath = "Assets/Resources/Data/SceneStructureData.json";

        if (!File.Exists(jsonPath))
        {
            Debug.LogError($"[SceneSplitter] JSON 파일을 찾을 수 없습니다: {jsonPath}");
            return;
        }

        try
        {
            string jsonText = File.ReadAllText(jsonPath);
            SceneStructureData data = JsonUtility.FromJson<SceneStructureData>(jsonText);

            if (data != null && data.sceneStructures != null)
            {
                foreach (var sceneInfo in data.sceneStructures)
                {
                    // 씬 종류가 'Game' 타입인 경우만 스플릿 대상으로 처리 (Login이나 Loading은 스플릿 제외)
                    if (sceneInfo.type != "Game")
                        continue;

                    // 실제 에셋 폴더 내에 씬 파일(.unity)이 존재하는지 검증
                    string sceneAssetPath = FindSceneAssetPath(sceneInfo.name);
                    if (!string.IsNullOrEmpty(sceneAssetPath))
                    {
                        _validScenesInJson.Add(sceneInfo);
                        _dropdownNames.Add($"{sceneInfo.name} (ID: {sceneInfo.id})");
                    }
                }
            }
        }
        catch (System.Exception e)
        {
            Debug.LogError($"[SceneSplitter] JSON 로드 중 예외 발생: {e.Message}");
        }
    }

    private void OnGUI()
    {
        GUILayout.Label("Scene Splitter (JSON Validated)", EditorStyles.boldLabel);
        EditorGUILayout.Space();

        if (_validScenesInJson.Count == 0)
        {
            EditorGUILayout.HelpBox("JSON에 명세되어 있고 프로젝트 에셋에 존재하는 'Game' 타입 씬이 없습니다.", MessageType.Warning);
            if (GUILayout.Button("Reload JSON & Scenes"))
            {
                LoadValidScenesFromJSON();
            }
            return;
        }

        // 1. JSON 기반 드롭다운 UI 제공
        _selectedSceneIndex = EditorGUILayout.Popup("Target Scene to Split", _selectedSceneIndex, _dropdownNames.ToArray());

        _sectorSize = EditorGUILayout.FloatField("Sector Size (m)", _sectorSize);
        _outputFolder = EditorGUILayout.TextField("Output Folder", _outputFolder);

        EditorGUILayout.Space();

        if (GUILayout.Button("Load & Split Scene"))
        {
            var selectedScene = _validScenesInJson[_selectedSceneIndex];
            string scenePath = FindSceneAssetPath(selectedScene.name);

            if (EditorUtility.DisplayDialog("Scene Splitter",
                $"선택한 씬 '{selectedScene.name}'을 열어 분할하시겠습니까?\n작업 전 마스터 씬의 변경사항은 모두 저장됩니다.", "예", "아니오"))
            {
                ExecuteSceneSplit(selectedScene.name, scenePath);
            }
        }

        if (GUILayout.Button("Refresh Scene List", GUILayout.Width(150)))
        {
            LoadValidScenesFromJSON();
        }
    }

    private void ExecuteSceneSplit(string sceneName, string scenePath)
    {
        // 1. 현재 편집 중인 씬 강제 저장
        EditorSceneManager.SaveOpenScenes();

        // 2. 타겟 마스터 씬을 에디터에 로드
        Scene masterScene = EditorSceneManager.OpenScene(scenePath, OpenSceneMode.Single);

        // 출력 폴더 생성
        if (!Directory.Exists(_outputFolder))
        {
            Directory.CreateDirectory(_outputFolder);
        }

        // 3. 환경 프롭 오브젝트 수집 (Environment 태그 기준)
        GameObject[] envObjects = GameObject.FindGameObjectsWithTag("Environment");

        Dictionary<Vector2Int, List<GameObject>> sectorGroups = new Dictionary<Vector2Int, List<GameObject>>();
        foreach (var go in envObjects)
        {
            Vector2Int sector = GetSectorFromPosition(go.transform.position);
            if (!sectorGroups.ContainsKey(sector))
            {
                sectorGroups[sector] = new List<GameObject>();
            }
            sectorGroups[sector].Add(go);
        }

        List<string> createdScenePaths = new List<string>();

        // 4. 각 섹터별로 서브 씬 분할
        foreach (var pair in sectorGroups)
        {
            Vector2Int sector = pair.Key;
            List<GameObject> objs = pair.Value;

            string subSceneName = $"{sceneName}_{sector.x}_{sector.y}";
            string subScenePath = $"{_outputFolder}/{subSceneName}.unity";

            // 빈 씬 생성 (Additive)
            Scene newScene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Additive);

            foreach (var go in objs)
            {
                if (go == null) continue;

                // 부모 자식 계층 정리 (루트 승격)
                if (go.transform.parent != null)
                {
                    go.transform.SetParent(null);
                }

                SceneManager.MoveGameObjectToScene(go, newScene);
            }

            // 서브 씬 파일 저장 및 닫기
            EditorSceneManager.SaveScene(newScene, subScenePath);
            createdScenePaths.Add(subScenePath);
            EditorSceneManager.CloseScene(newScene, true);
        }

        // 5. 생성된 서브 씬들 빌드 세팅 자동 업데이트
        RegisterScenesInBuildSettings(createdScenePaths);

        // 마스터 씬 변경사항 저장
        EditorSceneManager.SaveScene(masterScene);
        AssetDatabase.Refresh();

        EditorUtility.DisplayDialog("완료", $"'{sceneName}' 씬 분할이 성공적으로 끝났습니다.\n총 {createdScenePaths.Count}개의 서브 씬이 추가되었습니다.", "확인");
    }

    private void RegisterScenesInBuildSettings(List<string> newScenePaths)
    {
        List<EditorBuildSettingsScene> scenes = new List<EditorBuildSettingsScene>(EditorBuildSettings.scenes);

        foreach (var path in newScenePaths)
        {
            bool alreadyExists = scenes.Exists(s => s.path.Replace('\\', '/').Equals(path.Replace('\\', '/')));
            if (!alreadyExists)
            {
                scenes.Add(new EditorBuildSettingsScene(path, true));
            }
        }

        EditorBuildSettings.scenes = scenes.ToArray();
    }

    private string FindSceneAssetPath(string sceneName)
    {
        string[] guids = AssetDatabase.FindAssets($"{sceneName} t:Scene");
        foreach (var guid in guids)
        {
            string path = AssetDatabase.GUIDToAssetPath(guid);
            if (Path.GetFileNameWithoutExtension(path).Equals(sceneName, System.StringComparison.OrdinalIgnoreCase))
            {
                return path;
            }
        }
        return null;
    }

    private Vector2Int GetSectorFromPosition(Vector3 position)
    {
        int x = Mathf.FloorToInt(position.x / _sectorSize);
        int y = Mathf.FloorToInt(position.z / _sectorSize);
        return new Vector2Int(x, y);
    }
}
#endif
