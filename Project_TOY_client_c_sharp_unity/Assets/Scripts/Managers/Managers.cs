using UnityEngine;

public class Managers : MonoBehaviour
{
    // 단 하나만 존재하게 만드는 싱글톤
    private static Managers _instance;
    public static Managers Instance { get { Init(); return _instance; } }

    #region Core (네트워크, 리소스 등 게임 인프라)
    private NetworkManager _networkManager = new NetworkManager();
    private PacketManager _packetManager = new PacketManager();

    public static NetworkManager networkManager => Instance._networkManager;
    public static PacketManager packetManager => Instance._packetManager;
    #endregion

    #region Contents (오브젝트, UI, 씬 등 실제 게임 데이터)
    // 나중에 ObjectManager, UIManager 등이 여기 추가됩니다.
    // private ObjectManager _object = new ObjectManager();
    // public static ObjectManager Object => Instance._object;
    #endregion

    void Start()
    {
        Init();
    }

    void Update()
    {
        // 네트워크 수신 패킷 처리는 매 프레임마다 메인 스레드에서 이루어져야 합니다.
        _networkManager.Update();
    }

    static void Init()
    {
        if (_instance == null)
        {
            // 게임 내에 @Managers라는 이름의 오브젝트가 있는지 확인
            GameObject go = GameObject.Find("@Managers");
            if (go == null)
            {
                // 없으면 새로 생성
                go = new GameObject { name = "@Managers" };
                go.AddComponent<Managers>();
            }

            // 씬이 바뀌어도 파괴되지 않게 보호
            DontDestroyOnLoad(go);
            _instance = go.GetComponent<Managers>();


            //초기화가 필요한 Manger들 초기화로직 실행
            _instance._networkManager.Init();
        }

        
    }
}