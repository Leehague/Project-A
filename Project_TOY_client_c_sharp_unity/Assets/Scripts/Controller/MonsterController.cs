using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class MonsterController : CreatureController
{
    protected override void UpdateController()
    {
        InterpolateMovement();
    }


    void InterpolateMovement()
    {
        //캐릭터가 뚝뚝 끊기지 않게 Lerp 처리
        transform.position = Vector3.Lerp(transform.position, _targetPos, Time.deltaTime * 10f);

        Quaternion targetRot = Quaternion.Euler(0, _targetYaw, 0);
        transform.rotation = Quaternion.Slerp(transform.rotation, targetRot, Time.deltaTime * 10f);
    }
}
