# LanqiaoCup-Avear

蓝桥杯备赛资料的版本库：真题与题解、备赛资料、笔记、仿真电路图、STC15/51（Keil uVision）代码工程。

本仓库只保存**代码、笔记、小文档**；视频、安装包、压缩包和 Keil 编译产物不纳入版本控制（见 `.gitignore`）。
目录统一使用两位数字前缀，保证按 01 → 11 的顺序排列。

## 目录

| 目录 | 文件数 | 内容 |
| --- | ---: | --- |
| `01蓝桥杯备赛  资料` | 680 | 备赛资料（省赛/国赛真题、知识点） |
| `02仿真电路图` | 2 | 仿真电路 |
| `03底层` | 20 | 底层驱动代码 |
| `04DP2026_SCM赛点资源包（26年）` | 45 | 赛点资源包（不含 Tools 里的安装程序） |
| `05蓝桥杯学习 -by左岚` | 540 | 学习资料（左岚） |
| `06考前模板练习` | 83 | 考前模板练习工程 |
| `07总试题集` | 42 | 总试题集 |
| `08蓝桥杯笔记 MD笔记` | 9 | Markdown 笔记 |
| `09微型计算机课程` | 848 | 微型计算机课程作业与 STC89C51 练习工程 |
| `10真题-左岚` | 302 | 历届真题与题解（左岚） |
| `11客观题` | 3 | 客观题 |
| `Codex Program` | 65 | 国赛题目代码工程 |

## 未纳入版本控制

- 视频：`*.mp4 *.avi *.mov *.mkv *.wmv *.flv`
- 安装包/镜像：`*.exe *.msi *.iso *.dmg *.pkg`
- 压缩包：`*.zip *.rar *.7z *.tar *.gz`
- Keil 编译产物与个人 GUI 状态：`*.obj *.o *.d *.m51 *.lst *.hex *.bin *.crf *.axf *.lnp *.sct *.map *.dep *.iex *.plg *.uvgui.* *.uvopt *.bak`，以及 `Objects/ Listings/ DebugConfig/ RTE/`

## 使用

```bash
git clone https://github.com/Aveare/LanqiaoCup-Avear.git
```

更新：

```bash
git add -A
git commit -m "update notes"
git push
```
