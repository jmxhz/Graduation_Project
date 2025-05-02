using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using TMPro;

public class BuildingWallVisualizer : MonoBehaviour
{
    [Header("Model References")]
    public GameObject buildingModel;
    public Transform wall1;
    public Transform wall2;

    [Header("UI References")]
    public Scrollbar timelineScrollbar;
    public TextMeshProUGUI timeDisplayText;

    [Header("Animation Settings")]
    public float maxTiltAngle = 30f;
    public float maxDisplacement = 0.5f;

    private List<DataItem> rawData;
    private List<DataItem> predictData;
    private int currentDataIndex = 0;
    private bool modelInitialized = false;
    private bool isMouseOverUI = false;
    private bool allowModelRotation = true;

    private void OnEnable()
    {
        if (buildingModel != null)
        {
            buildingModel.SetActive(true);
        }
        InitializeData();
    }

    private void OnDisable()
    {
        if (buildingModel != null)
        {
            buildingModel.SetActive(false);
        }
    }

    private void InitializeData()
    {
        try
        {
            if (DataManager.Instance == null)
            {
                Debug.LogError("DataManager instance is null");
                return;
            }

            rawData = DataManager.Instance?.GetRawData() ?? new List<DataItem>();
            predictData = DataManager.Instance?.GetPredictData() ?? new List<DataItem>();

            if (rawData.Count == 0 && predictData.Count == 0)
            {
                Debug.LogWarning("No data available. Waiting for data...");
                StartCoroutine(WaitForData());
                return;
            }

            if (timelineScrollbar != null)
            {
                timelineScrollbar.onValueChanged.AddListener(OnTimelineChanged);
                timelineScrollbar.numberOfSteps = rawData.Count + predictData.Count;

                // 添加事件触发器检测鼠标悬停
                var trigger = timelineScrollbar.gameObject.GetComponent<EventTrigger>() ??
                            timelineScrollbar.gameObject.AddComponent<EventTrigger>();

                var enterEntry = new EventTrigger.Entry();
                enterEntry.eventID = EventTriggerType.PointerEnter;
                enterEntry.callback.AddListener((data) => { isMouseOverUI = true; });

                var exitEntry = new EventTrigger.Entry();
                exitEntry.eventID = EventTriggerType.PointerExit;
                exitEntry.callback.AddListener((data) => { isMouseOverUI = false; });

                trigger.triggers.Add(enterEntry);
                trigger.triggers.Add(exitEntry);
            }

            modelInitialized = true;
            UpdateWallPositions(0);
        }
        catch (System.Exception ex)
        {
            Debug.LogError($"InitializeData failed: {ex.Message}");
        }
    }

    private IEnumerator WaitForData()
    {
        yield return new WaitForSeconds(1f);
        InitializeData();
    }

    private void Update()
    {
        // 只在鼠标不在UI上且允许旋转时旋转模型
        if (allowModelRotation && !isMouseOverUI && Input.GetMouseButton(0))
        {
            float rotX = Input.GetAxis("Mouse X") * 5f;
            float rotY = Input.GetAxis("Mouse Y") * 5f;
            buildingModel.transform.Rotate(Vector3.up, -rotX, Space.World);
            buildingModel.transform.Rotate(Vector3.right, rotY, Space.World);
        }
    }

    private void OnTimelineChanged(float value)
    {
        if (!modelInitialized) return;

        // 在滑动时临时禁止模型旋转
        allowModelRotation = false;

        int dataIndex = Mathf.RoundToInt(value * (rawData.Count + predictData.Count - 1));
        dataIndex = Mathf.Clamp(dataIndex, 0, rawData.Count + predictData.Count - 1);

        UpdateWallPositions(dataIndex);

        // 延迟恢复模型旋转
        StartCoroutine(EnableModelRotationAfterDelay());
    }

    private IEnumerator EnableModelRotationAfterDelay()
    {
        yield return new WaitForSeconds(0.5f);
        allowModelRotation = true;
    }

    private void UpdateWallPositions(int index)
    {
        currentDataIndex = index;
        DataItem currentData = index < rawData.Count ? rawData[index] : predictData[index - rawData.Count];

        if (wall1 != null)
        {
            float tiltX = NormalizeValue(currentData.roll_1, -maxTiltAngle, maxTiltAngle);
            float tiltZ = NormalizeValue(currentData.pitch_1, -maxTiltAngle, maxTiltAngle);
            float displaceY = NormalizeValue(currentData.range_1, 0, maxDisplacement);

            wall1.localRotation = Quaternion.Euler(tiltX, 0, tiltZ);
            wall1.localPosition = new Vector3(0, displaceY, 0);
        }

        if (wall2 != null)
        {
            float tiltX = NormalizeValue(currentData.roll_2, -maxTiltAngle, maxTiltAngle);
            float tiltZ = NormalizeValue(currentData.pitch_2, -maxTiltAngle, maxTiltAngle);
            float displaceY = NormalizeValue(currentData.range_2, 0, maxDisplacement);

            wall2.localRotation = Quaternion.Euler(tiltX, 0, tiltZ);
            wall2.localPosition = new Vector3(0, displaceY, 0);
        }

        if (timeDisplayText != null)
        {
            timeDisplayText.text = currentData.time;
        }
    }

    private float NormalizeValue(float value, float min, float max)
    {
        float normalized = Mathf.InverseLerp(-180f, 180f, value);
        return Mathf.Lerp(min, max, normalized);
    }
}
