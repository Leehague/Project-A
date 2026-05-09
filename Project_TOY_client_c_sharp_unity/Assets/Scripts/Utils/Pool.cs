using JetBrains.Annotations;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using UnityEngine;

//CreatureController queue를 들고 있는 클래스
public class Pool
{
    public Pool(int templateId) 
    {
        _templatedId = templateId;
    }
    private readonly int _templatedId;
    public int TemplatedId => _templatedId;
    Queue<CreatureController> ccqueue = new();
    public void AddCreatureController(CreatureController cc) 
    {
        ccqueue.Enqueue(cc);
    }
    public CreatureController PopCreatureController() 
    {
        return ccqueue.Dequeue();
    }
}

