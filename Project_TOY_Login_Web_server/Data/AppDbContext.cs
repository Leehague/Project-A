using Microsoft.EntityFrameworkCore;
using Project_TOY_Login_Web_server.Models;

namespace Project_TOY_Login_Web_server.Data
{
    public class AppDbContext : DbContext
    {
        public AppDbContext(DbContextOptions<AppDbContext> options) : base(options)
        {
        }

        // Accounts 테이블을 조작할 수 있는 엔티티 셋 등록
        public DbSet<Account> Accounts { get; set; } 
        public DbSet<Character> Characters { get; set; } 
    }
}
