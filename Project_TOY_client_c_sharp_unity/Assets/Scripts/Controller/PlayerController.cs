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

        if (inputDir.magnitude > 0.1f)
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

            State = CreatureState.Moving;
        }
        else
        {
            State = CreatureState.Idle;
        }
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
        if (!isMoving && State == CreatureState.Idle)
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
            State = (ulong)State
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

        State = (CreatureState)info.State;
    }
}

