#include "SpeechRecognition.h"
#include "Runtime/Online/HTTP/Public/Http.h"
#include <speechapi_cxx.h>
#include <regex>
#include <string>

using namespace Microsoft::CognitiveServices::Speech;
using namespace Microsoft::CognitiveServices::Speech::Audio;

// 声明连续识别器变量
static std::shared_ptr<SpeechRecognizer> ContinuousRecognizer;
static std::shared_ptr<SpeechRecognizer> OnceSpeechRecognizer;
static std::shared_ptr<SpeechSynthesizer> textSynthesizeWaver;

static std::shared_ptr<SpeechConfig> speechConfigWav;
FTimerHandle TimeoutTimerHandle;
FString LastRecognizingText;  // 用于存储最近识别到的文本

FString currentFileName = "";
bool bIsRecognizing = false; // 添加一个标记，表示当前是否正在识别中

// 计时器以定期处理队列
FTimerHandle Audio2FaceTimerHandle;

static FCriticalSection AudioCriticalSection;

USpeechRecognition::USpeechRecognition()
{
    PrimaryComponentTick.bCanEverTick = false;
}
//连续语音识别
void USpeechRecognition::StartContinuousSpeechRecognition()
{
    // Azure Speech API订阅密钥和服务区域
    //S0
    const FString SubscriptionKey = "***";
    const FString ServiceRegion = "eastus";

    //F0
    /*const FString SubscriptionKey = "fbc8c18e3a8b43d59e8c8759490d325d";
    const FString ServiceRegion = "chinanorth3";*/

    // Azure语音配置
    auto config = SpeechConfig::FromSubscription(TCHAR_TO_UTF8(*SubscriptionKey), TCHAR_TO_UTF8(*ServiceRegion));
    // 设置语音识别的输入语言为中文
    config->SetSpeechRecognitionLanguage("zh-CN");

    // 指定MIC设备（最好是指定具体设备，识别与合成要分开，否则效率很低）
    auto audioConfig = AudioConfig::FromDefaultMicrophoneInput();
    //auto audioConfig = AudioConfig::FromMicrophoneInput("{0.0.1.00000000}.{93249018-7605-4649-b218-5f380349a833}");//家里
    //auto audioConfig = AudioConfig::FromMicrophoneInput("{0.0.1.00000000}.{c0854aeb-8853-47a2-af3a-6ba668ae953f}");//公司-kl
    //auto audioConfig = AudioConfig::FromMicrophoneInput("{0.0.1.00000000}.{8aec7ca8-dd6e-4995-b3b8-97a9df285341}");

    // 使用默认麦克风作为音频输入创建连续语音识别器
    ContinuousRecognizer = SpeechRecognizer::FromConfig(config, audioConfig);

    UE_LOG(LogTemp, Warning, TEXT("启动‘唤醒’语音识别..."));
 
    // 订阅 Recognizing 事件
    ContinuousRecognizer->Recognizing.Connect([this](const SpeechRecognitionEventArgs& args)
        {
            FString RecognizingText = FString(UTF8_TO_TCHAR(args.Result->Text.c_str()));

            // 将广播委托的执行移到主线程
            AsyncTask(ENamedThreads::GameThread, [this, RecognizingText]()
                {
                    OnSpeechRecognizing.Broadcast(RecognizingText);
                    LastRecognizingText = RecognizingText;

                    TArray<FString> Keywords = {
                        TEXT("张小婉"), TEXT("小婉小婉"), TEXT("小婉同学"), TEXT("你好小婉"),
                        TEXT("你好婉婉"), TEXT("婉婉在吗"), 
                    };

                    for (const FString& Keyword : Keywords)
                    {
                        if (RecognizingText.Contains(Keyword, ESearchCase::IgnoreCase))
                        {
                            // 发现关键词，触发委托
                            OnSpeechKeywordDetected.Broadcast(Keyword);
                            break;
                        }
                    }

                    // 每次识别到内容，重置定时器
                    GetWorld()->GetTimerManager().ClearTimer(TimeoutTimerHandle);
                    GetWorld()->GetTimerManager().SetTimer(
                        TimeoutTimerHandle,
                        [this]()
                        {
                            if (LastRecognizingText.IsEmpty())
                            {
                                UE_LOG(LogTemp, Warning, TEXT("如果你正在说话，可能当前网络有些不稳定。"));
                            }
                        },
                        5.0f,  // 5秒超时
                        false  // 不循环
                    );

                });

        });

    // 订阅 Canceled 事件
    ContinuousRecognizer->Canceled.Connect([this](const SpeechRecognitionCanceledEventArgs& e)
        {
            int32 ErrorCode = (int32)e.ErrorCode;
            FString ErrorDetails = FString(UTF8_TO_TCHAR(e.ErrorDetails.c_str()));

            // 将日志记录和可能的恢复操作移到主线程
            AsyncTask(ENamedThreads::GameThread, [ErrorCode, ErrorDetails]()
                {
                    UE_LOG(LogTemp, Warning, TEXT("CANCELED: ErrorCode=%d, ErrorDetails=%s"), ErrorCode, *ErrorDetails);
                    // 你可以在这里添加你的逻辑来处理取消事件
                });
        });

    // 订阅 SessionStopped 事件
    ContinuousRecognizer->SessionStopped.Connect([this](const SessionEventArgs& e)
        {
            // 将日志记录和可能的恢复操作移到主线程
            AsyncTask(ENamedThreads::GameThread, []
                {
                    UE_LOG(LogTemp, Warning, TEXT("Session stopped."));
                    // 你可以在这里添加你的逻辑来处理会话停止事件
                });
        });


    // 异步启动连续语音识别（注：如果需要，也可以像下边单次语音一样，把上边的ContinuousRecognizer初始化提出去）
    try
    {
        ContinuousRecognizer->StartContinuousRecognitionAsync().get();

    }
    catch (const std::exception& e)
    {
        UE_LOG(LogTemp, Error, TEXT("Exception during StartContinuousRecognitionAsync: %s"), *FString(e.what()));
    }
}

