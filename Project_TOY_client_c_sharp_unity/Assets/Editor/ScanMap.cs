using UnityEngine;
using UnityEditor;
using System.IO;
using System.Text;
using UnityEngine.AI;


public class MapExporter : EditorWindow
{
    [MenuItem("Tools/Export Map Collision")]
    public static void ShowWindow()
    {
        GetWindow<MapExporter>("Map Exporter");
    }

    // ���� ������
    Vector3 minBound = new Vector3(-50, 0, -50);
    Vector3 maxBound = new Vector3(50, 0, 50);
    float cellSize = 0.5f; // 0.5m �������� ���� ��ĵ
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
        // ���� ���� ��� (������Ʈ ���� ��)
        string path = Application.dataPath + $"/../MapData/{mapName}.txt";

        // ������ ������ ����
        if (!Directory.Exists(Application.dataPath + "/../MapData"))
            Directory.CreateDirectory(Application.dataPath + "/../MapData");

        StringBuilder sb = new StringBuilder();

        // �� ���� ��� (����, ���� ũ�� ��)
        int xCount = Mathf.CeilToInt((maxBound.x - minBound.x) / cellSize);
        int zCount = Mathf.CeilToInt((maxBound.z - minBound.z) / cellSize);

        sb.AppendLine($"{xCount} {zCount}"); // ������ ���� ���� ����
        sb.AppendLine($"{minBound.x} {minBound.z} {cellSize}");


        // ����ĳ��Ʈ ��ĵ ����
        for (int z = 0; z < zCount; z++)
        {
            for (int x = 0; x < xCount; x++)
            {
                float posX = minBound.x + (x * cellSize);
                float posZ = minBound.z + (z * cellSize);

                Vector3 rayStart = new Vector3(posX, 100.0f, posZ); // �ϴ� ������
                RaycastHit hit;

                // 1. Obstacle ���̾ ���� �ε������� Ȯ�� (�� üũ)
                int obstacleMask = 1 << LayerMask.NameToLayer("Obstacle");
                int groundMask = 1 << LayerMask.NameToLayer("Ground");

                if (Physics.Raycast(rayStart, Vector3.down, out hit, 200.0f, obstacleMask))
                {
                    // ���� ��� (���̴� �浹 ������ Y��)
                    sb.Append($"1|{hit.point.y:F2} ");
                }
                // 2. Ground ���̾ �ε������� Ȯ�� (�ٴ� üũ)
                else if (Physics.Raycast(rayStart, Vector3.down, out hit, 200.0f, groundMask))
                {
                    // �� �� �ִ� �ٴ��� ���
                    sb.Append($"0|{hit.point.y:F2} ");
                }
                else
                {
                    // ��������
                    sb.Append($"1|-100.0 ");
                }
            }
            sb.AppendLine();
        }

        File.WriteAllText(path, sb.ToString());
        Debug.Log($"Map Export Complete: {path}");
    }
}

public class NavMeshExporter : EditorWindow
{
    [MenuItem("Tools/Export NavMesh for C++ Server")]
    public static void Export()
    {
        // 1. ���� Bake�� NavMesh ������ ��������
        NavMeshTriangulation tri = NavMesh.CalculateTriangulation();

        // 2. ���� ��� ����
        string path = Application.dataPath + "/../MapData/NavMesh_01.bin";

        using (BinaryWriter writer = new BinaryWriter(File.Open(path, FileMode.Create)))
        {
            // [���� ���� ����]
            writer.Write(tri.vertices.Length);
            foreach (Vector3 v in tri.vertices)
            {
                writer.Write(v.x);
                writer.Write(v.y);
                writer.Write(v.z);
            }

            // [�ε���(�ﰢ�� ����) ���� ����]
            writer.Write(tri.indices.Length);
            foreach (int index in tri.indices)
            {
                writer.Write(index);
            }
        }
        Debug.Log($"NavMesh Export Complete: {path} (Vertices: {tri.vertices.Length})");

        // ScanMap.cs�� Export �Լ� ����
        
        Debug.Log($"Vertices: {tri.vertices.Length}, Indices: {tri.indices.Length}"); // ���⼭ 0�� �������� Ȯ��

    }
}