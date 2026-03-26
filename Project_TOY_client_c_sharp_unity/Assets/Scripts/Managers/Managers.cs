using Unity.VisualScripting;
using UnityEngine;

public class Managers : MonoBehaviour
{
    // 단 하나만 존재하게 만드는 싱글톤
    private static Managers _instance;
    public static Managers Instance { get { Init(); return _instance; } }

    #region Core (네트워크, 리소스 등 게임 인프라)
    //내부 생성
    private NetworkManager _networkManager = new NetworkManager();
    private PacketManager _packetManager = new PacketManager();
    private ResourceManager _resourceManager = new ResourceManager();

    //외부 접근용 property
    public static NetworkManager networkManager => Instance._networkManager;
    public static PacketManager packetManager => Instance._packetManager;
    public static ResourceManager resourceManager => Instance._resourceManager;
    #endregion

    #region Contents (오브젝트, UI, 씬 등 실제 게임 데이터)
    //내부 생성
    private ObjectManager _objectManager = new ObjectManager();
    private DataManager _dataManager = new DataManager();
    private SceneManagerEx SceneManagerEx = new SceneManagerEx();


    //외부 접근용 property
    public static ObjectManager objectManager => Instance._objectManager;
    public static DataManager dataManager => Instance._dataManager;
    public static SceneManagerEx sceneManagerEx => Instance.SceneManagerEx;
    #endregion

    void Start()
    {
        Init();
    }

    void Update()
    {
        //instance를 사용한 접근 (타이밍 이슈 방지)
        if (_instance == null) return;

        // 씬 전환 중에는 호출 자체를 하지 않음으로써 메인 스레드 점유율을 0으로 만듭니다.
        if (_instance._networkManager.enabled)
        {
            _instance._networkManager.Update();
        }
    }

    static void Init()
    {
        if (_instance == null)
        {
            // 게임 내에 @Managers라는 이름의 오브젝트가 있는지 확인
            GameObject go = GameObject.Find("@Managers");
            if (go == null)
            {
                // 2. 만약 정말로 없다면 새로 생성
                go = new GameObject { name = "@Managers" };
                _instance = go.AddComponent<Managers>();
                DontDestroyOnLoad(go);

                // 초기화 로직은 여기서 딱 한 번만!
                _instance._networkManager.Init();
                _instance._dataManager.Init();
                Debug.Log("@Managers 신규 생성 및 초기화 완료");
            }
            else
            {
                // 3. 이름은 있는데 _instance가 연결 안 된 경우 (씬 전환 직후 등)
                _instance = go.GetComponent<Managers>();
                if (_instance == null) // 이름만 같고 컴포넌트가 없는 경우 대비
                {
                    _instance = go.AddComponent<Managers>();
                }
            }
        }

        
    }

    void OnApplicationQuit()
    {
        // 소켓을 닫고 세션을 정리하는 함수 호출
        _networkManager.Close();
    }
}