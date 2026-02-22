using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using UnityEngine;


public class PlayerController : CreatureController
{
    protected override void Init()
    {
        base.Init();
    }

    protected override void UpdateController()
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
}

