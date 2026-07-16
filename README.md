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
|    功能    | 客户端发送                                 |        | 服务器发送                                            | 注释                 |
|:--------:|:--------------------------------------|--------|:-------------------------------------------------|:-------------------|
|    心跳    | heart \<id>                           |        |                                                  |                    |    
|    注册    |                                       |        |                                                  |                    |
|    登录    | login "number"                        |        |                                                  |                    |
|          |                                       | 登录成功   | login_true "id" "number" "create_time"           |                    |
|          |                                       | 登陆失败   | login_false ""                                   |                    |
|   创建房间   | room_create \<id>                     |        |                                                  |                    |
|          |                                       | 创建房间成功 | room_create_true "room_id"                       |                    |
| 邀请用户进入房间 | room_invite \<id1> \<id2>             |        |                                                  | 用户1邀请用户2           |
|          |                                       | 房间邀请信息 | room_invite_message \<user1> \<user2> \<room_id> |                    |
|   接受邀请   | room_invite_accept \<user> \<room_id> |        |                                                  |                    |
|   拒绝邀请   | room_invite_reject \<user1> \<user2>  |        |                                                  | user1 拒绝 user2 的邀请 |
|          |                                       | 用户进入房间 | room_join \<user> \<room_id>                     |                    |
|          |                                       | 用户离开房间 | room_leave \<user> \<room_id>                    |                    |
|   房间信息   | room_chat <user> "msg"                |        |                                                  |                    |
|          |                                       | 房间信息   | room_chat <user> "msg"                           |                    |
|   开始匹配   |                                       |        |                                                  |                    |
|   取消匹配   |                                       |        |                                                  |                    |
| 确认并进入对局  |                                       |        |                                                  |                    |
|   拒绝对局   |                                       |        |                                                  |                    |
|   对局取消   |                                       |        |                                                  |                    |
|   匹配成功   |                                       |        |                                                  |                    |



## 功能进度

