using System;
using UnityEngine;
using UnityEngine.Networking;
using System.Collections;
using System.Collections.Generic;
using Newtonsoft.Json;

public class DataManager : MonoBehaviour
{
    private static DataManager _instance;
    public static DataManager Instance { get { return _instance; } }

    // 存储三种类型的数据
    private List<DataItem> rawData = new List<DataItem>();
    private List<DataItem> predictData = new List<DataItem>();
    private DisplayData displayData;

    // 数据加载完成事件
    public delegate void DataLoadedHandler();
    public event DataLoadedHandler OnRawDataLoaded;
    public event DataLoadedHandler OnPredictDataLoaded;
    public event DataLoadedHandler OnDisplayDataLoaded;
    public event DataLoadedHandler OnAllDataLoaded;

    private const string API_URL = "https://zhouyulin.online/clientB";
    private const string AUTH_TOKEN = "Bearer SECRET_2025zylKEY_B";

    private void Awake()
    {
        if (_instance != null && _instance != this)
        {
            Destroy(this.gameObject);
        }
        else
        {
            _instance = this;
            DontDestroyOnLoad(gameObject);
        }
    }

    private void Start()
    {
        // 场景加载时自动获取所有数据
        LoadAllData();
    }

    // 加载所有数据
    public void LoadAllData()
    {
        StartCoroutine(LoadData("raw"));
        StartCoroutine(LoadData("predict"));
        StartCoroutine(LoadData("display"));
    }

    // 刷新特定类型数据
    public void RefreshData(string dataType)
    {
        StartCoroutine(LoadData(dataType));
    }

    private IEnumerator LoadData(string dataType)
    {
        string url = $"{API_URL}?data_type={dataType}";

        using (UnityWebRequest webRequest = UnityWebRequest.Get(url))
        {
            webRequest.SetRequestHeader("Authorization", AUTH_TOKEN);

            yield return webRequest.SendWebRequest();

            if (webRequest.result == UnityWebRequest.Result.Success)
            {
                string jsonResponse = webRequest.downloadHandler.text;

                switch (dataType)
                {
                    case "raw":
                        ProcessRawData(jsonResponse);
                        OnRawDataLoaded?.Invoke();
                        break;
                    case "predict":
                        ProcessPredictData(jsonResponse);
                        OnPredictDataLoaded?.Invoke();
                        break;
                    case "display":
                        ProcessDisplayData(jsonResponse);
                        OnDisplayDataLoaded?.Invoke();
                        break;
                }

                // 检查是否所有数据都已加载
                if (rawData.Count > 0 && predictData.Count > 0 && displayData != null)
                {
                    OnAllDataLoaded?.Invoke();
                }
            }
            else
            {
                Debug.LogError($"Error loading {dataType} data: {webRequest.error}");
            }
        }
    }

    private void ProcessRawData(string json)
    {
        try
        {
            var data = JsonConvert.DeserializeObject<ServerData>(json);
            if (data != null && data.raw_data != null)
            {
                rawData = data.raw_data;
                if (rawData.Count > 0)
                {
                }
            }
            else
            {
                rawData = new List<DataItem>();
            }
        }
        catch (Exception ex)
        {
            rawData = new List<DataItem>();
        }
    }

    private void ProcessPredictData(string json)
    {
        try
        {
            var data = JsonConvert.DeserializeObject<ServerData>(json);
            if (data != null && data.predict_data != null)
            {
                predictData = data.predict_data;
                if (predictData.Count > 0)
                {
                }
            }
            else
            {
                predictData = new List<DataItem>();
            }
        }
        catch (Exception ex)
        {
            predictData = new List<DataItem>();
        }
    }

    private void ProcessDisplayData(string json)
    {
        var data = JsonConvert.DeserializeObject<ServerDisplayData>(json);
        displayData = new DisplayData
        {
            id = data.id,
            timestamp = data.timestamp,
            content = data.display_data ?? "无显示数据"
        };
    }

    // 获取数据的公共接口
    public List<DataItem> GetRawData() => rawData;
    public List<DataItem> GetPredictData() => predictData;
    public DisplayData GetDisplayData() => displayData;
}

// 数据模型类
[System.Serializable]
public class ServerData
{
    public int id;
    public string timestamp;

    [JsonProperty("raw_data")]
    public List<DataItem> raw_data;

    [JsonProperty("predict_data")]
    public List<DataItem> predict_data;
}


[System.Serializable]
public class ServerDisplayData
{
    public int id;
    public string timestamp;
    public string display_data;
}

[System.Serializable]
public class DataItem
{
    [JsonProperty("pitch_1 (°)")]
    public float pitch_1;

    [JsonProperty("pitch_2 (°)")]
    public float pitch_2;

    [JsonProperty("range_1 (mm)")]
    public float range_1;

    [JsonProperty("range_2 (mm)")]
    public float range_2;

    [JsonProperty("roll_1 (°)")]
    public float roll_1;

    [JsonProperty("roll_2 (°)")]
    public float roll_2;

    [JsonProperty("time")]
    public string time;
}

[System.Serializable]
public class DisplayData
{
    public int id;
    public string timestamp;
    public string content;
}
