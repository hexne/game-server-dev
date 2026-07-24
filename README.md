# Game Server Dev

# Manager 命令
- [x] ls 显示客户端列表
- [x] add 添加客户端
  - [x] add number, 添加number个客户端
- [x] login 客户端登录
  - [x] login 无参数时全部登录
  - [x] login index, index客户端登录

# Client与Server通讯时的消息格式
> number 账号  
> id 用户id  
> room_id 房间id  
> \<number\> 表示数字  
> "string" 表示字符串  
> "" 空，无内容  

# 通信消息文档
| 功能             | 客户端发送                                         |              | 服务器发送                                    | 注释                                             |
|------------------|----------------------------------------------------|--------------|-----------------------------------------------|--------------------------------------------------|
| 心跳             | heart <id>                                         |              |                                               |                                                  |
| 注册             |                                                    |              |                                               |                                                  |
| 登录             | login "number"                                     |              |                                               |                                                  |
|                  |                                                    | 登录成功     | login_true "id" "number" "create_time"        |                                                  |
|                  |                                                    | 登陆失败     | login_false ""                                |                                                  |
| 创建房间         | room_create <id>                                   |              |                                               |                                                  |
|                  |                                                    | 创建房间成功 | room_create_true <room_id> <room_master>      |                                                  |
| 邀请用户进入房间 | room_invite <id1> <id2> <room_id>                  |              |                                               | 用户1邀请用户2                                   |
|                  |                                                    | 房间邀请信息 | room_invite_message <user1> <user2> <room_id> |                                                  |
| 接受邀请         | room_invite_accept <user> <room_id>                |              |                                               |                                                  |
| 拒绝邀请         | room_invite_reject <user1> <user2>                 |              |                                               | user1 拒绝 user2 的邀请                          |
|                  |                                                    | 用户进入房间 | room_join <user> <room_id> <\room_master>     |                                                  |
|                  |                                                    | 用户离开房间 | room_leave <user> <room_id>                   |                                                  |
| 房间信息         | room_chat <user> "msg"                             |              |                                               |                                                  |
|                  |                                                    | 房间信息     | room_chat <user> "msg"                        |                                                  |
| 开始匹配         | match_join <room_id>                               |              |                                               |                                                  |
|                  |                                                    | 匹配成功     | match_success <pending_match_id>              | 两个房间匹配成功后，向双方所有用户下发           |
| 确认对局         | match_accept <user_id> <pending_match_id>          |              |                                               |                                                  |
| 拒绝对局         | match_reject <user_id> <pending_match_id>          |              |                                               | 收到即视为整个对局取消                           |
|                  |                                                    | 取消匹配     | match_cancle ""                               |                                                  |
|                  |                                                    | 进入对局     | battle_pick_hero <battle_id>                  | 所有用户确认对局后下发，通知进入选英雄阶段       |
| 对局取消         |                                                    |              | match_cancel ""                               | 由任意一方拒绝或确认超时触发，广播给双方所有用户 |
| 选择英雄         | battle_pick_hero <battle_id> <user_id> <hero_name> |              |                                               | 等待所有玩家选择完毕                             |
|                  |                                                    | 开始加载     | battle_start_load ""                          | 所有玩家选完英雄后下发                           |
| 加载进度上报     | battle_load <battle_id> <user_id> <val>            |              |                                               | val 为加载进度百分比                             |
|                  |                                                    | 对局开始     | battle_start ""                               | 所有玩家加载完成后下发                           |

## 功能进度

