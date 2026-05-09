using System;
using System.Collections.Generic;
using UnityEngine;

// 1. 개별 데이터 단위 (예: 캐릭터 정보)
[Serializable]
public class Stat
{
    public Stat(Stat other) 
    { 
        this.id = other.id;
        this.name = other.name;
        this.MaxHp = other.MaxHp;
        this.MaxMp = other.MaxMp;

        this.attack = other.attack;
        this.speed = other.speed;
        this.modelPath = other.modelPath;
    }

    public void Init() 
    {
        hp = MaxHp;
        mp = MaxMp;

    }
    
    public int id;

    public string name;
    public int MaxHp;
    public int MaxMp;
    

    public int attack;
    public int speed;
    public string modelPath;

    public System.Action<int> OnHpChanged;
    public System.Action<int> OnMpChanged;

    private int _hp;
    public int hp
    {
        get => _hp;
        set 
        {
            _hp = value;
            OnHpChanged?.Invoke(_hp); // 체력이 바뀌면 UI에 알림
        }
    }

    private int _mp;
    public int mp
    {
        get => _mp;
        set
        {
            _mp = value;
            OnMpChanged?.Invoke(_mp); // 마나가 바뀌면 UI에 알림
        }
    }
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
            dict.Add(stat.id , stat);
        return dict;
    }
}

// 공통 인터페이스 (여러 종류의 데이터를 관리하기 위함)
public interface ILoader<Key, Value>
{
    Dictionary<Key, Value> MakeDict();
}