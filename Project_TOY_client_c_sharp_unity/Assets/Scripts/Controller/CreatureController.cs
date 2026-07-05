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
            case Define.CreatureState.Jump:
                _animator.CrossFade("WALK", 0.1f);
                break;
            case Define.CreatureState.Fall:
                _animator.CrossFade("WALK", 0.1f);
                break;
            case Define.CreatureState.Landing:
                _animator.CrossFade("IDLE", 0.1f);
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
    //위치를 강제로 조정하는 함수
    public virtual void SyncPos(Vector3 pos) 
    {
        _targetPos = pos;

        transform.position = pos;
    }
    //해당 Creautre가 사망시 호출될 함수
    public virtual void OnDead() 
    {
        if(_state == Define.CreatureState.Dead) return; //'살아있는' 상태였을때만 실행되어야 함
        
        
        _state = Define.CreatureState.Dead;

        //풀링 시스템 사용
        Managers.poolingManager.AddcreatureObejct(this);

        //Creatue 종류에 따라 해당하는 사망 로직 실행
        //ex ) 애니매이션, 특정 컨텐츠 실행 등
    }

    //임시 함수, 스킬마다 함수를 만들건 아니지만 일단 테스트용
    public void spawnfireball(Vector3 targetpos) 
    {
        GameObject fireball = Managers.resourceManager.Instantiate("Effect/FX_Fire_03");
        ProjectileController fireballcontroller;
        if (fireball.TryGetComponent<ProjectileController>(out fireballcontroller))
        {
            fireballcontroller.Init(transform.position, targetpos);
        }
    }

    // 대쉬 연출을 시작하는 public 함수
public void PlayDashAnimation(Vector3 targetPos, float duration = 0.15f)
{
    // 이미 다른 대쉬가 진행 중일 수도 있으니 코루틴을 중복 실행하지 않도록 관리할 수도 있습니다.
    // 여기서는 단순하게 바로 코루틴을 실행합니다.
    StartCoroutine(DashRoutine(targetPos, duration));
}

// 실제 부드러운 이동을 처리하는 코루틴
private System.Collections.IEnumerator DashRoutine(Vector3 targetPos, float duration)
{
    Vector3 startPos = transform.position;
    float elapsedTime = 0f;

    // TODO: 대쉬 시작 시 파티클 이펙트나 사운드를 재생하는 코드를 여기에 추가하면 좋습니다.
    // 예: dashEffect.Play();

    while (elapsedTime < duration)
    {
        // 시간에 따라 startPos에서 targetPos로 부드럽게 보간
        transform.position = Vector3.Lerp(startPos, targetPos, elapsedTime / duration);
        elapsedTime += Time.deltaTime;
        
        yield return null; // 다음 프레임까지 대기
    }

    // 루프가 끝난 후 정확한 최종 위치로 보정
    transform.position = targetPos;
    
    // (선택 사항) 대쉬가 끝난 후 상태를 Idle 등으로 되돌릴 수 있습니다.
    // State = Define.CreatureState.Idle;
}

}
