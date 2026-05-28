using UnityEngine;

public class ProjectileController : MonoBehaviour
{
    public float speed = 20.0f; // 서버의 투사체 속도와 동일하게 맞추는 것이 좋습니다. [TODO] 데이터시트 (정적 데이터)에 추가 
    public float maxLifeTime = 5.0f;
    
    private Vector3 _direction;
    private float _lifeTimer = 0f;
    
    // 투사체 초기화 함수
    public void Init(Vector3 startPos, Vector3 targetPos)
    {
        transform.position = startPos;
        
        // 방향 계산 (목표 위치 - 시작 위치)
        _direction = (targetPos - startPos).normalized;
        
        // 투사체가 날아가는 방향을 바라보게 회전
        if (_direction != Vector3.zero)
        {
            transform.rotation = Quaternion.LookRotation(_direction);
        }
    }
    
    void Update()
    {
        // 1. 매 프레임 지정된 방향으로 이동 (클라이언트 측 예측 이동)
        transform.position += _direction * speed * Time.deltaTime;
        
        // 2. 수명 체크 (서버가 별도로 파괴 패킷을 안 보내므로 클라이언트가 알아서 정리)
        _lifeTimer += Time.deltaTime;
        if (_lifeTimer >= maxLifeTime)
        {
            Destroy(gameObject);
        }
    }
    
    // (선택 사항) 물리적인 충돌 이펙트 연출
    // Unity의 Collider(Is Trigger 체크)와 Rigidbody를 붙여두면 작동합니다.
    private void OnTriggerEnter(Collider other)
    {
        // 실제 데미지 판정은 서버가 하므로, 클라이언트에서는 시각적인 폭발 이펙트만 생성하고 자신을 파괴합니다.

        // Hun0FX 폭발 이펙트 소환 (Resources/Prefabs/Effect/FX_Fire_03.prefab )
        //GameObject explosion = Managers.resourceManager.Instantiate("Effect/FX_Fire_03");
        //if (explosion != null)
        //{
        //    explosion.transform.position = transform.position;
        //}
        
        Destroy(gameObject);
    }
}