void USpeechRecognition::StopContinuousSpeechRecognition()
{
    // 检查连续识别器是否有效
    if (ContinuousRecognizer)
    {
        // 异步停止连续语音识别
        ContinuousRecognizer->StopContinuousRecognitionAsync().get();
        UE_LOG(LogTemp, Warning, TEXT("停止唤醒语音识别..."));
    }
}


FString FilterString(const FString& InputString)
{
    // 要过滤的字符：逗号、句号、空格、顿号、问号、冒号
    std::string CharactersToRemove = R"([, .、?：])";

    // 将FString转换为std::string
    std::string InputStdString(TCHAR_TO_UTF8(*InputString));

    // 使用正则表达式进行替换
    std::regex SpecialChars(CharactersToRemove);
    std::string FilteredStdString = std::regex_replace(InputStdString, SpecialChars, "");

    // 将std::string转换回FString
    return FString(FilteredStdString.c_str());
}


void USpeechRecognition::StartOnceSpeechRecognition()
{
    // 确保 OnceSpeechRecognizer 已经初始化
    if (!OnceSpeechRecognizer)
    {
        UE_LOG(LogTemp, Warning, TEXT("OnceSpeechRecognizer 尚未初始化，请先调用 InitializeSpeechRecognizer()"));
        return;
    }

    if (bIsRecognizing)
    {
        UE_LOG(LogTemp, Warning, TEXT("正在进行语音识别中，请稍后再试..."));
        return;
    }

    bIsRecognizing = true;

    // 解绑之前的绑定，防止重复绑定
    OnSpeechRecognitionResult.RemoveDynamic(this, &USpeechRecognition::OnSpeechRecognitionResultHandler);


    AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this]()
        {
            try
            {
                // 开始单次语音识别
                auto result = OnceSpeechRecognizer->RecognizeOnceAsync().get(); // 等待识别完成
                UE_LOG(LogTemp, Warning, TEXT("可以说话了..."));

                // 回到主线程中执行事件广播
                AsyncTask(ENamedThreads::GameThread, [this, result]()
                    {
                        bIsRecognizing = false; // 识别完成后重置标记

                        switch (result->Reason)
                        {
                        case ResultReason::RecognizedSpeech:
                        {
                            FString recognizedText = FString(UTF8_TO_TCHAR(result->Text.c_str()));

                            // 定义关键词集合
                            //TArray<FString> part1Keywords = { TEXT("请"), TEXT("吗"), TEXT("可以"), TEXT("能") };
                            TArray<FString> part2Keyword = { TEXT("换") };
                            TArray<FString> part3Keywords = { TEXT("衣服"), TEXT("装"), TEXT("人"), TEXT("角色"), TEXT("长相") };

                            bool isPart1Found = false, isPart2Found = false, isPart3Found = false;

                            // 检查关键词是否存在
                            /*for (const FString& keyword : part1Keywords) {
                                if (recognizedText.Contains(keyword)) {
                                    isPart1Found = true;
                                    break;
                                }
                            }*/

                            isPart2Found = recognizedText.Contains(part2Keyword[0]); // 假设“换”是必须的

                            for (const FString& keyword : part3Keywords) {
                                if (recognizedText.Contains(keyword)) {
                                    isPart3Found = true;
                                    break;
                                }
                            }

                            // 新增模型切换或动作模式关键词集合
                            TArray<FString> changeParams = { TEXT("可以"),TEXT("请"),TEXT("还是"),TEXT("切换"), TEXT("能"), TEXT("换一"), TEXT("进入") };
                            TArray<FString> actionKeywords = { TEXT("跳舞"), TEXT("跳个舞"),TEXT("跳支舞"),TEXT("跳一支舞"), TEXT("舞蹈"), TEXT("动作"), TEXT("模式"), TEXT("一个"), TEXT("一种"), TEXT("回去") };

                            bool isChangeParamFound = false, isActionKeywordFound = false;

                            // 检查模型切换或动作模式关键词是否存在
                            for (const FString& param : changeParams) {
                                if (recognizedText.Contains(param)) {
                                    isChangeParamFound = true;
                                }
                            }

                            for (const FString& actionKeyword : actionKeywords) {
                                if (recognizedText.Contains(actionKeyword)) {
                                    isActionKeywordFound = true;
                                }
                            }

                            // 新增购票关键词集合
                            TArray<FString> ticketChangeParams = { TEXT("可以"), TEXT("在线"), TEXT("现在"), TEXT("如何"), TEXT("我"),TEXT("能"), TEXT("怎么") };
                            TArray<FString> ticketKeywords = { TEXT("购票"), TEXT("买票"), TEXT("门票"), TEXT("票") };

                            bool isTicketChangeParamFound = false, isTicketKeywordFound = false;

                            // 检查购票关键词是否存在
                            for (const FString& param : ticketChangeParams) {
                                if (recognizedText.Contains(param)) {
                                    isTicketChangeParamFound = true;
                                    break;
                                }
                            }

                            for (const FString& ticketKeyword : ticketKeywords) {
                                if (recognizedText.Contains(ticketKeyword)) {
                                    isTicketKeywordFound = true;
                                    break;
                                }
                            }

                            // 新增安装关键词集合
                            TArray<FString> installParams = { TEXT("把"), TEXT("可以"), TEXT("安装"), TEXT("能"), TEXT("如何"), TEXT("怎么") };
                            TArray<FString> deviceKeywords = { TEXT("手机"), TEXT("你"), TEXT("姗姗") };

                            bool isInstallParamFound = false, isDeviceKeywordFound = false;

                            // 检查安装关键词是否存在
                            for (const FString& param : installParams) {
                                if (recognizedText.Contains(param)) {
                                    isInstallParamFound = true;
                                    break;
                                }
                            }

                            for (const FString& deviceKeyword : deviceKeywords) {
                                if (recognizedText.Contains(deviceKeyword)) {
                                    isDeviceKeywordFound = true;
                                    break;
                                }
                            }

                            // 如果关键词组合成立，则发送换装标识
                            if (isPart2Found && isPart3Found) {
                                OnSpeechRecognitionResult.Broadcast(TEXT("[replacement]"));
                            }
                            else if (isChangeParamFound && isActionKeywordFound) {
                                // 发送切换动作模式的标识
                                OnSpeechRecognitionResult.Broadcast(TEXT("[model_change_or_action]"));
                            }
                            else if(isTicketChangeParamFound && isTicketKeywordFound){ 
                                //购票
                                OnSpeechRecognitionResult.Broadcast(FString(UTF8_TO_TCHAR(result->Text.c_str())));
                                OnSpeechRecognitionShow.Broadcast(TEXT("[Ticket QR code]"));
                            }
                            else if (isInstallParamFound && isDeviceKeywordFound) {
                                // 发送安装标识
                                OnSpeechRecognitionResult.Broadcast(FString(UTF8_TO_TCHAR(result->Text.c_str())));
                                OnSpeechRecognitionShow.Broadcast(TEXT("[installation]"));
                            }
                            else {
                                OnSpeechRecognitionResult.Broadcast(FString(UTF8_TO_TCHAR(result->Text.c_str())));
                            }
                            break;
                        }
                        case ResultReason::NoMatch:
                            OnSpeechRecognitionResult.Broadcast("[no rec]");
                            UE_LOG(LogTemp, Warning, TEXT("-------------[no rec]-------------"));
                            break;
                        case ResultReason::Canceled:
                        {
                            auto cancellation = CancellationDetails::FromResult(result);
                            UE_LOG(LogTemp, Warning, TEXT("CANCELED: Reason=%d"), static_cast<int>(cancellation->Reason));

                            OnSpeechRecognitionNet.Broadcast(TEXT("[NETERROR]"));
                            UE_LOG(LogTemp, Warning, TEXT("-------------[NETERROR - switch:Canceled]-------------"));
                            if (cancellation->Reason == CancellationReason::Error)
                            {
                                UE_LOG(LogTemp, Warning, TEXT("CANCELED: ErrorCode=%d"), static_cast<int>(cancellation->ErrorCode));
                                UE_LOG(LogTemp, Warning, TEXT("CANCELED: ErrorDetails=%s"), *FString(UTF8_TO_TCHAR(cancellation->ErrorDetails.c_str())));
                                UE_LOG(LogTemp, Warning, TEXT("CANCELED: 检查是否设置了语音资源密钥和区域值。"));
                            }
                            break;
                        }
                        default:
                            UE_LOG(LogTemp, Warning, TEXT("-------------[NETERROR - switch:default]-------------"));
                            OnSpeechRecognitionNet.Broadcast(TEXT("[NETERROR]"));
                            break;
                        }
                    });
            }
            catch (const std::exception& e)
            {
                // 捕获并记录异常信息
                UE_LOG(LogTemp, Error, TEXT("Exception: %s"), *FString(e.what()));

                // 确保标记被重置
                AsyncTask(ENamedThreads::GameThread, [this]()
                    {
                        bIsRecognizing = false;
                    });
            }
        });
}

