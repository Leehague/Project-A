using System;
using System.Collections.Generic;

[Serializable]
public class Item
{
    // 공통 필드
    public int id;
    public string name;
    public int itemTypeId;
    public string description;
    public string iconPath;

    // 장비(Equipment) 전용 필드
    public int damage;
    public string modelPath;

    // 소모품(Consumable) 전용 필드
    public int value;
    public string effect;
    public int coolTime;
    public bool stackable;
    public int maxStack;
}

[Serializable]
public class ItemData : ILoader<int, Item>
{
    public List<Item> items = new List<Item>();

    public Dictionary<int, Item> MakeDict()
    {
        Dictionary<int, Item> dict = new Dictionary<int, Item>();
        foreach (Item item in items)
            dict.Add(item.id, item);
        return dict;
    }
}

public enum ItemType
{
    Equipment = 0,
    Consumable = 1,
    Etc = 2
}