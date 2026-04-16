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

    public Stat stat { get; set; }

    
    public bool IsMyPlayer { get; set; }


    float _lastSendTime = 0f;
    const float SEND_INTERVAL = 0.05f; // 20Hz (초당 20번 전송)
    Vector3 _lastSentPos;

    // 타인일 때 사용할 변수들
    Vector3 _targetPos;
    float _targetYaw;

    //카메라 제어 및 이동 관련 변수들
    Transform _cameraTransform;
    [SerializeField] float _moveSpeed =     0;
    [SerializeField] float _rotationSpeed = 10.0f;

    private CharacterController _charController;

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

            _cameraTransform = Camera.main.transform;

            // 마우스 커서 고정
            Cursor.lockState = CursorLockMode.Locked;
            Cursor.visible = false;

            _charController = GetComponent<CharacterController>();
            _moveSpeed = stat.speed;

            // UI 생성 및 데이터 연결
            UI_HPMPBar hpBar = Managers.uiManager.ShowSceneUI<UI_HPMPBar>();
            hpBar.SetStat(this.stat, "내 캐릭터");
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
        float h = Input.GetAxisRaw("Horizontal");
        float v = Input.GetAxisRaw("Vertical");
        Vector3 inputDir = new Vector3(h, 0, v).normalized;


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
            Vector3 moveDir = (forward * inputDir.z + right * inputDir.x).normalized;

            // 2. 실제 이동 및 회전
            //transform.position += moveDir * _moveSpeed * Time.deltaTime;
            //[수정] 직접 position을 건드리지 않고 Move 함수 사용
            Vector3 motion = moveDir * _moveSpeed * Time.deltaTime;
            _charController.Move(motion + Vector3.down * 1.0f * Time.deltaTime);


            //회전 로직
            Quaternion targetRotation = Quaternion.LookRotation(moveDir);
            transform.rotation = Quaternion.Slerp(transform.rotation, targetRotation, _rotationSpeed * Time.deltaTime);

            State = Define.CreatureState.Moving;
        }
        else 
        { 
            State = Define.CreatureState.Idle;
        }



         UpdateSkillInput();
        
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

    // 핸들러에서 호출할 데이터 갱신 함수
    public void RefreshPos(PosInfo info)
    {
        _targetPos = new Vector3(info.X, info.Y, info.Z);
        _targetYaw = info.Yaw;

        float distance = (_targetPos - transform.position).magnitude;

        State = (Define.CreatureState)info.State;
    }

    public void SyncPos(Vector3 pos)
    {
        // 보간용 목적지 좌표를 현재 좌표로 강제 일치시킴
        _targetPos = pos;
        transform.position = pos;
        _lastSentPos = pos; // 내 캐릭터일 경우 패킷 중복 전송 방지
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
            SendSkillPacket(201, targetPos);
            
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
}