void USpeechRecognition::OnSpeechRecognitionResultHandler(const FString& Result)
{
    // 处理语音识别结果的逻辑，例如：
    UE_LOG(LogTemp, Log, TEXT("Recognized Text: %s"), *Result);
}

void USpeechRecognition::StopOnceSpeechRecognition()
{
    // 解绑事件
    OnSpeechRecognitionResult.RemoveDynamic(this, &USpeechRecognition::OnSpeechRecognitionResultHandler);
}
//单次语音识别初始化（回答问题）
void USpeechRecognition::InitializeSpeechRecognizer()
{
    // Azure Speech API订阅密钥和服务区域
    //S0
    const FString SubscriptionKey = "***";
    const FString ServiceRegion = "eastus";

    //F0
    /*const FString SubscriptionKey = "fbc8c18e3a8b43d59e8c8759490d325d";
    const FString ServiceRegion = "chinanorth3";*/

    // Azure语音配置
    auto config = SpeechConfig::FromSubscription(TCHAR_TO_UTF8(*SubscriptionKey), TCHAR_TO_UTF8(*ServiceRegion));
    // 设置语音识别的输入语言为中文
    config->SetSpeechRecognitionLanguage("zh-CN");

    // 指定MIC设备（最好是指定具体设备，识别与合成要分开，否则可能效率很低）
    auto audioConfig = AudioConfig::FromDefaultMicrophoneInput();
    //auto audioConfig = AudioConfig::FromMicrophoneInput("{0.0.1.00000000}.{1fe107a1-ace2-4373-bde7-4297634b0122}");//家里
    //auto audioConfig = AudioConfig::FromMicrophoneInput("{0.0.1.00000000}.{c0854aeb-8853-47a2-af3a-6ba668ae953f}");//公司-kl
    //auto audioConfig = AudioConfig::FromMicrophoneInput("{0.0.1.00000000}.{8aec7ca8-dd6e-4995-b3b8-97a9df285341}");

    OnceSpeechRecognizer = SpeechRecognizer::FromConfig(config, audioConfig);
    UE_LOG(LogTemp, Warning, TEXT("启动单次语音识别初始化..."));

}



