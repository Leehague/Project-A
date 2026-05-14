using System;
using System.Collections.Generic;

[Serializable]
public class Skill
{
    // 공통 필드 (JSON 키값과 정확히 일치시켜야 함)
    public int id;
    public string name;
    public int skillTypeId;
    public float coolTime;
    public int CostTypeId;
    public float cost;
    public string animName;

    // 상세 필드 (JSON에 있는 모든 가능성 추가)
    public int damage;
    public float range;

    // Projectile 전용
    public float projectileSpeed;
    public int projectileId;

    // Dash 전용
    public float dashDistance;
    public float dashSpeed;
}

[Serializable]
public class SkillData : ILoader<int, Skill>
{
    public List<Skill> skills = new List<Skill>();

    public Dictionary<int, Skill> MakeDict()
    {
        Dictionary<int, Skill> dict = new Dictionary<int, Skill>();
        foreach (Skill skill in skills)
            dict.Add(skill.id, skill);
        return dict;
    }
}

public enum skillType 
{
    Common =0,
    Melee =1,
    Projectile =2,
    Dash =3
}

public enum CostType
{
    None =0,
    Mana =1,
    Hp =2,
}