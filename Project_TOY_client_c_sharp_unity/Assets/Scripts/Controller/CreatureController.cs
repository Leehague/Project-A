using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class CreatureController : MonoBehaviour
{
    public enum CreatureState
    {
        Idle,
        Moving,
        Skill,
        Dead,
    }

    [SerializeField]
    protected CreatureState _state = CreatureState.Idle;
    // 상태 변경 시 애니메이션 처리를 위해 가상 함수로 만듦
    public virtual CreatureState State
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
            case CreatureState.Idle:
                _animator.CrossFade("IDLE", 0.1f);
                break;
            case CreatureState.Moving:
                _animator.CrossFade("WALK", 0.1f);
                break;
            case CreatureState.Skill:
                _animator.CrossFade("ATTACK", 0.1f);
                break;
            case CreatureState.Dead:
                _animator.CrossFade("Charge", 0.1f);
                break;
        }

        

    }
}
