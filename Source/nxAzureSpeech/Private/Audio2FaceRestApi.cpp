#include "Audio2FaceRestApi.h"
#include "Runtime/Online/HTTP/Public/Http.h"
#include <speechapi_cxx.h>

UAudio2FaceRestApi::UAudio2FaceRestApi()
{
    PrimaryComponentTick.bCanEverTick = true;
}
//播放
void UAudio2FaceRestApi::RestAudio2Face(bool bIsWakeUp)
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
    // 先解绑之前绑定的委托
    HttpRequest->OnProcessRequestComplete().Unbind();


    // 设置回调函数以处理HTTP响应
    HttpRequest->OnProcessRequestComplete().BindLambda([this, bIsWakeUp](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
        {
            if (bSuccess && Response.IsValid())
            {
                // 处理HTTP响应
                FString ResponseContent = Response->GetContentAsString();
                //UE_LOG(LogTemp, Warning, TEXT("HTTP响应成功: %s"), *ResponseContent);
                UE_LOG(LogTemp, Warning, TEXT("********************  完成Audio2Face **************************"));
                // 获取音频时长并延迟处理
                //RestGetA2FRange_C([this, ResponseContent, bIsWakeUp](float AudioDuration)
                //    {
                //        FTimerHandle TimerHandle;
                //        GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this, ResponseContent, bIsWakeUp]()
                //            {
                //                
                //                // 根据是否为唤醒应答进行不同的处理
                //                if (bIsWakeUp)
                //                {
                //                    OnAudio2FaceResponse_0_wakeup.Broadcast(ResponseContent);
                //                }
                //                else
                //                {
                //                    UE_LOG(LogTemp, Warning, TEXT("********************  开始绑定 **************************"));
                //                    OnAudio2FaceResponse_0.Broadcast(ResponseContent);
                //                }
                //            }, AudioDuration, false);
                //    });
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("HTTP请求失败"));
                // 你也可以将失败信息传递给蓝图
                OnAudio2FaceRequestFailed.Broadcast();
            }
        });
    UE_LOG(LogTemp, Warning, TEXT("++++++++++++++++ 准备调取 GetA2FRange_C ++++++++++++++++++++++"));
    RestGetA2FRange_C([this, bIsWakeUp](float AudioDuration)
        {
            // 在游戏线程中设置定时器
            AsyncTask(ENamedThreads::GameThread, [this, bIsWakeUp, AudioDuration]()
                {
                UE_LOG(LogTemp, Warning, TEXT("++++++++++++++++ 时间调用完成，启动延迟 ++++++++++++++++++++++"));
                FTimerHandle TimerHandle;
                FDateTime CallbackStartTime = FDateTime::UtcNow();
                UE_LOG(LogTemp, Warning, TEXT("定时器回调开始时间: %s"), *CallbackStartTime.ToString());
                GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this, CallbackStartTime, bIsWakeUp]()
                    {
                        FString ResponseContent = "";
                        // 根据是否为唤醒应答进行不同的处理
                        if (bIsWakeUp)
                        {
                            OnAudio2FaceResponse_0_wakeup.Broadcast(ResponseContent);
                        }
                        else
                        {
                            UE_LOG(LogTemp, Warning, TEXT("********************  延迟结束，开始绑定 **************************"));
                            OnAudio2FaceResponse_0.Broadcast(ResponseContent);
                        }

                        FDateTime CallbackEndTime = FDateTime::UtcNow();
                        UE_LOG(LogTemp, Warning, TEXT("定时器回调结束时间: %s"), *CallbackEndTime.ToString());
                        FTimespan CallbackDuration = CallbackEndTime - CallbackStartTime;
                        UE_LOG(LogTemp, Warning, TEXT("定时器回调持续时间: %f 秒"), CallbackDuration.GetTotalSeconds());
                    }, AudioDuration, false);
                });
        });

    // 发送HTTP请求
    HttpRequest->ProcessRequest();
}
// 在关卡切换前清理定时器
void UAudio2FaceRestApi::CleanupBeforeLevelChange()
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
        UE_LOG(LogTemp, Warning, TEXT("定时器已清理"));
    }
}
// 获取音频时长
void UAudio2FaceRestApi::RestGetA2FRange_C(TFunction<void(float)> Callback)
{
    FString URL = "http://localhost:8011/A2F/Player/GetRange";

    // Build HTTP request
    TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetVerb("POST");
    HttpRequest->SetURL(URL);
    HttpRequest->SetHeader(TEXT("accept"), TEXT("application/json"));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

    // Build request data
    FString JsonPayload = FString::Printf(TEXT("{\"a2f_player\": \"/World/audio2face/Player\"}"));

    HttpRequest->SetContentAsString(JsonPayload);

    // 先解绑之前绑定的委托
    HttpRequest->OnProcessRequestComplete().Unbind();

    // 记录开始时间
    FDateTime StartTime = FDateTime::UtcNow();

    // 设置回调函数以处理HTTP响应
    HttpRequest->OnProcessRequestComplete().BindLambda([this, Callback, StartTime](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
        {
            if (bSuccess && Response.IsValid())
            {
                // 记录结束时间
                FDateTime EndTime = FDateTime::UtcNow();
                FTimespan ElapsedTime = EndTime - StartTime;
                float ElapsedSeconds = ElapsedTime.GetTotalSeconds();

                // 处理HTTP响应
                FString ResponseContent = Response->GetContentAsString();
                if (ResponseContent != "Internal Server Error")
                {
                    UE_LOG(LogTemp, Warning, TEXT("++++++++++++++++ 完成GetA2FRange_C ++++++++++++++++++++++"));

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
                                    UE_LOG(LogTemp, Warning, TEXT("获取到音频时长: %f"), AudioDuration);
                                    UE_LOG(LogTemp, Warning, TEXT("获取到音频时长的处理处理: %f"), ElapsedSeconds);
                                    UE_LOG(LogTemp, Warning, TEXT("应延迟时间: %f"), AudioDuration - ElapsedSeconds);
                                    // 回调传递音频时长，减去获取时长花费的时间
                                    Callback(AudioDuration - ElapsedSeconds);
                                }
                                else
                                {
                                    UE_LOG(LogTemp, Error, TEXT("无法获取音频时长，数组元素不足"));
                                    Callback(0.0f);
                                }
                            }
                            else
                            {
                                UE_LOG(LogTemp, Error, TEXT("无法获取音频时长，缺少 'default' 标签"));
                                Callback(0.0f);
                            }
                        }
                        else
                        {
                            UE_LOG(LogTemp, Error, TEXT("无法获取音频时长，缺少 'result' 字段"));
                            Callback(0.0f);
                        }
                    }
                    else
                    {
                        UE_LOG(LogTemp, Error, TEXT("无法解析服务器返回的 JSON 数据"));
                        Callback(0.0f);
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("HTTP响应失败: Internal Server Error"));
                    Callback(0.0f);
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("HTTP请求失败"));
                Callback(0.0f);
            }
        });

    // 发送HTTP请求
    HttpRequest->ProcessRequest();
}

