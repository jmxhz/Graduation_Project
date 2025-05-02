using UnityEngine;
using UnityEngine.UI;

public class AndroidExitHandler : MonoBehaviour
{
    [Header("UI References")]
    public GameObject confirmPanel;  // 二次确认面板
    public Button confirmExitButton; // 确认退出按钮
    public Button cancelExitButton;  // 取消退出按钮

    private void Start()
    {
        // 初始化时隐藏确认面板
        if (confirmPanel != null)
        {
            confirmPanel.SetActive(false);
        }

        // 设置按钮事件
        if (confirmExitButton != null)
        {
            confirmExitButton.onClick.AddListener(ConfirmExit);
        }

        if (cancelExitButton != null)
        {
            cancelExitButton.onClick.AddListener(CancelExit);
        }
    }

    private void Update()
    {
        // 检测返回键（Android的Escape键）
        if (Input.GetKeyDown(KeyCode.Escape))
        {
            ShowExitConfirmation();
        }
    }

    private void ShowExitConfirmation()
    {
        // 显示二次确认面板
        if (confirmPanel != null)
        {
            confirmPanel.SetActive(true);
        }
    }

    private void ConfirmExit()
    {
        // 退出应用程序
        Debug.Log("Application quitting...");
        Application.Quit();

        // 在编辑器中也能看到效果
#if UNITY_EDITOR
        UnityEditor.EditorApplication.isPlaying = false;
#endif
    }

    private void CancelExit()
    {
        // 隐藏确认面板
        if (confirmPanel != null)
        {
            confirmPanel.SetActive(false);
        }
    }
}
