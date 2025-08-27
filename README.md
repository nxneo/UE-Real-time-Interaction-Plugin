# Real-time Interactive 3D Digital Human for Unreal Engine
<!-- 建议在这里放一个项目的 Logo 或 GIF 动图 -->

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![UE Version](https://img.shields.io/badge/Unreal%20Engine-5.5%2B-blue.svg)](https://www.unrealengine.com/)
[![GitHub stars](https://img.shields.io/github/stars/nxneo/UE-Real-time-Interaction-Plugin?style=social&label=Star)](https://github.com/nxneo/UE-Real-time-Interaction-Plugin)

**An powerful Unreal Engine plugin for creating localized, real-time interactive 3D digital humans. This project integrates cutting-edge AI technologies to provide you with a ready-to-use, all-in-one solution.**

**一个强大的虚幻引擎插件，旨在实现本地化的、实时的3D数字人交互。本项目整合了业界前沿的AI技术栈，为您提供一套开箱即用的解决方案。**

---

## ✨ Core Features / 核心功能

*   **Real-time Audio-driven Facial Animation:** Low-latency speech-to-lip-sync and expression generation based on NVIDIA Omniverse Audio2Face.
    > 基于 NVIDIA Omniverse Audio2Face，实现低延迟的语音到口型和表情的实时转换。
*   **LLM-driven Dialogue:** Integrated with the LangChain framework, supporting both locally deployed LLMs (via Ollama) and cloud APIs to empower digital humans with intelligent conversation capabilities.
    > 集成 LangChain 框架，支持对接本地部署的 LLM (通过 Ollama) 或云端 API，赋予数字人智能对话的能力。
*   **Microsoft Azure SDK Integration:** Provides high-quality Text-to-Speech (TTS) and Speech-to-Text (ASR) functionalities.
    > 集成微软 Azure SDK，提供高质量的语音合成 (TTS) 和语音识别 (ASR) 功能。
*   **UE MetaHuman Compatible:** Perfectly adapted for Unreal Engine's MetaHuman characters, enabling the rapid creation of high-fidelity digital humans.
    > 完美适配 Unreal Engine 的 MetaHuman 角色，快速打造高保真数字人。
*   **Blueprint Friendly:** Comes with a rich set of Blueprint nodes and interfaces, allowing you to build and customize interaction logic without deep C++ knowledge.
    > 提供了丰富的蓝图节点和接口，无需深入C++即可快速搭建和定制您的交互逻辑。

---

## 🎥 Demo Video / 演示视频

A picture is worth a thousand words, but a video is worth a million! Check out our demo video on Bilibili.
*一个直观的视频胜过千言万语，请在B站查看我们的高清演示视频！*

<!-- 建议在这里放几张高质量的截图 -->
<p align="center">
  <video src=https://github.com/user-attachments/assets/9340300b-cf15-439b-be73-778a52ac6d4e  width="720px" controls preload></video>
  <img width="720" alt="Image" src="https://github.com/user-attachments/assets/6b2f00e5-2ce2-4a4b-8739-0c8507bdbafc" />
  <img width="720" alt="Image" src="https://github.com/user-attachments/assets/1072cf3e-e62b-4688-ae28-5f6e059596cd" />
</p>

---

## 🚀 Quick Start / 快速开始

### 1. Prerequisites / 依赖环境

Before you begin, please ensure your system environment is set up correctly:

*   **Unreal Engine:** Version 5.5 or newer.
*   **NVIDIA Omniverse Audio2Face:** Version `2023.2` is recommended. Ensure the service is running.
    *   **Tutorial / 参考教程:** [https://www.bilibili.com/video/BV1Vw4m1i79b/](https://www.bilibili.com/video/BV1Vw4m1i79b/)
    *   **Minimal Setup Steps / 最简设置步骤:**
        1.  Install Omniverse Launcher.
        2.  Install Audio2Face from the launcher.
        3.  Create a Nucleus service.
        4.  Download the sample model via Nucleus: `localhost/NVIDIA/Assets/Audio2Face/Samples_2023.2/blendshape_solve/`.
        5.  Run `audio2face_headless.bat`.
        6.  Configure via REST API (refer to `http://localhost:8011/docs` for details):
            *   **Set Model (`/A2F/USD/Load`):** Use the local path for fast loading.
              ```
              D:/blendshape_solve/claire_solved_arkit.usd
              ```
            *   **Set Driving Character (`/A2F/Player/SetRootPath`):**
              ```json
              {
                "a2f_player": "/World/audio2face/Player",
                "dir_path": "D:/audio2faceFiles/"
              }
              ```
            *   **Activate LiveLink (`/World/audio2face/StreamLivelink`).**
    > **Note / 注：** NVIDIA has announced the discontinuation of Omniverse services starting October 1, 2025. Please contact us for alternative solutions if needed.
    > *Nvidia官方公布自2025.10.1 omniverse各项服务将取消，如需帮助请联系我们提供解决方案。*
*   **Langchain:** Version `0.3.0` is used by default.
*   **Ollama (for local LLMs):** Required if you want to use local large language models.
*   **Microsoft Azure Account:** Required for Azure TTS/ASR services. You will need to obtain a subscription key and service region.

### 2. Plugin Installation / 插件安装

1.  **Download Release:** Go to the project's [Releases Page](链接到你的GitHub Release页面) to download the latest `.zip` file.
2.  **Extract to Project:** Unzip the downloaded folder into your UE project's `Plugins` directory (create one if it doesn't exist).
3.  **Restart Editor:** Restart your Unreal Engine editor and enable the plugin from the plugins list.

### 3. Configuration / 配置

After installation, you need to configure the plugin with your specific keys and endpoints.

*   **Project Settings:** Navigate to the plugin's configuration page in `Project Settings`. Enter your Audio2Face service address, Azure API Key, etc.

*   **⚠️ C++ Code Modification (Required) / C++代码修改 (必需):**
    For security reasons, some sensitive keys and URLs are hardcoded and need to be modified directly in the source code before compiling.
    > *出于安全考虑，部分敏感密钥和URL需要直接在C++源码中修改后编译。*

    *   **In `SpeechRecognition.cpp`:** Update your Azure Speech API key and region (in several places).
        ```cpp
        const FString SubscriptionKey = "YOUR_AZURE_KEY_HERE";
        const FString ServiceRegion = "YOUR_AZURE_REGION_HERE";
        ```
    *   **In `LangchainAPI.cpp`:** Update the endpoint for your LangChain service.
        ```cpp
        HttpRequest->SetURL("http://YOUR_LANGCHAIN_IP:7861/chat/kb_chat");
        ```
    *   **In `Audio2FaceRestApi.cpp`:** Verify or update the Audio2Face REST API endpoints if your setup is not on localhost.
        ```cpp
        FString URL = "http://localhost:8011/A2F/Player/SetTrack";
        // ... and other URLs in this file.
        ```

---

## 📖 Documentation & Tutorials / 文档与教程

We are working on providing detailed documentation and tutorials to help you get started and dive deeper.
*(我们正在努力提供详细的文档和教程，敬请期待！如有急需，请通过以下微信交流)*

---

## 💬 Community & Support / 社区与支持

We believe in the power of open collaboration! Join our community for discussions and support.
*我们相信开放与协作的力量！欢迎加入我们的社区进行交流和讨论。*

*   **WeChat Group / 微信交流群:**
    <br/>
    <img width="200" alt="Image" src="https://github.com/user-attachments/assets/3de7fec8-596c-4b8e-bc66-37e193290bf4" />
    <br/>
    *We will share the complete Blueprint project files, latest updates, and have in-depth technical discussions in the group.*
    >*我们会在群内分享完整的蓝图工程文件、最新的进展以及进行深入的技术探讨。*

---

## 📜 License / 开源许可证

This project is licensed under the **MIT License**. See the `LICENSE` file for details.
*本项目采用 **MIT License**。详情请见 `LICENSE` 文件。*

---

## About the Author / 关于作者

Hello, I'm **Yingjie Zhang**, a developer focusing on the fusion of AI and real-time graphics. This project is a result of my exploration in this field, and I hope it helps you.

In the future, I will be dedicated to developing a more advanced **3D Behavior Model** to truly grant "souls" to digital life. If you are interested in this direction, feel free to stay tuned!


*   **Bilibili:** [https://b23.tv/JpbMlw2](https://b23.tv/JpbMlw2)
*   **Email:** nxoor2022@gmail.com