float UAudio2FaceRestApi::RestParseAudioDurationInfo(const FString& ResponseContent)
{
    // 在这里解析服务器返回的信息，获取音频时长
    // 你需要根据具体的服务器响应格式和音频时长字段来实现这个函数
    // 返回音频时长，单位可能是秒或毫秒，根据实际情况进行转换
    // 这里假设返回的是秒
    float AudioDuration = 0.0f;

    // 解析 ResponseContent，提取音频时长字段到 AudioDuration

    return AudioDuration;
}
//设置音频文件
void UAudio2FaceRestApi::RestSetA2FTrack(const FString& FileName, bool bIsWakeUp)
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

    // 解绑之前绑定的委托
    HttpRequest->OnProcessRequestComplete().Unbind();

    // 设置回调函数以处理HTTP响应
    HttpRequest->OnProcessRequestComplete().BindLambda([this, bIsWakeUp](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
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
                // 根据是否为唤醒应答进行不同的处理
                if (bIsWakeUp)
                {
                    OnWakeUpResponse.Broadcast(ResponseContent);
                }
                else
                {
                    OnAudio2FaceResponse.Broadcast(ResponseContent);
                }

                // 你可能需要将数据传递给蓝图，可以通过蓝图暴露的委托或事件进行通知
                //OnAudio2FaceResponse.Broadcast(ResponseContent);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("HTTP请求失败"));

                // 根据是否为唤醒应答进行不同的失败处理
                if (bIsWakeUp)
                {
                    OnWakeUpRequestFailed.Broadcast();
                }
                else
                {
                    OnAudio2FaceRequestFailed.Broadcast();
                }
            }
        });
    // 发送HTTP请求
    HttpRequest->ProcessRequest();
}

//获取当前Track的时间长度，
//注：因为在蓝图中受硬件性能影响执行计时不准，暂时不用这个了，使用上边的C++版
void UAudio2FaceRestApi::RestGetA2FRange(bool bIsWakeUp)
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
    // 先解绑之前绑定的委托
    HttpRequest->OnProcessRequestComplete().Unbind();

    // 设置回调函数以处理HTTP响应
    HttpRequest->OnProcessRequestComplete().BindLambda([this, bIsWakeUp](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
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
                                    if (bIsWakeUp)
                                    {
                                        OnAudioDurationReceived_wakeup.Broadcast(AudioDuration);
                                    }
                                    else
                                    {
                                        OnAudioDurationReceived.Broadcast(AudioDuration);
                                    }
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

void UAudio2FaceRestApi::RestA2FPause()
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