void USpeechRecognition::TextSynthesizeWav(const FString& TextToSynthesize, const FString& FileName)
{
    //后台线程，避免在主线程运行造成场景卡住
    AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this, TextToSynthesize, FileName]()
        {
            const FString OutputFilePath = FString::Printf(TEXT("D:/audio2faceFiles/%s"), *FileName);
            // 创建 AudioConfig 对象，指定输出到.wav文件
            auto audioConfig = AudioConfig::FromWavFileOutput(TCHAR_TO_UTF8(*OutputFilePath));

            // 实例化 SpeechSynthesizer
            textSynthesizeWaver = SpeechSynthesizer::FromConfig(speechConfigWav, audioConfig);

            // 合成语音并写入文件（这是一个异步操作，注意完成后的情况变化）
            UE_LOG(LogTemp, Warning, TEXT(">>>>>>>>> 语音合成开始执行: %s"), *FileName);
            auto result = textSynthesizeWaver->SpeakTextAsync(*TextToSynthesize).get();
            //UE_LOG(LogTemp, Warning, TEXT(">>>>>>>>> 后台开始写入文件: %s"), *FileName); //不能这么写，因为合成成功与否还不知道。写入到硬盘是第二步； 
            
            if (result->Reason == ResultReason::SynthesizingAudioCompleted)
            {
                UE_LOG(LogTemp, Warning, TEXT("语音合成成功完成，开始保存文件：%s"), *FileName);
                bool bFileAvailable = false;
                int32 MaxAttempts = 10;
                int32 AttemptCount = 0;

                // 循环检查文件可用性，最多10次
                while (!bFileAvailable && AttemptCount < MaxAttempts)
                {
                    if (FPaths::FileExists(OutputFilePath) && FPlatformFileManager::Get().GetPlatformFile().FileSize(*OutputFilePath) > 0)
                    {
                        bFileAvailable = true;
                    }
                    else
                    {
                        AttemptCount++;
                        FPlatformProcess::Sleep(0.5f); // 等待1秒后再次尝试
                    }
                }

                if (bFileAvailable)
                {
                    UE_LOG(LogTemp, Warning, TEXT("语音合成文件已确认有效且完整可用: %s"), *OutputFilePath);
                    textSynthesizeWaver = nullptr; // 安全释放资源
                    OnTextSynthesisComplete.Broadcast(FileName);
                    
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("在10次尝试后，语音合成文件仍不可用或未生成完整: %s"), *OutputFilePath);
                }
            }
            else if (result->Reason == ResultReason::Canceled)
            {
                auto cancellation = SpeechSynthesisCancellationDetails::FromResult(result);
                FString ErrorMessage = FString::Printf(TEXT("语音合成被取消。取消原因: %d"), (int)cancellation->Reason);
                UE_LOG(LogTemp, Error, TEXT("%s"), *ErrorMessage);
                if (cancellation->Reason == CancellationReason::Error)
                {
                    UE_LOG(LogTemp, Error, TEXT("错误代码: %d, 错误详情: %s"), (int)cancellation->ErrorCode, *FString(cancellation->ErrorDetails.c_str()));
                }
            }

        });
}
//多了一个textSynthesizeWaver = nullptr;尝试生成文件后能直接播放
void USpeechRecognition::TextSynthesizeWav_0(const FString& TextToSynthesize, const FString& FileName)
{
    //后台线程，避免在主线程运行造成场景卡住
    AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this, TextToSynthesize, FileName]()
        {
            const FString OutputFilePath = FString::Printf(TEXT("D:/audio2faceFiles/%s"), *FileName);
            // 创建 AudioConfig 对象，指定输出到.wav文件
            auto audioConfig = AudioConfig::FromWavFileOutput(TCHAR_TO_UTF8(*OutputFilePath));

            // 实例化 SpeechSynthesizer
            textSynthesizeWaver = SpeechSynthesizer::FromConfig(speechConfigWav, audioConfig);

            // 合成语音并写入文件（这是一个异步操作，注意完成后的情况变化）
            auto result = textSynthesizeWaver->SpeakTextAsync(*TextToSynthesize).get();
            //UE_LOG(LogTemp, Warning, TEXT(">>>>>>>>> 后台开始写入文件: %s"), *FileName);

            //直接调用委托，后边在蓝图中处理异步生成的wav文件
            //OnTextSynthesisComplete.Broadcast(FileName);

            //这个SynthesisCompleted会丢掉一部分记录，不知道怎么回事，暂时先不用
            textSynthesizeWaver->SynthesisCompleted.Connect([this, FileName](const SpeechSynthesisEventArgs& eventArgs)
                {
                    //FString synthesisSizeStr = FString::FromInt(eventArgs.Result->GetAudioData()->size());
                    UE_LOG(LogTemp, Warning, TEXT("TextSynthesizeWav SynthesisCompleted: %s"), *FileName);
                    // 调用委托，通知已注册的事件监听器
                    OnTextSynthesisComplete.Broadcast(FileName);
                    // 将 textSynthesizeWaver 指针设置为 nullptr，释放资源
                    // 注意，下边这样写是有问题的，SynthesisCompleted完成的是合成动作，不等于文件生成保存动作；
                    textSynthesizeWaver = nullptr;
                });

        });
}
//语音文件合成初始化
void USpeechRecognition::InitializeTextSynthesizeWav()
{
    // 从Azure获取的订阅密钥和服务区域
    //S0
    const FString SubscriptionKey = "***";
    const FString ServiceRegion = "eastus";

    //F0
    /*const FString SubscriptionKey = "fbc8c18e3a8b43d59e8c8759490d325d";
    const FString ServiceRegion = "chinanorth3";*/

    // 创建 SpeechConfig 对象
    speechConfigWav = SpeechConfig::FromSubscription(TCHAR_TO_UTF8(*SubscriptionKey), TCHAR_TO_UTF8(*ServiceRegion));
    //speechConfigWav->SetSpeechSynthesisVoiceName("zh-CN-YunxiNeural");
    speechConfigWav->SetSpeechSynthesisVoiceName("zh-CN-XiaoxiaoMultilingualNeural");
    //speechConfigWav->SetSpeechSynthesisVoiceName("zh-CN-XiaoyanNeural");
    //speechConfigWav->SetSpeechSynthesisVoiceName("zh-CN-YunzeNeural");
    //speechConfigWav->SetSpeechSynthesisVoiceName("zh-CN-YunjieNeural");
    //speechConfigWav->SetSpeechSynthesisVoiceName("zh-CN-XiaoshuangNeural");
}

