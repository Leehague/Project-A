using Cinemachine;
using Protocol;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using UnityEngine;


public class PlayerController : CreatureController
{
    

    public bool IsMyPlayer { get; set; }

    [SerializeField] int MAXjumpcount = 1;
    private int jumpcount = 0;

    float _lastSendTime = 0f;
    const float SEND_INTERVAL = 0.05f; // 20Hz (초당 20번 전송)
    Vector3 _lastSentPos;

    

    //카메라 제어 및 이동 관련 변수들
    Transform _cameraTransform;
    [SerializeField] float _moveSpeed =     0;
    [SerializeField] float _rotationSpeed = 10.0f;

    private CharacterController _charController;
    bool _isStuckByServer = false;
    float _stuckTimer = 0f;


    [Header("Physics")]
    [SerializeField] float _gravity = -20.0f;     // 중력 가속도 (음수값)
    [SerializeField] float _jumpForce = 8.0f;     // 점프 초기 수직 속도
    float _yVelocity = 0.0f;                      // 실시간 Y축 수직 속도



    private float _jumpStateTimer = 0f;
    private float _landingStateTimer = 0f;
    const float JUMP_STATE_DURATION = 0.15f;    // 도약 애니메이션이 최소한 보여야 하는 시간
    const float LANDING_STATE_DURATION = 0.2f;  // 착지 애니메이션이 고정


    protected override void Init()
    {
        base.Init();
        if (IsMyPlayer)
        {
            // 씬에 배치된 Virtual Camera를 찾습니다.
            var vcam = GameObject.FindObjectOfType<CinemachineFreeLook>();
            if (vcam != null)
            {
                vcam.Follow = this.transform;
                vcam.LookAt = this.transform;

            }
            else
            {
                Managers.resourceManager.Instantiate("PlayerFollowCamera");
            }



             _cameraTransform = Camera.main.transform;

            if (_cameraTransform != null)
            {
                CinemachineBrain brain = _cameraTransform.GetComponent<CinemachineBrain>();

                if (brain == null)
                {
                    brain = _cameraTransform.gameObject.AddComponent<CinemachineBrain>();
                }
            }

            // 마우스 커서 고정
            Cursor.lockState = CursorLockMode.Locked;
            Cursor.visible = false;

            _charController = GetComponent<CharacterController>();
            _moveSpeed = stat.speed;

            // UI 생성 및 데이터 연결
            UI_HPMPBar hpBar = Managers.uiManager.ShowSceneUI<UI_HPMPBar>();
            hpBar.SetStat(this.stat, "내 캐릭터");

            // 채팅 UI 동적 생성 (UI_Root 자식으로 자동 할당됨)
            Managers.uiManager.ShowPopupUI<UI_Chat>();
        }
    }

    protected override void UpdateController()
    {
        if (IsMyPlayer)
        {
            HandleInput();
            CheckAndSendMovePacket();
        }
        else
        {
            InterpolateMovement();
        }
    }

