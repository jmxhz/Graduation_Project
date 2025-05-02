using UnityEngine;
using UnityEngine.Networking;
using TMPro;
using System.Collections;

public class JsonLoader : MonoBehaviour
{
    public TMP_Text displayText; // 用于显示文本的TextMeshPro组件

    void OnEnable()
    {
        // 当对象激活/显示时开始加载数据
        LoadJsonData();
    }

    void LoadJsonData()
    {
        // 更新文本内容
        displayText.text = DataManager.Instance.GetDisplayData().content;

        // 更新高度并设置位置
        UpdateTextHeightAndPosition();
    }

    void UpdateTextHeightAndPosition()
    {
        // 强制计算文本布局
        displayText.ForceMeshUpdate();

        // 获取文本实际需要的高度
        float preferredHeight = displayText.preferredHeight + 50;

        // 更新高度但不改变宽度
        RectTransform rt = displayText.rectTransform;
        rt.sizeDelta = new Vector2(rt.sizeDelta.x, preferredHeight);

        // 设置Y坐标为-177
        Vector2 pos = rt.anchoredPosition;
        pos.y = -177f;
        rt.anchoredPosition = pos;
    }
}
