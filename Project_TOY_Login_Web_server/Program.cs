
using Microsoft.EntityFrameworkCore;
using Project_TOY_Login_Web_server.Data;
using Project_TOY_Login_Web_server.Services;
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

            //DataManager 추가
            builder.Services.AddSingleton<DataManager>();
            



            builder.Services.AddDbContext<AppDbContext>(options =>
            options.UseSqlServer(builder.Configuration.GetConnectionString("GameDbConnection")));


            builder.Services.AddControllers();
            

            builder.Services.AddEndpointsApiExplorer();
            builder.Services.AddSwaggerGen();


            //빌드
            var app = builder.Build();

            // 초기화 로직
            // 서버 시작 시 메모리에 1회 로드
            DataManager? dataManager = app.Services.GetRequiredService<DataManager>();
            var env = app.Services.GetRequiredService<IWebHostEnvironment>();
            dataManager.Init(env);


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
