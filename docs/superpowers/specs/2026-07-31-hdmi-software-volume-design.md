# HDMI(无硬件音量)输出设备的软件音量控制

日期:2026-07-31
基线:上游 `8c25450`(v0.5.0)

## 问题

输出设备为 HDMI/DisplayPort 显示器(如 GS49UK)时,设备不支持硬件音量。
上游行为:`BGMDeviceControlsList::MatchControlsListOf` 会禁用 BGMDevice 的主音量控制,
导致菜单栏滑块变灰、系统音量键失效,无法全局调音量。

## 方案(方案 A:驱动内软件音量)

驱动已有现成基础设施:`BGM_VolumeControl::ApplyVolumeToAudioRT()` 把增益乘到采样上,
IO 路径已条件调用(`BGM_Device::Device_WillDoIOOperation` / `Device_DoIOOperation`),
UI 音效设备已启用。只需让主设备可以按需打开这个开关。

### 驱动改动

- `SharedSource/BGM_Types.h`
  - 新增自定义属性 `kAudioDeviceCustomPropertyApplyVolumeToAudio = 'apva'`
    (CFBoolean,可设置,默认 false)及地址常量 `kBGMApplyVolumeToAudioAddress`(output scope)。
- `BGMDriver/BGMDriver/BGM_VolumeControl.h`
  - `mWillApplyVolumeToAudio` 改为 `std::atomic<bool>`(现在会在运行期被主机线程修改、
    IO 线程读取)。
- `BGMDriver/BGMDriver/BGM_Device.cpp`
  - 按 `kAudioDeviceCustomPropertyDebugLoggingEnabled` 的模板为新属性接入
    HasProperty / IsPropertySettable / GetPropertyDataSize / GetPropertyData /
    SetPropertyData,并加入 CustomPropertyInfoList(7 → 8 项)。
  - SetPropertyData:调用 `mVolumeControl.SetWillApplyVolumeToAudio(value)`,
    值变化时发属性变更通知。

### BGMApp 改动

- `BGMApp/BGMApp/BGMDeviceControlsList.cpp` — `MatchControlsListOf`
  - BGMDevice 的主音量控制**始终保持启用**(不再跟随输出设备禁用)。
  - 输出设备有硬件/虚拟音量 → 设 `ApplyVolumeToAudio = false`(维持上游同步行为);
    没有 → 设 `ApplyVolumeToAudio = true`(驱动应用增益)。
  - 静音控制维持上游行为(跟随输出设备)。
- `BGMDeviceControlSync` 无需改动:对无音量设备,`CopyVolumeFrom` 各级回退后自然 no-op。
- `BGMOutputVolumeMenuItem` 无需改动:控件启用后 `HasSettableMainVolume` 为 true,滑块自动可用。

## 效果

- 选中 HDMI 设备时菜单栏滑块可用,拖动即全局调音量(所有经过 BGMDevice 的音频)。
- 键盘音量键(F11/F12)同样生效(操作的是默认设备 BGMDevice 的音量控制)。
- 滑块拖到 0 → 增益 0,完全静音(无需 mute 控制)。
- 有硬件音量的设备行为与上游完全一致。

## 风险

- `mWillApplyVolumeToAudio` 的跨线程读写:用 atomic 解决。
- 切换输出设备时的瞬时状态(软件增益开着但已切到有硬件音量的设备):
  最坏情况是短暂双重衰减,下一次 MatchControlsListOf 即恢复,可接受。

## 验证

1. 构建 driver + app,安装。
2. 输出设备选 GS49UK:滑块可用,拖动能改变实际响度;音量键生效;滑到 0 静音。
3. 输出设备选内置扬声器:行为与上游一致(滑块同步硬件音量)。
