using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using TMPro;
using UnityEngine.UI;
using Protocol;
using System;

public class UI_Chat : UI_Scene
{
    enum InputFields
    {
        ChatInput
    }

    enum Buttons
    {
        SendButton
    }

    enum Texts
    {
        ChatHistoryText
    }

    private void Start()
    {
        Init();
    }

    public void Init()
    {
        Bind<TMP_InputField>(typeof(InputFields));
        Bind<Button>(typeof(Buttons));
        Bind<TextMeshProUGUI>(typeof(Texts));

        // 이벤트 리스너 등록
        Get<Button>((int)Buttons.SendButton).onClick.AddListener(OnClickSendButton);
        Get<TMP_InputField>((int)InputFields.ChatInput).onSubmit.AddListener(OnSubmitChat);
    }

    private void OnClickSendButton()
    {
        SendChat();
    }

    private void OnSubmitChat(string text)
    {
        SendChat();
    }

    private void SendChat()
    {
        TMP_InputField input = Get<TMP_InputField>((int)InputFields.ChatInput);
        if (string.IsNullOrEmpty(input.text)) return;

        // 서버로 채팅 패킷 전송
        CS_CHAT chatPkt = new CS_CHAT { Msg = input.text };
        Managers.networkManager.Send(chatPkt); // 실제 프로젝트의 NetworkManager 전송 함수명에 맞게 수정

        input.text = ""; // 입력창 비우기
        input.ActivateInputField(); // 포커스 유지 (연속 타이핑 지원)
    }

    // 서버에서 SC_CHAT_BROADCAST 수신 시 호출할 함수
    public void AddChat(Int32 senderId, string msg)
    {
        TextMeshProUGUI historyText = Get<TextMeshProUGUI>((int)Texts.ChatHistoryText);
        historyText.text += $"\n[Player {senderId}]: {msg}";
    }
}
