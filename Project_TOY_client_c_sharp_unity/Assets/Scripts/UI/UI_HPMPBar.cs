using UnityEngine;
using UnityEngine.UI;

public class UI_HPMPBar : UI_Scene
{
    // 1. 프리팹의 자식 오브젝트 이름과 정확히 일치해야 함
    enum Texts
    {
        PlayerNameText,
    }

    enum Sliders
    {
        HPBar,
        MPBar
    }

    private Stat _stat;

    public override void Init()
    {
        // 이미 초기화가 되었다면 다시 하지 않음 (중요)
        if (_objects.ContainsKey(typeof(Text)))
            return;
        base.Init(); // 캔버스 설정 등 부모 로직 수행

        // 2. 자동 바인딩
        Bind<Text>(typeof(Texts));
        Bind<Slider>(typeof(Sliders));
    }

    // 외부(PlayerController 등)에서 Stat 정보를 넘겨줌
    public void SetStat(Stat stat, string name)
    {
        Init();
        _stat = stat;
        var nameText = GetText((int)Texts.PlayerNameText);
        if (nameText != null)
            nameText.text = name;

        // 3. 데이터 바인딩 (이벤트 구독)
        // Stat 클래스에 OnHpChanged 액션이 있다고 가정
        _stat.OnHpChanged -= UpdateHP;
        _stat.OnHpChanged += UpdateHP;

        UpdateHP(_stat.MaxHp); // 초기값 설정

        _stat.OnMpChanged -= UpdateMp;
        _stat.OnMpChanged += UpdateMp;

        UpdateMp(_stat.MaxMp);
    }

    void UpdateHP(int hp)
    {
        if (_stat == null || _stat.hp == 0) return;

        float ratio = (float)hp / _stat.hp;
        Get<Slider>((int)Sliders.HPBar).value = ratio;
    }

    void UpdateMp(int mp) 
    {
        if (_stat == null || _stat.mp == 0) return;

        float ratio = (float)mp / _stat.mp;
        Get<Slider>((int)Sliders.MPBar).value = ratio;
    }
}