    private void HandleInput()
    {
        

        if (_isStuckByServer)
        {
            _stuckTimer -= Time.deltaTime;
            if (_stuckTimer <= 0) _isStuckByServer = false;
            return; // 서버가 보정한 직후에는 키 입력을 무시함
        }

        // UI 입력 필드(채팅창)에 포커스가 맞춰져 있다면 모든 조작 입력을 무시합니다.
        if (UnityEngine.EventSystems.EventSystem.current != null && 
            UnityEngine.EventSystems.EventSystem.current.currentSelectedGameObject != null)
        {
            TMPro.TMP_InputField inputField = UnityEngine.EventSystems.EventSystem.current.currentSelectedGameObject.GetComponent<TMPro.TMP_InputField>();
            if (inputField != null && inputField.isFocused)
            {
                State = Define.CreatureState.Idle; // 이동 중이었다면 즉시 대기 상태로 멈추도록 처리
                return; 
            }
        }

        
        // 지면에 닿아있을 때는 중력이 누적되지 않도록 막고, 경사면 밀착을 위해 최소한의 하향력(-0.5f)만 줍니다.
        if (_charController.isGrounded)
        {
            
            _yVelocity = -0.5f;
            if (State == Define.CreatureState.Fall) { State = Define.CreatureState.Landing; }
            else if (State == Define.CreatureState.Landing) { State = Define.CreatureState.Idle;  jumpcount = 0; }

            //jumpcount = 0;
        }
        else
        {
            // 공중에 떠 있는 경우 중력 가속도를 지속해서 누적
            _yVelocity += _gravity * Time.deltaTime;
            if (_yVelocity < -1.0f && State == Define.CreatureState.Jump) { State = Define.CreatureState.Fall;  } 
            
        }


        // 지면에 닿아있는 상태에서 스페이스바 입력 시 수직 속도를 추진력만큼 솟구치게 변경
        if (Input.GetKeyDown(KeyCode.Space) && _charController.isGrounded && jumpcount < MAXjumpcount )
        {
            State = Define.CreatureState.Jump;
            _yVelocity = _jumpForce;
            jumpcount++;
            
        }


        //수평 입력 처리 (이동 방향 계산)
        float h = Input.GetAxisRaw("Horizontal");
        float v = Input.GetAxisRaw("Vertical");

        Vector3 inputDir = new Vector3(h, 0, v).normalized;
        Vector3 moveDir = Vector3.zero;

        if (inputDir.magnitude > 0.01f)
        {
            // 1. 카메라가 바라보는 방향을 기준으로 이동 방향 계산
            Vector3 forward = _cameraTransform.forward;
            Vector3 right = _cameraTransform.right;

            forward.y = 0;
            right.y = 0;
            forward.Normalize();
            right.Normalize();

            // 최종 이동 방향: (카메라앞 * 세로입력) + (카메라옆 * 가로입력)
            moveDir = (forward * inputDir.z + right * inputDir.x).normalized;

            //회전 로직
            Quaternion targetRotation = Quaternion.LookRotation(moveDir);
            transform.rotation = Quaternion.Slerp(transform.rotation, targetRotation, _rotationSpeed * Time.deltaTime);

            if (_charController.isGrounded && jumpcount < 1)
            {
                State = Define.CreatureState.Moving;
            }

        }
        else
        {
            if (_charController.isGrounded && jumpcount <1)
            {
                State = Define.CreatureState.Idle;
            }
        }





        //이동로직
        //직접 position을 건드리지 않고 Move 함수 사용
        Vector3 velocity = moveDir * _moveSpeed;
        velocity.y = _yVelocity;
        _charController.Move(velocity * Time.deltaTime);

        // [추가] 지면 위에 있는 상태에서만 NavMesh 영역 밖으로 나가는 것을 방지
        // (점프/낙하 중에 체크하면 공중에 뜬 캐릭터가 바닥으로 갑자기 강제 착지(Snapping)될 수 있습니다)
        if (_charController.isGrounded && State == Define.CreatureState.Moving)
        {
            UnityEngine.AI.NavMeshHit hit;
            // 캐릭터 중심 기준 1.0f 반경 내의 가장 가까운 유효한 NavMesh 지점을 찾습니다.
            if (UnityEngine.AI.NavMesh.SamplePosition(transform.position, out hit, 1.0f, UnityEngine.AI.NavMesh.AllAreas))
            {
                // 현재 캐릭터의 실제 위치와 NavMesh 위 유효한 위치가 아주 미세하게라도 다르다면 보정
                if (Vector3.Distance(transform.position, hit.position) > 0.01f)
                {
                    // CharacterController를 잠시 끄고 위치를 보정해야 정상 적용됩니다.
                    _charController.enabled = false;
                    transform.position = new Vector3(hit.position.x, transform.position.y, hit.position.z); // XZ축만 보정
                    _charController.enabled = true;
                }
            }
        }

        UpdateSkillInput();
        HandleUIInput();
        
    }

