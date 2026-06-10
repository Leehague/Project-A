using Protocol;
using TMPro;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;

public class UI_InventoryItem : MonoBehaviour, IPointerEnterHandler, IPointerExitHandler
{
    private string description;
    private int Count = 0;
    private string memo;

    public Image itemIcon;
    public TextMeshProUGUI countText;
    public TextMeshProUGUI descriptionText; 

    

    public void SetInfo(ItemInfo info)
    {
        // info.TemplateId 를 이용하여 DataManager에서 정적 아이템 데이터를 찾습니다.
        if (Managers.dataManager.ItemDict.TryGetValue(info.TemplateId, out Item item))
        {
            // ResourceManager를 통해 iconPath 경로의 Sprite를 로드하여 할당합니다.
            Sprite icon = Managers.resourceManager.Load<Sprite>(item.iconPath);
            if (icon != null)
            {
                //temp
                Debug.Log("icon load");


                itemIcon.sprite = icon;
            }
        }

        // 아이템 개수 및 메모 세팅
        Count = info.Count;
        countText.text = Count.ToString();
        memo = info.ItemMemo;
        
        Setdescription(); // description 문자열 미리 세팅

        // 초기 상태에서는 설명 텍스트 숨기기
        if (descriptionText != null)
        {
            descriptionText.gameObject.SetActive(false);
        }
    }

    public void Setdescription()
    {
        description = $"Count: {Count} \n memo: {memo}";
    }

    public string Getdescription()
    {
        return description;
    }

    // 마우스 포인터가 아이템(슬롯) 위로 들어왔을 때 호출됨
    public void OnPointerEnter(PointerEventData eventData)
    {
        if (descriptionText != null)
        {
            descriptionText.text = description;
            descriptionText.gameObject.SetActive(true); // 텍스트 표시
        }
    }

    // 마우스 포인터가 아이템(슬롯) 밖으로 나갔을 때 호출됨
    public void OnPointerExit(PointerEventData eventData)
    {
        if (descriptionText != null)
        {
            descriptionText.gameObject.SetActive(false); // 텍스트 숨기기
        }
    }
}
