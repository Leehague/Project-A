using System.Collections.Generic;
using UnityEngine;
using UnityEngine.SceneManagement;

public class SceneManagerEx
{
    public BaseScene CurrentScene { get { return GameObject.FindObjectOfType<BaseScene>(); } }

    public System.Action<int> OnEnterGameReceived;

    public int MapId { get; set; }

    public void LoadScene(Define.SceneType type)
    {
        // 1. 기존 씬의 오브젝트들 정리 (Managers.Object.Clear() 등)
        Managers.objectManager.Clear();

        // 2. 유니티 씬 로드
        string name = System.Enum.GetName(typeof(Define.SceneType), type);
        UnityEngine.SceneManagement.SceneManager.LoadScene(name);
    }
    public AsyncOperation LoadSceneAsync(Define.SceneType type , int id) 
    {
        Managers.objectManager.Clear();
        SceneStructure sceneStructure= Managers.dataManager.SceneStructureDict[id];
        string typename = sceneStructure.name;

        
        return UnityEngine.SceneManagement.SceneManager.LoadSceneAsync(typename);
    }
    
    private SectorTracker _tracker;
    
    private string _baseMapName = ""; // 맵 씬의 접두사
    private int _visibleRadius = 1;      // 주변 몇 칸까지 로드할지 (1이면 3x3 섹터)

    // 현재 로드 완료되었거나 로딩 중인 씬들을 추적하는 캐시 (씬 이름 기준)
    private HashSet<Vector2Int> _loadedSectors = new HashSet<Vector2Int>();

    // 현재 로딩 비동기 작업을 관리하여 중복 로딩을 방지하기 위함
    private Dictionary<Vector2Int, AsyncOperation> _loadingOperations = new Dictionary<Vector2Int, AsyncOperation>();

    //GameScene 의 Init에서 호출 , 즉 새로운 게임 씬을 구성할때 마다 호출
    public void Init(SectorTracker tracker)
    {
        if (tracker == null)
        {
            Debug.LogError("[SceneManagerEx] 주입된 SectorTracker가 null입니다.");
            return;
        }

        _baseMapName = Managers.dataManager.SceneStructureDict[MapId].name;


        // 기존에 혹시 연결되어 있던 이벤트가 있다면 해제 (안전 장치)
        if (_tracker != null)
        {
            _tracker.OnSectorChanged -= OnPlayerSectorChanged;
        }

        _tracker = tracker;
        _tracker.OnSectorChanged += OnPlayerSectorChanged;
        // 씬 로드 시 최초 1회 주변 씬 강제 로드
        UpdateNeighborSectors(_tracker.CurrentSector);
    }

    // 씬 전환 등으로 인해 정리할 때 호출할 클린업 함수
    public void Clear()
    {
        if (_tracker != null)
        {
            _tracker.OnSectorChanged -= OnPlayerSectorChanged;
            _tracker = null;
        }

        _loadedSectors.Clear();
        _loadingOperations.Clear();

        // 기존 씬 오브젝트 클리어
        Managers.objectManager.Clear();
    }

    public void OnDestroy()
    {
        if (_tracker != null)
        {
            _tracker.OnSectorChanged -= OnPlayerSectorChanged;
        }
    }


    private void OnPlayerSectorChanged(Vector2Int oldSector, Vector2Int newSector)
    {
        UpdateNeighborSectors(newSector);
    }
    private void UpdateNeighborSectors(Vector2Int centerSector)
    {
        HashSet<Vector2Int> targetSectors = new HashSet<Vector2Int>();
        // 플레이어 기준 활성화되어야 하는 3x3 섹터 목록 계산
        for (int x = -_visibleRadius; x <= _visibleRadius; x++)
        {
            for (int y = -_visibleRadius; y <= _visibleRadius; y++)
            {
                targetSectors.Add(new Vector2Int(centerSector.x + x, centerSector.y + y));
            }
        }
        // 1. Unload 대상: 현재 로드되어 있으나, 새 목표 영역에 포함되지 않는 섹터
        List<Vector2Int> sectorsToUnload = new List<Vector2Int>();
        foreach (var sector in _loadedSectors)
        {
            if (!targetSectors.Contains(sector))
            {
                sectorsToUnload.Add(sector);
            }
        }
        foreach (var sector in sectorsToUnload)
        {
            UnloadSubScene(sector);
        }
        // 2. Load 대상: 새 목표 영역에 포함되어 있으나, 아직 로드되지 않은 섹터
        foreach (var sector in targetSectors)
        {
            if (!_loadedSectors.Contains(sector) && !_loadingOperations.ContainsKey(sector))
            {
                LoadSubScene(sector);
            }
        }
    }
    private void LoadSubScene(Vector2Int sector)
    {
        string sceneName = $"{_baseMapName}_{sector.x}_{sector.y}";

        Debug.Log($"LoadSubScene: {sceneName}");
        // 실제 빌드 설정(Build Settings)에 씬이 등록되어 있어야 로드 가능합니다.
        // 또는 Addressables를 사용한다면 Addressables.LoadSceneAsync를 사용할 수 있습니다.
        try
        {
            // 중복 실행 방지를 위해 operation 등록
            AsyncOperation op = SceneManager.LoadSceneAsync(sceneName, LoadSceneMode.Additive);
            if (op != null)
            {
                _loadingOperations[sector] = op;
                op.completed += (operation) =>
                {
                    _loadingOperations.Remove(sector);
                    _loadedSectors.Add(sector);
                    Debug.Log($"[SubSceneManager] Load Completed: {sceneName}");
                };
            }
        }
        catch (System.Exception e)
        {
            Debug.LogError($"[SubSceneManager] Failed to load scene {sceneName}: {e.Message}");
        }
    }
    private void UnloadSubScene(Vector2Int sector)
    {
        string sceneName = $"{_baseMapName}_{sector.x}_{sector.y}";
        if (_loadedSectors.Contains(sector))
        {
            AsyncOperation op = SceneManager.UnloadSceneAsync(sceneName);
            if (op != null)
            {
                op.completed += (operation) =>
                {
                    _loadedSectors.Remove(sector);
                    Debug.Log($"[SubSceneManager] Unload Completed: {sceneName}");
                };
            }
        }
    }


}
