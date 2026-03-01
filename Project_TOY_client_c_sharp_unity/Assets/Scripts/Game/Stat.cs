using System;
using System.Collections.Generic;

// 1. 개별 데이터 단위 (예: 캐릭터 정보)
[Serializable]
public class Stat
{
    public int id;
    public string name;
    public int hp;
    public int attack;
    public string modelPath;
}

// 2. 리스트 형태의 JSON을 받기 위한 래퍼 클래스
[Serializable]
public class StatData : ILoader<int, Stat>
{
    public List<Stat> stats = new List<Stat>();

    // List를 Dictionary로 변환하는 규격
    public Dictionary<int, Stat> MakeDict()
    {
        Dictionary<int, Stat> dict = new Dictionary<int, Stat>();
        foreach (Stat stat in stats)
            dict.Add(stat.id, stat);
        return dict;
    }
}

// 공통 인터페이스 (여러 종류의 데이터를 관리하기 위함)
public interface ILoader<Key, Value>
{
    Dictionary<Key, Value> MakeDict();
}