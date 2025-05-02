using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;

public class DeBug : MonoBehaviour
{
    [Header("组件绑定")]
    [SerializeField] private ScrollRect scrollRect;

    void Start()
    {
        if (scrollRect == null)
        {
            Debug.LogError("ScrollRect 未绑定！");
            return;
        }

        // 事件监听
        scrollRect.onValueChanged.AddListener(OnScrollValueChanged);

        // 添加事件触发器
        var trigger = scrollRect.gameObject.AddComponent<EventTrigger>();

        // 滚动事件
        var scrollEntry = new EventTrigger.Entry
        {
            eventID = EventTriggerType.Scroll
        };
        scrollEntry.callback.AddListener(OnScroll);
        trigger.triggers.Add(scrollEntry);

        // 拖动事件
        var dragEntry = new EventTrigger.Entry
        {
            eventID = EventTriggerType.Drag
        };
        dragEntry.callback.AddListener(OnDrag);
        trigger.triggers.Add(dragEntry);
    }

    private void OnScrollValueChanged(Vector2 pos)
    {
        Debug.Log($"滚动位置更新：{pos}");
    }

    private void OnScroll(BaseEventData data)
    {
        if (data is PointerEventData pointerData)
        {
            Debug.Log($"滚动事件 delta:{pointerData.scrollDelta}");
        }
    }

    private void OnDrag(BaseEventData data)
    {
        if (data is PointerEventData pointerData)
        {
            Debug.Log($"拖动位置：{pointerData.position}");
        }
    }
}
