from openai import OpenAI

# 初始化客户端
client = OpenAI(api_key="", base_url="https://api.deepseek.com/v1")

# 初始对话（系统提示可以帮助定义 AI 的行为）
conversation = [
    {"role": "system", "content": "You are a helpful assistant，默认中文回答"}
]

print("请输入聊天内容（输入 exit 退出）：")
while True:
    user_input = input("User: \n")
    if user_input.lower() in {"exit", "quit"}:
        break

    # 将用户输入添加到对话历史中
    conversation.append({"role": "user", "content": user_input})

    print("Assistant: \n", end="", flush=True)
    # 调用接口生成回复
    response = client.chat.completions.create(
        model="deepseek-chat",
        messages=conversation,
        stream=True,
        temperature=1.3,
        max_tokens=1280,
    )

    assistant_response = ""
    # 处理流式响应
    for chunk in response:
        # 如果 delta 对象中包含 content 属性，则输出，否则输出空字符串
        if hasattr(chunk.choices[0].delta, "content"):
            content = chunk.choices[0].delta.content
            print(content, end="", flush=True)
            assistant_response += content

    # 将 AI 的回复添加到对话历史中，以便保持上下文
    conversation.append({"role": "assistant", "content": assistant_response})
    print("\n")
