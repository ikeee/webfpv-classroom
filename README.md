# WebFPV

在浏览器里练习 FPV 飞行，支持键盘、手柄，也可以用 ESP32-S3 + MPU6050 做体感控制器。

本项目基于 [WebFPVSimulator](https://github.com/Mathew-Harvey/WebFPVSimulator)，增加了 Windows 启动脚本和体感控制器支持。模拟器、飞行物理和课程来自原项目。

## 运行

安装 Node.js 22 或以上版本，在 PowerShell 中运行：

```powershell
git clone https://github.com/ikeee/webfpv-classroom.git
cd webfpv-classroom
.\start-local.ps1
```

打开 <http://127.0.0.1:4186/?map=field>，选择 **Fly Solo → Career → Start → Fly**。
无需安装 npm 依赖或 Steam。停止服务运行 `.\stop-local.ps1`。

## 操作

- **W / S**：加 / 减油门
- **A / D**：左右转向
- **方向键**：俯仰和横滚
- **R**：重置；**Esc**：暂停

## 体感控制器

需要 XH-S3E（ESP32-S3）、MPU6050 模块和 USB 数据线。XH-S3E 默认接线：

| MPU6050 | XH-S3E |
| --- | --- |
| VCC | 3.3V |
| GND | GND |
| SDA | IO41 |
| SCL | IO42 |

烧录固件后，倾斜控制俯仰和横滚，扭转控制转向，W / S 控制油门。
保持模块静止；需要重新归中时，长按 **IO0 / BOOT 3 秒**，松开后再静置 2 秒。
在 **Settings → Choose joystick** 选择控制器；方向反了，在 **Check sticks** 中反转对应通道并保存。

固件、烧录方法、XIAO 接线和常见问题见[控制器教程](hardware/tilt-controller/README.md)。

## 其他说明

- [本地运行说明](LOCAL-DEPLOY.md)：启动、停止与故障排查。
- 排行榜和多人服务需另行部署，本仓库可直接运行单人模式。
- 发布前运行 `npm run public:check`。
- 源码采用 [GPL-3.0-or-later](LICENSE)；第三方图片、音乐等素材的许可见 [NOTICE](NOTICE)。
