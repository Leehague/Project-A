
public class Define
{
    // UI 관련 이벤트 정의
    public enum UIEvent
    {
        Click,   // 마우스 클릭
        Drag,    // 마우스 드래그
        BeginDrag, // 드래그 시작
        EndDrag,   // 드래그 종료
    }

    // 씬 종류 정의 
    public enum SceneType
    {
        Unknown = 0,
        Login = 1,
        Loading = 2,
        Game = 3,
        
    }

    public enum CreatureState
    {
        Idle,
        Moving,
        Jump,
        Fall,
        Landing,
        Skill,
        OnDead,
        Dead,
    }

    
}
