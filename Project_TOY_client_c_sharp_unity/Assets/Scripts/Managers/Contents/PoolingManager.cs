using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class PoolingManager 
{
    Dictionary<int,Pool> pools = new Dictionary<int,Pool>(); //프리팹 Id (templated id) , pool

    //새롭게 풀링 시킬 CreatureObject가 있다면 사용할 함수
    public void AddcreatureObejct(CreatureController cc)
    {
        Pool pool;
        if (!pools.TryGetValue(cc.stat.id, out pool)) 
        {
            //PoolingManager의 내부인자 pools에 해당하는 templateId 를 가지는 creature controller들의 Pool이 없다면 새롭게 만들어서 추가함
            Pool Newpool = new Pool(cc.stat.id);
            pools[cc.stat.id] = Newpool;

            //pool = Newpool;
        }
        pool = pools[cc.stat.id];
        //해당 Creature에 대한 풀링 처리(초기화)
        cc.gameObject.SetActive(false);
        cc.stat.Init();
        //다른 초기화 처리가 미비해서 문제가 생길지도?

        pool.AddCreatureController(cc);
    }
    
    //풀링하고 있는 객체 중에 해당하는 TemplateId의 객체가 있다면 반환
    public bool TryPopcreatureObject(int templateId , out CreatureController cc) 
    {
        if (pools.TryGetValue(templateId, out Pool pool)) 
        {
            cc =pool.PopCreatureController();
            return true;
        }
        else
        {
            cc = null;
            return false;
        }
    }
}
