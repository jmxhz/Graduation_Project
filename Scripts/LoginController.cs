using UnityEngine;
using UnityEngine.UI;
using TMPro;
using UnityEngine.SceneManagement;
using UnityEngine.Networking;
using System.Collections;


public class LoginController : MonoBehaviour
{
    // 声明公共变量用于绑定
    public TMP_InputField usernameInput;
    public TMP_InputField passwordInput;
    public Button loginButton;

    void Start()
    {
        // 绑定按钮点击事件
        loginButton.onClick.AddListener(OnLoginClick);
    }

    private void OnLoginClick()
    {
        Debug.Log($"尝试登录：用户={usernameInput.text}");
        // 后续添加实际登录逻辑
        // 替换原有Debug.Log
        StartCoroutine(LoginRequest(
            usernameInput.text,
            passwordInput.text
        ));

        // 新增协程方法
        IEnumerator LoginRequest(string username, string password)
        {
            string url = "https://zhouyulin.online/login";
            WWWForm form = new WWWForm();
            form.AddField("user", username);
            form.AddField("pass", password);

            using (UnityWebRequest www = UnityWebRequest.Post(url, form))
            {
                yield return www.SendWebRequest();

                if (www.result == UnityWebRequest.Result.Success)
                {
                    SceneManager.LoadScene("MainScene");
                }
                else
                {
                    Debug.LogError($"登录失败：{www.error}");
                }
            }
        }

    }
}
