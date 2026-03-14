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

    protected override void Init()
    {
        base.Init();
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
        // 1. 입력 확인 (키보드 WASD 등)
        float h = Input.GetAxisRaw("Horizontal");
        float v = Input.GetAxisRaw("Vertical");

        Vector3 moveDir = new Vector3(h, 0, v).normalized;

        // 2. 상태 결정 및 이동
        if (moveDir.magnitude > 0.1f)
        {
            State = CreatureState.Moving;
            // 실제 이동 (속도 5.0f 적용)
            transform.position += moveDir * Time.deltaTime * 5.0f;

            // 부드러운 회전 (LookRotation)
            Quaternion lookRotation = Quaternion.LookRotation(moveDir);
            transform.rotation = Quaternion.Slerp(transform.rotation, lookRotation, 0.2f);
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

        // 3. 위치가 거의 변하지 않았으면 보내지 않음 (최적화)
        if (Vector3.Distance(_lastSentPos, transform.position) < 0.01f)
            return;

        // 4. 패킷 생성 및 전송
        CS_MOVING movePkt = new CS_MOVING();
        movePkt.PosInfo = new PosInfo
        {
            X = transform.position.x,
            Y = transform.position.y,
            Z = transform.position.z,
            Yaw = transform.eulerAngles.y // 바라보는 방향
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
    }
}

