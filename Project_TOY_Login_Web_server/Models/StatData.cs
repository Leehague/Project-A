

namespace Project_TOY_Login_Web_server.Models
{
    //StatData.json 파일( 캐릭터 스탯 템플릿 데이터에 해당)의 내용을 로그인 서버의 메모리에서 사용하기 위한 c# 클래스
    public class StatData
    {
        public int id { get; set; }
        public string name { get; set; } = string.Empty; // string으로 수정
        public int MaxHp { get; set; }
        public int MaxMp { get; set; }
        public int attack { get; set; }
        public int speed { get; set; }
        //string modelPath = string.Empty; , 클라의 아트리소스를 위한 것이니 여기서는 필요없음

        public class StatDataLoader
        {
            public List<StatData> stats { get; set; } = new();
        }
    }
}
