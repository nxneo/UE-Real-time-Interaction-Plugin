#include "A2FRestForWait.h"
#include "Runtime/Online/HTTP/Public/Http.h"
#include <speechapi_cxx.h>

UA2FRestForWait::UA2FRestForWait()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UA2FRestForWait::Audio2FaceForwait()
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

float UA2FRestForWait::ParseAudioDurationInfoForwait(const FString& ResponseContent)
{
    // 在这里解析服务器返回的信息，获取音频时长
    // 你需要根据具体的服务器响应格式和音频时长字段来实现这个函数
    // 返回音频时长，单位可能是秒或毫秒，根据实际情况进行转换
    // 这里假设返回的是秒
    float AudioDuration = 0.0f;

    // 解析 ResponseContent，提取音频时长字段到 AudioDuration

    return AudioDuration;
}

void UA2FRestForWait::SetA2FTrackForwait(const FString& FileName)
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

                // 你可能需要将数据传递给蓝图，可以通过蓝图暴露的委托或事件进行通知
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
void UA2FRestForWait::GetA2FRangeForwait()
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

void UA2FRestForWait::A2FPauseForwait()
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

