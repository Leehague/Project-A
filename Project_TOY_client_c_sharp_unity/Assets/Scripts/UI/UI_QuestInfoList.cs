using System;
using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;

public class UI_QuestInfoList : UI_Popup
{
    enum Texts
    {
        Quest_List
    }

    //현재는 에디터상에서 설정
    public GameObject Quest_List;

    private bool _init = false;


    public override void Init()
    {
        if (_init) return;
        _init = true;

        base.Init(); // 부모(UI_Popup)의 Init()을 호출하여 드래그 타겟 설정 및 화면 중앙 배치를 처리합니다.
        // 계층 구조상 맨 아래로 보내 강제로 화면 최상단(맨 앞)에 렌더링되도록 합니다.
        transform.SetAsLastSibling();

    }


    private void Start()
    {
        Init();
    }

    public void RefreshUI()
    {
        TextMeshProUGUI ListText = Quest_List.GetComponent<TextMeshProUGUI>();


        if (ListText != null)
        {
            foreach (var questpair in Managers.questManager.Quests)
            {
                Quest quest = questpair.Value;


                String client_name = "None";

                if (Managers.objectManager.Find(quest.client_object_id) != null)
                {
                    client_name = Managers.objectManager.Find(quest.client_object_id).name;
                }

                String acquirer_name = "None";

                if (Managers.objectManager.Find(quest.acquirer_object_id) != null)
                {
                    acquirer_name = Managers.objectManager.Find(quest.acquirer_object_id).name;
                }


                ListText.text += $"Quest Name : {quest.Name} , client: {client_name} , acquirer: {acquirer_name}\n";
            }
        }

    }
}
