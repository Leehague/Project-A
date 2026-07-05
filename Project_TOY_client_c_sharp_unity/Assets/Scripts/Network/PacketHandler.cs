using System;
using Google.Protobuf;
using Protocol;

using UnityEngine;

public class PacketHandler
{
    public static void Handle_SC_LOGIN_OK(PacketSession session, IMessage packet)
    {
        SC_LOGIN_OK pkt = packet as SC_LOGIN_OK;
        Debug.Log($"Handle_SC_LOGIN_OK: {pkt.ToString()}");

        Managers.objectManager.Myplayer_playerId = pkt.PlayerId;


        if (pkt.Success)
        {

            Managers.sceneManagerEx.LoadScene(Define.SceneType.Loading);
        }
        else 
        {
            Debug.Log("Login is not ok");
        }
        
    }

    
    // PacketHandler.cs 내부 수정
    public static void Handle_SC_CHAT_BROADCAST(PacketSession session, IMessage packet)
    {
        SC_CHAT_BROADCAST chatPkt = packet as SC_CHAT_BROADCAST; // 변수명을 chatPkt로 통일

        // UIManager에 추가한 메소드를 통해 UI_Chat 스크립트를 찾아서 가져옵니다.
        UI_Chat chatUI = Managers.uiManager.FindUI<UI_Chat>();

        if (chatUI != null)
        {
            chatUI.AddChat(chatPkt.PlayerId, chatPkt.Msg);
        }
    }



    public static void Handle_SC_WHISPER(PacketSession session, IMessage packet)
    {
        SC_WHISPER pkt = packet as SC_WHISPER;
        Debug.Log($"Handle_SC_WHISPER: {pkt.ToString()}");
    }

    public static void Handle_SC_MOVING(PacketSession session, IMessage packet)
    {
        SC_MOVING movepkt = packet as SC_MOVING;
        if (movepkt == null) return;

        foreach (PosInfo posInfo in movepkt.PosInfo) 
        {
            GameObject go = Managers.objectManager.Find(posInfo.ObjectId);
            if (go == null) return;

            // 1. 공통 부모인 CreatureController를 먼저 찾습니다.
            CreatureController cc = go.GetComponent<CreatureController>();
            if (cc == null) return;

            // 2. 타입에 따라 분기 처리
            if ((cc is PlayerController pc) && (pc.IsMyPlayer))
            {

                // [내 캐릭터] 서버 위치와 동기화 및 오차 보정
                Vector3 serverPos = new Vector3(posInfo.X, posInfo.Y, posInfo.Z);
                float distance = Vector3.Distance(go.transform.position, serverPos);

                if (distance > 0.5f)
                {
                    // 서버가 강제로 위치를 되돌려야 하는 상황 (예: 갈 수 없는 지역)
                    pc.SyncPos(serverPos);
                }

            }
            else
            {
                // [몬스터] [타인 캐릭터] 몬스터, 타인캐릭터는 항상 서버가 주도권을 가지므로 RefreshPos로 목적지만 갱신
                cc.RefreshPos(posInfo);

                //디버그
                Debug.Log($"yaw of {posInfo.ObjectId} : {posInfo.Yaw}");
            }
        }

    }

    public static void Handle_SC_PLAYER_SPAWN(PacketSession session, IMessage packet)
    {

        
        SC_PLAYER_SPAWN spawnPkt = packet as SC_PLAYER_SPAWN;
               
        
        Managers.objectManager.HandleSpawn(packet as SC_PLAYER_SPAWN);
               
        

    }
    public static void Handle_SC_ENTER_GAME(PacketSession session, IMessage packet) 
    {
        SC_ENTER_GAME enterGamePkt = packet as SC_ENTER_GAME;


        //로그인 패킷에서 받았던 playerId(obejcId)와 일치하는지 확인
        if (enterGamePkt.PosInfo.ObjectId != Managers.objectManager.Myplayer_playerId) { return; }

        //Map 로드
        string Mapname = "Map" + enterGamePkt.MapId.ToString();
        if (!GameObject.Find(Mapname))
        {
            Managers.resourceManager.Instantiate(Mapname);
        }

        //서버가 보내준 나의 초기 위치 정보를 저장
        Managers.objectManager.MyplayerPosInfo=enterGamePkt.PosInfo;

        //내 캐릭터 스폰
        Managers.objectManager.SpawnPlayer(enterGamePkt.PosInfo, enterGamePkt.TemplateId, true);

        //CS_GAME_READY 전송 로직
        Protocol.CS_GAME_READY _CS_GAME_READY = new Protocol.CS_GAME_READY();
        _CS_GAME_READY.PlayerId = Managers.objectManager.Myplayer_playerId; // This playerId must equal to 'Real' player Id in server.
        Managers.networkManager.Send(_CS_GAME_READY);
    }

