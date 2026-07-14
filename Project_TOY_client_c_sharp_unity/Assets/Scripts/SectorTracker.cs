using System;
using System.Collections;
using UnityEngine;

public class SectorTracker : MonoBehaviour
{
    [Header("설정(setting)")]
    public Transform TrackerTarget_transform; // 감시 대상, 주로 플레이어 자신
    public float sectorSize = 50f;    // 섹터 크기 (m)
    public float checkInterval = 0.2f;// 감시 주기 (초)

    // 섹터 변경 이벤트: (이전 섹터, 새로운 섹터)
    public event Action<Vector2Int, Vector2Int> OnSectorChanged;

    public Vector2Int CurrentSector { get; private set; }

    private WaitForSeconds waitInstruction;
    private Coroutine trackCoroutine;

    void Awake()
    {
        waitInstruction = new WaitForSeconds(checkInterval);
    }

    void OnEnable()
    {
        if (TrackerTarget_transform != null)
        {
            CurrentSector = GetSectorFromPosition(TrackerTarget_transform.position);
            trackCoroutine = StartCoroutine(CoTrackPlayerSector());
        }
    }

    void OnDisable()
    {
        if (trackCoroutine != null)
        {
            StopCoroutine(trackCoroutine);
            trackCoroutine = null;
        }
    }

    IEnumerator CoTrackPlayerSector()
    {
        while (true)
        {
            if (TrackerTarget_transform != null)
            {
                Vector2Int newSector = GetSectorFromPosition(TrackerTarget_transform.position);

                if (newSector != CurrentSector)
                {
                    Vector2Int oldSector = CurrentSector;
                    CurrentSector = newSector;

                    // 이벤트를 전파합니다.
                    OnSectorChanged?.Invoke(oldSector, newSector);
                }
            }
            yield return waitInstruction;
        }
    }

    private Vector2Int GetSectorFromPosition(Vector3 position)
    {
        int x = Mathf.FloorToInt(position.x / sectorSize);
        int y = Mathf.FloorToInt(position.z / sectorSize); // XZ 평면
        return new Vector2Int(x, y);
    }
}