    private void CheckAndSendMovePacket() 
    {
        if (Time.time - _lastSendTime < SEND_INTERVAL)
            return;

        // 거리 차이 계산
        float distance = Vector3.Distance(_lastSentPos, transform.position);

        // 핵심 로직: 
        // 1. 거리가 멀거나 (이동 중)
        // 2. 거리는 가깝지만, 현재 내 상태가 '이동'인데 멈추려고 할 때 (상태 변화 시점)
        // 이 두 경우에만 패킷을 보냅니다.

        bool isMoving = distance > 0.01f;

        // 만약 움직이지도 않는데, 이전 전송 상태도 Idle이었다면 보낼 필요 없음 (최적화 유지)
        if (!isMoving && State == Define.CreatureState.Idle)
            return;

        // 4. 패킷 생성 및 전송
        CS_MOVING movePkt = new CS_MOVING();
        movePkt.PosInfo = new PosInfo
        {
            X = transform.position.x,
            Y = transform.position.y,
            Z = transform.position.z,
            Yaw = transform.eulerAngles.y, // 바라보는 방향
            ObjectId = Managers.objectManager.Myplayer_playerId,
            State = (int)State
        };

        // 전송 (네트워크 매니저 혹은 세션을 통해)
        Managers.networkManager.Send(movePkt);

        _lastSendTime = Time.time;
        _lastSentPos = transform.position;
    }

    void InterpolateMovement()
    {
        // 타인 캐릭터가 뚝뚝 끊기지 않게 Lerp 처리
        transform.position = Vector3.Lerp(transform.position, _targetPos, Time.deltaTime * 10f);

        Quaternion targetRot = Quaternion.Euler(0, _targetYaw, 0);
        transform.rotation = Quaternion.Slerp(transform.rotation, targetRot, Time.deltaTime * 10f);
    }

    

    public override void SyncPos(Vector3 pos)
    {
        if (_charController != null) _charController.enabled = false;
        // 보간용 목적지 좌표를 현재 좌표로 강제 일치시킴
        _targetPos = pos;
        transform.position = pos;
        _lastSentPos = pos; // 내 캐릭터일 경우 패킷 중복 전송 방지
        if (_charController != null) _charController.enabled = true;

        // [추가] 서버가 위치를 강제로 되돌렸다면, 0.1초간 입력을 무시하여 
        // 벽에 부딪혀 멈춘 듯한 마찰력을 시뮬레이션합니다.
        _isStuckByServer = true;
        _stuckTimer = 0.1f;
    }

    // PlayerController.cs 내부 HandleInput 메서드 등에 추가
    void UpdateSkillInput()
    {
        if (!IsMyPlayer) return;

        // 예시: 숫자 1키를 누르면 '일반 공격(101)' 사용
        if (Input.GetKeyDown(KeyCode.Alpha1))
        {
            SendSkillPacket(101); // Melee (타겟팅 혹은 즉발)
            
        }

        // 예시: 숫자 2키를 누르면 '화염구(201)' 사용
        if (Input.GetKeyDown(KeyCode.Alpha2))
        {
            
            // 논타겟팅: 현재 마우스 위치나 캐릭터 정면 좌표를 전송
            Vector3 targetPos = transform.position + transform.forward * 10.0f;
            SendSkillPacket(201,targetPos);
            
        }

        //Dash (301)사용
        if (Input.GetKeyDown(KeyCode.Alpha3))
        {
            Vector3 targetPos = transform.position + transform.forward * 10.0f;
            SendSkillPacket(301, targetPos);
        }
    }

    void SendSkillPacket(int skillId, Vector3? targetPos = null)
    {
        CS_SKILL skillPkt = new CS_SKILL();
        skillPkt.SkillId = skillId;

        if (targetPos.HasValue)
        {
            //[cite_start]// 논타겟팅/이동기용 위치 정보 세팅 [cite: 49]
            skillPkt.DestPos = new TargetPosInfo
            {
                X = targetPos.Value.x,
                Y = targetPos.Value.y,
                Z = targetPos.Value.z
            };
        }
        //[cite_start]// TODO: 타겟팅 스킬일 경우 skillPkt.Target 세팅 [cite: 48]

        Managers.networkManager.Send(skillPkt);
    }

    private void HandleUIInput()
    {
        // 인벤토리 UI 토글
        if (Input.GetKeyDown(KeyCode.I))
        {
            UI_Inventory invenUI = Managers.uiManager.FindUI<UI_Inventory>();
            if (invenUI != null && invenUI.isActiveAndEnabled)
            {
                Debug.Log("close invenUI");
                // 이미 열려있으면 닫기
                Managers.uiManager.ClosePopupUI(invenUI);
            }
            else
            {
                Debug.Log("open invenUI");
                // 닫혀있으면 열기
                Managers.uiManager.ShowPopupUI<UI_Inventory>();
            }
        }
    }
}
