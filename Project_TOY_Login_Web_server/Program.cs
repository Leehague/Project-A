
using Microsoft.EntityFrameworkCore;
using Project_TOY_Login_Web_server.Data;
using StackExchange.Redis;

namespace Project_TOY_Login_Web_server
{
    public class Program
    {
        public static void Main(string[] args)
        {
            var builder = WebApplication.CreateBuilder(args);

            // Add services to the container.

            builder.Services.AddSingleton<IConnectionMultiplexer>(sp =>
            {
                var configuration = "localhost:6379"; // 로컬 도커 Redis 주소
                return ConnectionMultiplexer.Connect(configuration);
            });

            builder.Services.AddDbContext<AppDbContext>(options =>
            options.UseSqlServer(builder.Configuration.GetConnectionString("GameDbConnection")));


            builder.Services.AddControllers();
            

            builder.Services.AddEndpointsApiExplorer();
            builder.Services.AddSwaggerGen();

            var app = builder.Build();

            // Configure the HTTP request pipeline.
            if (app.Environment.IsDevelopment())
            {
                app.UseSwagger();
                app.UseSwaggerUI();
            }

            app.UseHttpsRedirection();

            app.UseAuthorization();


            app.MapControllers();

            app.Run();
        }
    }
}
