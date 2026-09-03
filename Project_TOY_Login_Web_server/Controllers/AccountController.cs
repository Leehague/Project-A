using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using Microsoft.Identity.Client;
using Project_TOY_Login_Web_server.Data;
using Project_TOY_Login_Web_server.Models;
using Project_TOY_Login_Web_server.Services;
using StackExchange.Redis;
using System.ComponentModel.DataAnnotations;

namespace Project_TOY_Login_Web_server.Controllers
{
    //DTO defintion
    public record RegisterRequest([Required] string Username, [Required, MinLength(4)] string Password);

    public record RegisterResponse(bool Success, string Message);

    public record LoginRequest([Required] string Username, [Required] string Password);
    public record LoginResponse(
        bool Success,
        string Message,
        string Token,
        int AccountId,
        List<Character>? Characters,
        string GameServerIp,
        int GameServerPort
        );

    public record CharacterDto(int CharacterId, string CharacterName, int Level, int MapId);


    public record CharacterCreateRequest(
        [Required] int AccountId ,
        [Required] string CharacterName ,
        [Required] int TemplateId,
        [Required] int MapId
        );


    [ApiController]
    [Route("api/[controller]")]
    public class AccountController : ControllerBase
    {
        private readonly AppDbContext _db;      // 게임 MSSQL DB
        private readonly IDatabase _redisDb;    // 세션 Redis
        private readonly DataManager _dataManager;


        public AccountController(AppDbContext db, IConnectionMultiplexer redis, DataManager dataManager)
        {
            _db = db;
            _redisDb = redis.GetDatabase();
            _dataManager = dataManager;
        }

        //회원가입 API (실제 DB에 쓰기)
        [HttpPost("register")]
        public async Task<IActionResult> Register([FromBody] RegisterRequest request)
        {
            Console.WriteLine("회원가입 시도");

            // DB에서 중복 유저 검사
            bool isExist = await _db.Accounts.AnyAsync(u => u.Username == request.Username);
            if (isExist)
            {
                Console.WriteLine("회원가입 실패");

                return BadRequest(new RegisterResponse(
                Success: false,
                Message: "회원가입 실패, 이미 존재하는 사용자 이름입니다"

                ));
            }

            // 패스워드 솔트 및 해시화
            string hash = BCrypt.Net.BCrypt.HashPassword(request.Password);

            var newAccount = new Account
            {
                Username = request.Username,
                PasswordHash = hash,

                CreatedDate = DateTime.UtcNow,
                LastLoginDate = DateTime.UtcNow,
            };

            // DB에 계정 추가 및 저장
            _db.Accounts.Add(newAccount);
            await _db.SaveChangesAsync();

            Console.WriteLine("회원가입 성공");

            return Ok(new RegisterResponse(
                Success: true,
                Message: "회원가입 성공!"

            ));
        }

        //로그인 API (DB 검증 후 Redis에 토큰 + AccountId 매핑 저장)
        [HttpPost("login")]
        public async Task<IActionResult> Login([FromBody] LoginRequest request)
        {
            // DB에서 유저 조회
            var account = await _db.Accounts.FirstOrDefaultAsync(u => u.Username == request.Username);
            if (account == null)
            {
                return Unauthorized(new LoginResponse(false, "아이디 또는 비밀번호가 틀렸습니다.", "", 0, null, "", 0));
            }

            // 패스워드 해시 비교 검증
            bool isPasswordCorrect = BCrypt.Net.BCrypt.Verify(request.Password, account.PasswordHash);
            if (!isPasswordCorrect)
            {
                return Unauthorized(new LoginResponse(false, "아이디 또는 비밀번호가 틀렸습니다.", "", 0, null, "", 0));
            }


            // 1회성 세션 토큰 생성 (GUID)
            string sessionToken = Guid.NewGuid().ToString();

            // Redis Key: "Session:Token:{토큰}"
            string redisKey = $"Session:Token:{sessionToken}";

            // Redis Value: "게임 DB의 AccountId" 저장 (문자열로 변환하여 저장)
            string redisValue = account.AccountId.ToString();

            // 토큰 유효 기간 (1시간)
            TimeSpan expiry = TimeSpan.FromHours(1);

            // Redis 저장 완료
            await _redisDb.StringSetAsync(redisKey, redisValue, expiry);

            //해당 Account의 character 정보 db로부터 가져오기
            List<Character> Characterlist = await GetCharactersByAccountIdAsync(account.AccountId);

            if (Characterlist == null)
            {
                return StatusCode(500, new { Message = "데이터베이스에서 캐릭터 정보를 조회하는 도중 오류가 발생했습니다." });
            }

            account.LastLoginDate = DateTime.UtcNow;

            _db.SaveChanges();

            // 클라이언트에 성공 상태와 토큰, AccountId 반환 
            // TODO: targetGameServerIp,targetGameServerPort 를 외부에 저장해놓고 불러와서 전송해주는 식으로 수정
            string targetGameServerIp = "127.0.0.1";
            int targetGameServerPort = 7777;

            return Ok(new LoginResponse(
                Success: true,
                Message: "로그인 성공!",
                Token: sessionToken,
                AccountId: account.AccountId,
                Characters: Characterlist,
                GameServerIp: targetGameServerIp,
                GameServerPort: targetGameServerPort
            ));
        }



        //캐릭터 정보 가져오기 API
        [HttpPost("characters/{accountId}")]
        public async Task<IActionResult> GetCharacters(int accountId)
        {

            var list = await GetCharactersByAccountIdAsync(accountId);
            return Ok(list);
        }

        //헬퍼 함수 (내부 호출용)
        private async Task<List<Character>> GetCharactersByAccountIdAsync(int accountId)
        {
            return await _db.Characters
            .Where(c => c.AccountId == accountId)
            .ToListAsync();
        }


        //캐릭터 생성 API
        [HttpPost("createcharacter")]
        public async Task<IActionResult> CreateCharacter([FromBody]  CharacterCreateRequest request)
        {
            // 메모리에서 O(1)로 템플릿 유효성 검증 및 기본 스탯 획득
            if (!_dataManager.StatDict.TryGetValue(request.TemplateId, out var template))
            {
                return BadRequest("존재하지 않는 캐릭터 템플릿입니다.");
            }

            //일부는 사용자 입력, 일부는 기본값, 일부는 템플릿 데이터로부터 가져온 데이터들을 합쳐서 생성함
            var newCharacter = new Character
            {
                AccountId = request.AccountId, //사용자 입력
                CharacterName = request.CharacterName, //사용자 입력
                TemplateId = request.TemplateId, //사용자 입력
                Level =0, //기본값
                CurrentExp=0, //기본값
                CurrentHp = template.MaxHp, //템플릿 데이터
                CurrentMp = template.MaxMp, //템플릿 데이터
                CurrentAttack = template.attack, //템플릿 데이터
                CurrentSpeed = template.speed, //템플릿 데이터
                MapId = request.MapId, //사용자 입력
                LastSavedDate = DateTime.UtcNow, //기본값
                Gold =0 //기본값
            };

             _db.Characters.Add(newCharacter);

            await _db.SaveChangesAsync();
            return Ok(newCharacter);
        }

    }
}