void USpeechRecognition::Audio2Face()
{

    FString URL = "http://localhost:8011/A2F/Player/Play";

    // Build HTTP request
    TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetVerb("POST");
    HttpRequest->SetURL(URL);
    HttpRequest->SetHeader(TEXT("accept"), TEXT("application/json"));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

    // Build request data
    FString JsonPayload = FString::Printf(
        TEXT("{\"a2f_player\": \"/World/audio2face/Player\"}")
    );

    HttpRequest->SetContentAsString(JsonPayload);

    // 设置回调函数以处理HTTP响应
    HttpRequest->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
        {
            if (bSuccess && Response.IsValid())
            {
                // 处理HTTP响应
                FString ResponseContent = Response->GetContentAsString();
                //UE_LOG(LogTemp, Warning, TEXT("HTTP响应成功: %s"), *ResponseContent);
                UE_LOG(LogTemp, Warning, TEXT("********************  完成Audio2Face **************************"));

                // 解析服务器返回的信息，获取音频时长
                //float AudioDuration = ParseAudioDurationInfo(ResponseContent);

                // 设置异步计时器，在音频时长后释放锁
                //FTimerHandle TimerHandle;
                //GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this, ResponseContent]()
                //    {
                //        // 你可能需要将数据传递给蓝图，可以通过蓝图暴露的委托或事件进行通知
                //        OnAudio2FaceResponse_0.Broadcast(ResponseContent);

                //    }, AudioDuration, false);
                OnAudio2FaceResponse_0.Broadcast(ResponseContent);

            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("HTTP请求失败"));

                // 在这里处理HTTP请求失败的情况

                // 你也可以将失败信息传递给蓝图
                OnAudio2FaceRequestFailed.Broadcast();
            }
        });
    // 发送HTTP请求
    HttpRequest->ProcessRequest();

}

