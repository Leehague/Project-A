using Protocol;
using UnityEngine;
using UnityEngine.UI;
using TMPro;

public class UI_InventoryItem : MonoBehaviour
{
    public Image itemIcon;
    public TextMeshProUGUI countText;
    public TextMeshProUGUI memoText; // DB에 추가한 ItemMemo 표시용

    public void SetInfo(ItemInfo info)
    {
        // TODO: info.TemplateId 를 이용하여 DataManager에서 아이템 아이콘(Sprite) 경로를 찾아와 교체
        // itemIcon.sprite = Managers.resourceManager.Load<Sprite>(...);

        // 아이템 개수 및 메모 세팅
        countText.text = info.Count.ToString();
        memoText.text = info.ItemMemo;
    }
}
