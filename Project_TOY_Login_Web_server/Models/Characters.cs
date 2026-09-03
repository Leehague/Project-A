using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;
using System.Numerics;

namespace Project_TOY_Login_Web_server.Models
{
    [Table("Characters")]
    public class Character
    {
        [Key]
        [Column("CharacterId")]
        public int CharacterId { get; set; }

        [Required]
        [Column("AccountId")]
        public int AccountId { get; set; }


        [Required]
        [StringLength(50)]
        public string CharacterName { get; set; } = string.Empty;

        [Required]
        public int TemplateId { get; set; }

        [Required]
        public int Level { get; set; }

        [Required]
        public int CurrentExp { get; set; }

        [Required]
        public int CurrentHp { get; set; }

        [Required]
        public int CurrentMp { get; set; }

        [Required]
        public int CurrentAttack { get; set; }

        [Required]
        public double CurrentSpeed { get; set; }

        [Required]
        public double PosX { get; set; }

        [Required]
        public double PosY { get; set; }

        [Required]
        public double PosZ { get; set; }

        [Required]
        public double Yaw { get; set; }

        [Required]
        public int MapId { get; set; }

        [Required]
        public DateTime LastSavedDate { get; set; }

        [Required]
        public long Gold { get; set; }
    }
}
