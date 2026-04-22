using Protocol;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class CreatureController : MonoBehaviour
{
    public Stat stat { get; set; }

    // 타인일 때 사용할 변수들
    protected Vector3 _targetPos;
    protected float _targetYaw;

    [SerializeField]
    protected Define.CreatureState _state = Define.CreatureState.Idle;
    // 상태 변경 시 애니메이션 처리를 위해 가상 함수로 만듦
    public virtual Define.CreatureState State
    {
        get { return _state; }
        set
        {
            if (_state == value) return;
            _state = value;
            UpdateAnimation();
        }
    }

    protected Animator _animator;

    protected virtual void Init()
    {
        _animator = GetComponent<Animator>();
    }

    void Start()
    {
        Init();
    }

    void Update()
    {
        UpdateController();
    }

    // 자식들(Player, Monster)이 각자 상황에 맞게 오버라이딩
    protected virtual void UpdateController() { }

    // 상태에 따른 애니메이션 파라미터 조절
    protected virtual void UpdateAnimation()
    {
        if (_animator == null) return;

        switch (_state) 
        {
            case Define.CreatureState.Idle:
                _animator.CrossFade("IDLE", 0.1f);
                break;
            case Define.CreatureState.Moving:
                _animator.CrossFade("WALK", 0.1f);
                break;
            case Define.CreatureState.Skill:
                _animator.CrossFade("ATTACK", 0.1f);
                break;
            case Define.CreatureState.Dead:
                _animator.CrossFade("Charge", 0.1f);
                break;
        }

        

    }

    // 애니메이션 이벤트에서 호출될 함수
    public void OnAttackEnded()
    {
        // 공격 동작이 끝났으므로 다시 Idle 상태로 돌려줌
        State = Define.CreatureState.Idle;

        
    }

    // 핸들러에서 호출할 데이터 갱신 함수
    public void RefreshPos(PosInfo info)
    {
        _targetPos = new Vector3(info.X, info.Y, info.Z);
        _targetYaw = info.Yaw;

        float distance = (_targetPos - transform.position).magnitude;

        State = (Define.CreatureState)info.State;
    }
    public virtual void SyncPos(Vector3 pos) 
    {
        transform.position = pos;
    }
}
