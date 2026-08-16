using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using StackExchange.Redis;
using Project_TOY_Login_Web_server.Data;
using Project_TOY_Login_Web_server.Models;
using System.ComponentModel.DataAnnotations;

namespace Project_TOY_Login_Web_server.Controllers
{
    public record RegisterRequest([Required] string Username, [Required, MinLength(4)] string Password);
    public record LoginRequest([Required] string Username, [Required] string Password);
    public record LoginResponse(bool Success, string Message, string Token, int AccountId, string GameServerIp, int GameServerPort);

    [ApiController]
    [Route("api/[controller]")]
    public class AccountController : ControllerBase
    {
        private readonly AppDbContext _db;      // 게임 MSSQL DB
        private readonly IDatabase _redisDb;    // 세션 Redis

        public AccountController(AppDbContext db, IConnectionMultiplexer redis)
        {
            _db = db;
            _redisDb = redis.GetDatabase();
        }

        // 1. 회원가입 API (실제 DB에 쓰기)
        [HttpPost("register")]
        public async Task<IActionResult> Register([FromBody] RegisterRequest request)
        {
            // DB에서 중복 유저 검사
            bool isExist = await _db.Accounts.AnyAsync(u => u.Username == request.Username);
            if (isExist)
            {
                return BadRequest(new { Message = "이미 존재하는 사용자 이름입니다." });
            }

            // 패스워드 솔트 및 해시화
            string hash = BCrypt.Net.BCrypt.HashPassword(request.Password);

            var newAccount = new Account
            {
                Username = request.Username,
                PasswordHash = hash
            };

            // DB에 계정 추가 및 저장
            _db.Accounts.Add(newAccount);
            await _db.SaveChangesAsync();

            return Ok(new { Message = "회원가입 성공!" });
        }

        // 2. 로그인 API (DB 검증 후 Redis에 토큰 + AccountId 매핑 저장)
        [HttpPost("login")]
        public async Task<IActionResult> Login([FromBody] LoginRequest request)
        {
            // DB에서 유저 조회
            var account = await _db.Accounts.FirstOrDefaultAsync(u => u.Username == request.Username);
            if (account == null)
            {
                return Unauthorized(new LoginResponse(false, "아이디 또는 비밀번호가 틀렸습니다.", "", 0, "", 0));
            }

            // 패스워드 해시 비교 검증
            bool isPasswordCorrect = BCrypt.Net.BCrypt.Verify(request.Password, account.PasswordHash);
            if (!isPasswordCorrect)
            {
                return Unauthorized(new LoginResponse(false, "아이디 또는 비밀번호가 틀렸습니다.", "", 0, "", 0));
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

            // 클라이언트에 성공 상태와 토큰, AccountId 반환
            string targetGameServerIp = "127.0.0.1";
            int targetGameServerPort = 7777;

            return Ok(new LoginResponse(
                Success: true,
                Message: "로그인 성공!",
                Token: sessionToken,
                AccountId: account.AccountId, // 클라이언트에게도 전달
                GameServerIp: targetGameServerIp,
                GameServerPort: targetGameServerPort
            ));
        }
    }
}