float USpeechRecognition::ParseAudioDurationInfo(const FString& ResponseContent)
{
    // 在这里解析服务器返回的信息，获取音频时长
    // 你需要根据具体的服务器响应格式和音频时长字段来实现这个函数
    // 返回音频时长，单位可能是秒或毫秒，根据实际情况进行转换
    // 这里假设返回的是秒
    float AudioDuration = 0.0f;

    // 解析 ResponseContent，提取音频时长字段到 AudioDuration

    return AudioDuration;
}

void USpeechRecognition::SetA2FTrack(const FString& FileName)
{
    FString URL = "http://localhost:8011/A2F/Player/SetTrack";

    // Build HTTP request
    TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetVerb("POST");
    HttpRequest->SetURL(URL);
    HttpRequest->SetHeader(TEXT("accept"), TEXT("application/json"));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

    // Build request data
    FString JsonPayload = FString::Printf(
        TEXT("{\"a2f_player\": \"/World/audio2face/Player\", \"file_name\": \"%s\"}"),
        *FileName
    );

    HttpRequest->SetContentAsString(JsonPayload);

    // 设置回调函数以处理HTTP响应
    HttpRequest->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
        {
            if (bSuccess && Response.IsValid())
            {
                // 处理HTTP响应
                FString ResponseContent = Response->GetContentAsString();
                if (ResponseContent != "Internal Server Error") {
                    //UE_LOG(LogTemp, Warning, TEXT("HTTP响应成功: %s"), *ResponseContent);
                    UE_LOG(LogTemp, Warning, TEXT("++++++++++++++++ 完成设置新track ++++++++++++++++++++++"));
                    //Audio2Face();
                }
                else {
                    UE_LOG(LogTemp, Warning, TEXT("HTTP响应失败:Internal Server Error"));
                }
                // 在这里处理服务器返回的数据

                // 你可能需要将数据传递给蓝图，可以通过蓝图暴露的委托或事.件进行通知
                OnAudio2FaceResponse.Broadcast(ResponseContent);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("HTTP请求失败"));

                // 在这里处理HTTP请求失败的情况

                // 你也可以将失败信息传递给蓝图
                OnAudio2FaceRequestFailed.Broadcast();
            }
        });
    // 发送HTTP请求
    HttpRequest->ProcessRequest();
}

