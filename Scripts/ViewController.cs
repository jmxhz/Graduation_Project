using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class ViewController : MonoBehaviour
{
    [Header("主菜单面板")]
    public GameObject mainMenuPanel;

    [Header("子界面面板")]
    public GameObject[] contentPanels;

    private GameObject currentContentPanel;

    void Start()
    {
        // 初始化隐藏所有内容面板
        foreach (var panel in contentPanels)
        {
            panel.SetActive(false);
        }
        mainMenuPanel.SetActive(true);
    }

    // 进入子界面
    public void EnterContentPanel(int index)
    {
        if (index < 0 || index >= contentPanels.Length) return;

        Debug.Log($"当前激活面板：{contentPanels[index].name}");
        mainMenuPanel.SetActive(false);
        contentPanels[index].SetActive(true);
        currentContentPanel = contentPanels[index];
    }

    // 返回主菜单
    public void BackToMainMenu()
    {
        mainMenuPanel.SetActive(true);
        if (currentContentPanel != null)
        {
            currentContentPanel.SetActive(false);
        }
    }


}