    public static void Handle_SC_PLAYER_DESPAWN(PacketSession session, IMessage packet) 
    {
        SC_PLAYER_DESPAWN despawnPkt = packet as SC_PLAYER_DESPAWN;

        foreach (int id in despawnPkt.PlayerId)
        {
            Managers.objectManager.Remove(id); // 오브젝트 매니저에 제거 요청
        }
    }
    public static void Handle_SC_SKILL(PacketSession session, IMessage packet)
    {
        SC_SKILL skillPkt = packet as SC_SKILL;
        
        Debug.Log($"skill used :{skillPkt.SkillId}");
        Skill skilldata=Managers.dataManager.SkillDict[skillPkt.SkillId]; //여기서 skilldata 는 수정하면 안됨.
        GameObject Caster=Managers.objectManager.Find(skillPkt.ObjectId); //스킬을 쓴 사람 (Caster)
        CreatureController cc = Caster.GetComponent<CreatureController>();

        switch ((skillType)skilldata.skillTypeId) //애니매이션 틀고 UI 업데이트하고 
        {
            
            case skillType.Common:
                break;
            case skillType.Melee:
                cc.State = Define.CreatureState.Skill;
                break;
            case skillType.Projectile:
                cc.State = Define.CreatureState.Skill;

                if (skillPkt.DestPos == null) { break; }
                Vector3 targetpos = new Vector3(skillPkt.DestPos.X, skillPkt.DestPos.Y, skillPkt.DestPos.Z);
                cc.spawnfireball(targetpos); //TODO: 투사체 종류입력으로 받아서 각 투사체 별 아트리소스를 맵핑하는 함수 필요
                break;
            case skillType.Dash:
                cc.State = Define.CreatureState.Skill;
                Vector3 dashtargetpos = new Vector3(skillPkt.DestPos.X, skillPkt.DestPos.Y, skillPkt.DestPos.Z);
                cc.PlayDashAnimation(dashtargetpos);
                break;
        }

    }
    public static void Handle_SC_CHANGE_HP(PacketSession session, IMessage packet)
    {
        SC_CHANGE_HP sc_change_hp_pkt = packet as SC_CHANGE_HP;
        GameObject HPChanger = Managers.objectManager.Find(sc_change_hp_pkt.ObjectId);
        CreatureController HPChanger_cc = HPChanger.GetComponent<CreatureController>();
        if (HPChanger_cc != null)
        {
            HPChanger_cc.stat.hp = sc_change_hp_pkt.CurrentHp; 
            //stat class에 등록된 UI의 함수들로 자동 처리, UI를 직접 호출할 필요 없음 }

            //attacker_id 와 damage 를 이용한 UI 이벤트 발생등 코드 추가 가능

        }
    }
    public static void Handle_SC_CHANGE_MP(PacketSession session, IMessage packet) 
    {
        SC_CHANGE_MP sc_change_mp_pkt= packet as SC_CHANGE_MP;
        GameObject MpChanger = Managers.objectManager.Find(sc_change_mp_pkt.ObjectId);
        CreatureController MpChanger_cc = MpChanger.GetComponent<CreatureController>();
        if (MpChanger_cc != null) 
        {
            MpChanger_cc.stat.mp = sc_change_mp_pkt.CurrentMp;
        }
        
    }

    public static void Handle_SC_MONSTER_SPAWN(PacketSession session, IMessage packet)
    {
        SC_MONSTER_SPAWN sc_monster_spawn_pkt= packet as SC_MONSTER_SPAWN;

        foreach (SpawnInfo info in sc_monster_spawn_pkt.MonstersSpawnInfo) 
        {
            // ObjectManager에서 몬스터 스폰 처리 
            Managers.objectManager.SpawnMonster(info.Spawnposinfo, info.TemplateId);
        }
    }

    public static void Handle_SC_MONSTER_DEAD(PacketSession session, IMessage packet)
    {
        SC_MONSTER_DEAD sc_monster_dead_pkt = packet as SC_MONSTER_DEAD;

        foreach (int obejectid in sc_monster_dead_pkt.DeadObjectIdList) 
        {
            CreatureController deadcc =Managers.objectManager.FindController(obejectid);
            deadcc.OnDead();
        }
    }

    public static void Handle_SC_ITEM_RESPONSE(PacketSession session, IMessage packet)
    {
        SC_ITEM_RESPONSE sc_item_resonse_pkt = packet as SC_ITEM_RESPONSE;

        // 열려있는 인벤토리 팝업 UI를 찾아 즉시 갱신합니다.
        UI_Inventory invenUI = Managers.uiManager.FindUI<UI_Inventory>();
        if (invenUI != null)
        {
            invenUI.RefreshUI(sc_item_resonse_pkt.Items);
        }
        
        // (선택) UI가 닫혀있을 때를 대비해 Managers.objectManager.MyInventory 등에
        // sc_item_resonse_pkt.Items 데이터를 캐싱(저장)해 두는 구조를 추가하면 더 좋습니다.
    }
}
