using Protocol;
using System.Collections;
using System.Collections.Generic;
using TMPro;
using Unity.VisualScripting;
using UnityEngine;
using UnityEngine.UI;

public class UI_QuestInfo : UI_Popup
{
    enum Texts
    {
        Quest_description,
        Quest_Template_data,
        Quest_Current_state
    }
    //현재는 에디터상에서 설정
    public GameObject Quest_description;
    public GameObject Quest_Template_data;
    public GameObject Quest_Current_state;

    private bool _init = false;
    public int? cash_questid {  get; private set; }
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

    public void RefreshUI(int questid)
    {
        
        cash_questid = questid;

        // 2. 매니저로부터 퀘스트 인스턴스 가져오기
        Quest quest = Managers.questManager.GetQuest(questid);
        if (quest == null)
        {
            Debug.LogError($"UI_QuestInfo: 퀘스트 ID {questid}를 찾을 수 없습니다.");
            return;
        }
        // 3. DataManager로부터 템플릿(기획 데이터) 정보 매핑하기
        QuestTemplate template = null;
        if (Managers.dataManager.QuestTemplateDict.TryGetValue(quest.quest_template_id, out template) == false)
        {
            Debug.LogError($"UI_QuestInfo: 템플릿 ID {quest.quest_template_id}를 DataManager에서 찾을 수 없습니다.");
            return;
        }
        // 4. 각 GameObject로부터 TextMeshProUGUI 컴포넌트 가져오기
        TextMeshProUGUI descText = Quest_description.GetComponent<TextMeshProUGUI>();
        TextMeshProUGUI templateText = Quest_Template_data.GetComponent<TextMeshProUGUI>();
        TextMeshProUGUI stateText = Quest_Current_state.GetComponent<TextMeshProUGUI>();
        // 5. 텍스트 설정
        if (descText != null)
        {
            // 예: 몬스터 사냥 설명 가공
            descText.text = $"[임무] 대상 몬스터 (ID: {template.trargetMonsterId})를 {template.targetCount}마리 사냥하세요.";
        }
        if (templateText != null)
        {
            // 퀘스트 이름 및 보상 정보 출력
            string rewardInfo = "";
            if (template.rewardExp > 0) rewardInfo += $"경험치 {template.rewardExp} ";
            if (template.rewardGold > 0) rewardInfo += $"골드 {template.rewardGold}G ";
            if (template.rewardItemTemplateId > 0) rewardInfo += $"아이템 (ID:{template.rewardItemTemplateId}) {template.rewardItemCount}개 ";
            templateText.text = $"퀘스트 명: {template.name}\n보상: {rewardInfo}";
        }
        if (stateText != null)
        {
            // 퀘스트 진행도 및 한글 상태값 출력
            string stateStr = GetQuestStateKorean(quest.quest_state);
            stateText.text = $"진행 상황: {quest.current_count} / {quest.target_count} ({stateStr})";
        }
    }
    // 퀘스트 상태값 한글 변환 헬퍼 메서드
    private string GetQuestStateKorean(QuestState state)
    {
        switch (state)
        {
            case QuestState.NotAccepted: return "미수락";
            case QuestState.Accepted: return "진행 중";
            case QuestState.Completed: return "완료 가능 (보상 수령 전)";
            case QuestState.Rewardreceived: return "완료됨";
            default: return "알 수 없음";
        }
    }

    public void RefreshUI()
    {
        if (cash_questid == null) return;

        RefreshUI((int)cash_questid);

    }

}
