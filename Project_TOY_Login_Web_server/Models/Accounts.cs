using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;


namespace Project_TOY_Login_Web_server.Models
{
    
    [Table("Accounts")]
    public class Account
    {
        [Key]
        [Column("AccountId")] 
        public int AccountId { get; set; }

        [Required]
        [StringLength(50)]
        public string Username { get; set; } = string.Empty;

        [Required]
        [StringLength(100)]
        public string PasswordHash { get; set; } = string.Empty;



        
        [StringLength(100)]
        public string? Email { get; set; } = string.Empty;

        [Required]
        public DateTime CreatedDate { get; set; }


        
        public DateTime? LastLoginDate { get; set; }
        
    }

    
}


