using UnityEngine;
using System.Collections.Generic;

public class DataManager
{
    // 외부에서 Managers.Data.StatDict[1] 로 바로 접근 가능
    public Dictionary<int, Stat> StatDict { get; private set; } = new Dictionary<int, Stat>();
    public Dictionary<int, Skill> SkillDict { get; private set; } = new Dictionary<int, Skill>();

    public void Init()
    {
        // 1. JSON 로드 (Resources/Data/StatData.json)
        StatDict = LoadJson<StatData, int, Stat>("StatData").MakeDict();

        SkillDict = LoadJson<SkillData, int, Skill>("SkillData").MakeDict();
    }

    // 제네릭을 이용해 어떤 타입의 데이터든 로드하는 범용 함수
    Loader LoadJson<Loader, Key, Value>(string path) where Loader : ILoader<Key, Value>
    {
        TextAsset textAsset = Managers.resourceManager.Load<TextAsset>($"Data/{path}");
        return JsonUtility.FromJson<Loader>(textAsset.text);
    }
}