//获取当前Track的时间长度
void USpeechRecognition::GetA2FRange()
{
    FString URL = "http://localhost:8011/A2F/Player/GetRange";

    // Build HTTP request
    TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetVerb("POST");
    HttpRequest->SetURL(URL);
    HttpRequest->SetHeader(TEXT("accept"), TEXT("application/json"));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

    // Build request data
    FString JsonPayload = FString::Printf(
        TEXT("{\"a2f_player\": \"/World/audio2face/Player\"}")
    );

    HttpRequest->SetContentAsString(JsonPayload);

    // 设置回调函数以处理HTTP响应
    HttpRequest->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
        {
            if (bSuccess && Response.IsValid())
            {
                // 处理HTTP响应
                FString ResponseContent = Response->GetContentAsString();
                if (ResponseContent != "Internal Server Error") {
                    //UE_LOG(LogTemp, Warning, TEXT("HTTP响应成功: %s"), *ResponseContent);
                    UE_LOG(LogTemp, Warning, TEXT("++++++++++++++++ 完成设置新GetA2FRange ++++++++++++++++++++++"));

                    // 解析服务器返回的 JSON 数据
                    TSharedPtr<FJsonObject> JsonObject;
                    TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(ResponseContent);

                    if (FJsonSerializer::Deserialize(JsonReader, JsonObject))
                    {
                        // 提取 "result" 字段
                        TSharedPtr<FJsonObject> ResultObject = JsonObject->GetObjectField("result");

                        if (ResultObject.IsValid())
                        {
                            // 提取 "default" 标签中的数组
                            TArray<FString> DefaultArray;
                            if (ResultObject->TryGetStringArrayField("default", DefaultArray))
                            {
                                // 默认取第二个值，即索引为 1 的值
                                if (DefaultArray.Num() > 1)
                                {
                                    FString AudioDurationString = DefaultArray.Num() > 1 ? DefaultArray[1] : FString();
                                    float AudioDuration = FCString::Atof(*AudioDurationString);
                                    //UE_LOG(LogTemp, Warning, TEXT("获取到音频时长: %f"), AudioDuration);

                                    // 这里你可以将音频时长传递给蓝图，通过委托或事件进行通知
                                    OnAudioDurationReceived.Broadcast(AudioDuration);
                                }
                                else
                                {
                                    UE_LOG(LogTemp, Error, TEXT("无法获取音频时长，数组元素不足"));
                                }
                            }
                            else
                            {
                                UE_LOG(LogTemp, Error, TEXT("无法获取音频时长，缺少 'default' 标签"));
                            }
                        }
                        else
                        {
                            UE_LOG(LogTemp, Error, TEXT("无法获取音频时长，缺少 'result' 字段"));
                        }
                    }
                    else
                    {
                        UE_LOG(LogTemp, Error, TEXT("无法解析服务器返回的 JSON 数据"));
                    }


                }
                else {
                    UE_LOG(LogTemp, Warning, TEXT("HTTP响应失败:Internal Server Error"));
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("HTTP请求失败"));

                // 在这里处理HTTP请求失败的情况

                // 你也可以将失败信息传递给蓝图
                OnAudio2FaceRequestFailed.Broadcast();
            }
        });
    // 发送HTTP请求
    HttpRequest->ProcessRequest();
}
//停止正在播放的音频，一般是在打断操作中起作用
void USpeechRecognition::A2FPause()
{

    FString URL = "http://localhost:8011/A2F/Player/Pause";

    // Build HTTP request
    TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetVerb("POST");
    HttpRequest->SetURL(URL);
    HttpRequest->SetHeader(TEXT("accept"), TEXT("application/json"));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

    // Build request data
    FString JsonPayload = FString::Printf(
        TEXT("{\"a2f_player\": \"/World/audio2face/Player\"}")
    );

    HttpRequest->SetContentAsString(JsonPayload);

    // 设置回调函数以处理HTTP响应
    HttpRequest->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
        {
            if (bSuccess && Response.IsValid())
            {
                // 处理HTTP响应
                FString ResponseContent = Response->GetContentAsString();
                //UE_LOG(LogTemp, Warning, TEXT("HTTP响应成功: %s"), *ResponseContent);
                UE_LOG(LogTemp, Warning, TEXT("------------------------  暂停Audio2Face播放 -------------------------"));

            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("HTTP请求失败"));
            }
        });
    // 发送HTTP请求
    HttpRequest->ProcessRequest();

}