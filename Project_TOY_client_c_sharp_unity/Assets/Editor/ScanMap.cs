using UnityEngine;
using UnityEditor;
using System.IO;
using System.Text;

public class MapExporter : EditorWindow
{
    [MenuItem("Tools/Export Map Collision")]
    public static void ShowWindow()
    {
        GetWindow<MapExporter>("Map Exporter");
    }

    // 설정 변수들
    Vector3 minBound = new Vector3(-50, 0, -50);
    Vector3 maxBound = new Vector3(50, 0, 50);
    float cellSize = 0.5f; // 0.5m 간격으로 정밀 스캔
    string mapName = "Map_01";

    void OnGUI()
    {
        mapName = EditorGUILayout.TextField("Map Name", mapName);
        minBound = EditorGUILayout.Vector3Field("Min Bound", minBound);
        maxBound = EditorGUILayout.Vector3Field("Max Bound", maxBound);
        cellSize = EditorGUILayout.FloatField("Cell Size", cellSize);

        if (GUILayout.Button("Export Map Data"))
        {
            Export();
        }
    }

    void Export()
    {
        // 파일 저장 경로 (프로젝트 폴더 내)
        string path = Application.dataPath + $"/../MapData/{mapName}.txt";

        // 폴더가 없으면 생성
        if (!Directory.Exists(Application.dataPath + "/../MapData"))
            Directory.CreateDirectory(Application.dataPath + "/../MapData");

        StringBuilder sb = new StringBuilder();

        // 맵 정보 헤더 (가로, 세로 크기 등)
        int xCount = Mathf.CeilToInt((maxBound.x - minBound.x) / cellSize);
        int zCount = Mathf.CeilToInt((maxBound.z - minBound.z) / cellSize);

        sb.AppendLine($"{xCount} {zCount}"); // 서버가 먼저 읽을 정보
        sb.AppendLine($"{minBound.x} {minBound.z} {cellSize}");


        // 레이캐스트 스캔 루프
        for (int z = 0; z < zCount; z++)
        {
            for (int x = 0; x < xCount; x++)
            {
                float posX = minBound.x + (x * cellSize);
                float posZ = minBound.z + (z * cellSize);

                Vector3 rayStart = new Vector3(posX, 100.0f, posZ); // 하늘 위에서
                RaycastHit hit;

                // 1. Obstacle 레이어에 먼저 부딪히는지 확인 (벽 체크)
                int obstacleMask = 1 << LayerMask.NameToLayer("Obstacle");
                int groundMask = 1 << LayerMask.NameToLayer("Ground");

                if (Physics.Raycast(rayStart, Vector3.down, out hit, 200.0f, obstacleMask))
                {
                    sb.Append("1 "); // 벽이 있음 (갈 수 없음)
                }
                // 2. Ground 레이어에 부딪히는지 확인 (바닥 체크)
                else if (Physics.Raycast(rayStart, Vector3.down, out hit, 200.0f, groundMask))
                {
                    sb.Append("0 "); // 바닥임 (갈 수 있음)
                }
                else
                {
                    sb.Append("1 "); // 낭떠러지나 허공 (갈 수 없음)
                }
            }
            sb.AppendLine();
        }

        File.WriteAllText(path, sb.ToString());
        Debug.Log($"Map Export Complete: {path}");
    }
}