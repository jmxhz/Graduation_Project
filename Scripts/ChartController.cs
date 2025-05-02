using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using TMPro;
using XCharts.Runtime;

public class DataVisualization : MonoBehaviour
{
    [Header("UI References")]
    public TMP_Dropdown columnDropdown;
    public Button refreshButton;
    public LineChart chart;
    public GameObject loadingIndicator;

    private readonly string[] columns = {
        "pitch_1 (°)", "pitch_2 (°)",
        "range_1 (mm)", "range_2 (mm)",
        "roll_1 (°)", "roll_2 (°)"
    };

    // 定义颜色常量
    private readonly Color32 RAW_DATA_COLOR = new Color32(30, 144, 255, 255);    // 蓝色 - 原始数据
    private readonly Color32 PREDICT_DATA_COLOR = new Color32(255, 71, 87, 255); // 红色 - 预测数据

    private void Start()
    {
        columnDropdown.ClearOptions();
        columnDropdown.AddOptions(new List<string>(columns));
        columnDropdown.onValueChanged.AddListener(OnColumnSelected);
        refreshButton.onClick.AddListener(RefreshData);
        InitializeChart();

        // 订阅数据加载事件
        DataManager.Instance.OnAllDataLoaded += OnDataLoaded;

        // 立即检查当前数据状态
        CheckCurrentDataStatus();
    }

    private void CheckCurrentDataStatus()
    {

        if (DataManager.Instance.GetRawData().Count > 0 ||
            DataManager.Instance.GetPredictData().Count > 0)
        {
            OnDataLoaded();
        }
        else
        {
            RefreshData();
        }
    }

    private void OnDestroy()
    {
        if (DataManager.Instance != null)
        {
            DataManager.Instance.OnAllDataLoaded -= OnDataLoaded;
        }
    }

    private void InitializeChart()
    {
        chart.RemoveData();
        var title = chart.EnsureChartComponent<Title>();
        title.text = "数据可视化";
        title.subText = "请选择要显示的列";

        var xAxis = chart.EnsureChartComponent<XAxis>();
        xAxis.type = Axis.AxisType.Category;
        xAxis.boundaryGap = true;
    }

    private void OnDataLoaded()
    {
        if (columnDropdown.options.Count > 0)
        {
            UpdateChart(columns[columnDropdown.value]);
        }
    }

    private void OnColumnSelected(int index)
    {
        UpdateChart(columns[index]);
    }

    private void UpdateChart(string selectedColumn)
    {

        if (string.IsNullOrEmpty(selectedColumn))
        {
            return;
        }

        chart.ClearData();

        var rawData = DataManager.Instance.GetRawData();
        var predictData = DataManager.Instance.GetPredictData();


        if (rawData.Count == 0 && predictData.Count == 0)
        {
            return;
        }

        var timeList = new List<string>();
        var valueList = new List<double>();
        int validDataPoints = 0;

        // 处理原始数据
        foreach (var entry in rawData)
        {
            try
            {
                double value = GetValueFromDataItem(entry, selectedColumn);

                if (value != 0) validDataPoints++;

                timeList.Add(FormatTime(entry.time));
                valueList.Add(value);
            }
            catch (Exception e)
            {
                continue;
            }
        }

        // 处理预测数据
        foreach (var entry in predictData)
        {
            try
            {
                double value = GetValueFromDataItem(entry, selectedColumn);

                if (value != 0) validDataPoints++;

                timeList.Add(FormatTime(entry.time));
                valueList.Add(value);
            }
            catch (Exception e)
            {
                continue;
            }
        }


        if (timeList.Count == 0 || valueList.Count == 0)
        {
            return;
        }

        // 配置X轴
        var xAxis = chart.EnsureChartComponent<XAxis>();
        xAxis.type = Axis.AxisType.Category;
        xAxis.boundaryGap = true;
        xAxis.axisLabel.inside = false;
        xAxis.data.Clear();
        xAxis.data.AddRange(timeList);

        // 添加系列
        var serie = chart.AddSerie<Line>("数据曲线");
        serie.lineStyle.type = LineStyle.Type.Solid;
        serie.lineStyle.width = 2f;
        serie.symbol.show = true;
        serie.symbol.size = 3;

        // 添加数据点并设置颜色
        for (int i = 0; i < valueList.Count; i++)
        {
            var serieData = serie.AddData(valueList[i]);

            if (serieData == null) continue;

            serieData.EnsureComponent<ItemStyle>();

            // 设置颜色 - 原始数据蓝色，预测数据红色
            if (i < rawData.Count)
            {
                serieData.itemStyle.color = RAW_DATA_COLOR;
            }
            else
            {
                serieData.itemStyle.color = PREDICT_DATA_COLOR;
            }
        }

        chart.RefreshChart();
    }

    private string FormatTime(string timeStr)
    {
        if (DateTime.TryParse(timeStr, out DateTime dt))
        {
            return dt.ToString("HH:mm:ss");
        }
        return timeStr.Length > 8 ? timeStr.Substring(timeStr.Length - 8) : timeStr;
    }

    private double GetValueFromDataItem(DataItem item, string selectedColumn)
    {
        if (item == null)
        {
            return 0;
        }

        double value = 0;

        switch (selectedColumn)
        {
            case "pitch_1 (°)":
                value = item.pitch_1;
                break;
            case "pitch_2 (°)":
                value = item.pitch_2;
                break;
            case "range_1 (mm)":
                value = item.range_1;
                break;
            case "range_2 (mm)":
                value = item.range_2;
                break;
            case "roll_1 (°)":
                value = item.roll_1;
                break;
            case "roll_2 (°)":
                value = item.roll_2;
                break;
            default:
                break;
        }

        value = Math.Round(value, 2);
        return value;
    }

    public void RefreshData()
    {
        StartCoroutine(RefreshDataCoroutine());
    }

    private IEnumerator RefreshDataCoroutine()
    {
        loadingIndicator.SetActive(true);

        // 重置事件标志
        bool rawDataLoaded = false;
        bool predictDataLoaded = false;

        DataManager.Instance.OnRawDataLoaded += () => {
            rawDataLoaded = true;
        };

        DataManager.Instance.OnPredictDataLoaded += () => {
            predictDataLoaded = true;
        };

        // 触发数据刷新
        DataManager.Instance.RefreshData("raw");
        DataManager.Instance.RefreshData("predict");

        // 等待数据加载完成
        float timeout = 10f;
        float elapsed = 0f;

        while ((!rawDataLoaded || !predictDataLoaded) && elapsed < timeout)
        {
            elapsed += Time.deltaTime;
            yield return null;
        }

        loadingIndicator.SetActive(false);

        if (!rawDataLoaded || !predictDataLoaded)
        {
        }
        else
        {
            // 强制更新图表
            if (columnDropdown.options.Count > 0)
            {
                UpdateChart(columns[columnDropdown.value]);
            }
        }
    }
}
