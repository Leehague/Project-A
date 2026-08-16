using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;

namespace Project_TOY_Login_Web_server.Models
{
    // 실제 MSSQL의 테이블명이 "Accounts"라고 가정합니다.
    [Table("Accounts")]
    public class Account
    {
        [Key]
        [Column("AccountId")] // DB 컬럼명이 AccountId 일 경우
        public int AccountId { get; set; }

        [Required]
        [StringLength(50)]
        public string Username { get; set; } = string.Empty;

        [Required]
        [StringLength(100)]
        public string PasswordHash { get; set; } = string.Empty;

        // 인게임 닉네임, 생성일 등이 더 있다면 아래에 추가 정의합니다.
    }
}
