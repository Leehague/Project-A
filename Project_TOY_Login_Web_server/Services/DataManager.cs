using Project_TOY_Login_Web_server.Models;
using System.Text.Json;
using static Project_TOY_Login_Web_server.Models.StatData;

namespace Project_TOY_Login_Web_server.Services
{
    public class DataManager
    {

        // O(1) 조회를 위한 메모리 캐시 (Key: TemplateId, Value: Stat)
        public Dictionary<int, StatData> StatDict { get; private set; } = new();
        public void Init(IWebHostEnvironment env)
        {
            string filePath = Path.Combine(env.ContentRootPath, "Data", "StatData.json");
            if (File.Exists(filePath))
            {
                string json = File.ReadAllText(filePath);
                // 래퍼 클래스로 역직렬화
                var loader = JsonSerializer.Deserialize<StatDataLoader>(json, new JsonSerializerOptions
                {
                    PropertyNameCaseInsensitive = true
                });
                if (loader != null && loader.stats != null)
                {
                    StatDict = loader.stats.ToDictionary(stat => stat.id);
                    Console.WriteLine($"[DataManager] StatData 로드 완료: {StatDict.Count}개 템플릿 등록됨.");
                }
            }
            else
            {
                Console.WriteLine($"[DataManager] 파일을 찾을 수 없습니다: {filePath}");
            }
        }
    }

    
}
