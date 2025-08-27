// Fill out your copyright notice in the Description page of Project Settings.


#include "ChatglmAPI.h"
#include "Runtime/Online/HTTP/Public/Http.h"
#include "../json.hpp"

// Sets default values for this component's properties
UChatglmAPI::UChatglmAPI()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = true;

    // ...
}
//流式响应组句
FString words_sentence = "";
//设置一个记录一次回答需要生成的文件数
int fileNameCounter = 0;
int currentCounter = 0;//配合计数用

//执行聊天
void UChatglmAPI::PerformChatCompletion(const FString& UserContent)
{
    //再次调用时需要初始化参数
    words_sentence = "";
    fileNameCounter = 0;
    currentCounter = 0;
    // 构建HTTP请求
    TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetVerb("POST");
    HttpRequest->SetURL("http://*.*.*.*:28082/v1/chat/completions");
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

    //选择数据流式响应：
    // false:完整接收，一般答案长度超过10个字，完整接收时间会比较长，影响用户体验； 
    // true:流式响应，逐字或逐词推给语音合成，要保证语音合成频繁调用的效率，否则字、词之间的时间长也会影响体验；
    auto stream = true;



    // 构建请求数据
    FString JsonPayload = FString::Printf(
        TEXT("{\"model\": \"chatglm3-6b\", \"messages\": [{\"role\": \"system\", \"content\": \"你的名字叫玥珊珊, 极目AI团队开发的大语言模型. Follow the user's instructions carefully. Respond using markdown.\"}, {\"role\": \"user\", \"content\": \"%s\"}], \"stream\": %s, \"max_tokens\": 100, \"temperature\": 0.8, \"top_p\": 0.8}"),
        *UserContent,
        stream ? TEXT("true") : TEXT("false")
    );
    /*FString JsonPayload = FString::Printf(
        TEXT("{\"model\": \"chatglm3-6b\", \"messages\": [{\"role\": \"system\", \"content\": \"你是名字是ChatKL-7B，是快来AI试验室基于ChatGLM3-6B训练的中文语言大模型, Follow the user's instructions carefully. Respond using markdown.\"}, {\"role\": \"user\", \"content\": \"%s\"}], \"stream\": %s, \"max_tokens\": 100, \"temperature\": 0.8, \"top_p\": 0.8}"),
        *UserContent,
        stream ? TEXT("true") : TEXT("false")
    );*/

    HttpRequest->SetContentAsString(JsonPayload);

    //HttpRequest->OnRequestProgress().BindLambda([this, stream](FHttpRequestPtr Request, int32 BytesSent, int32 BytesReceived)
    HttpRequest->OnRequestProgress().BindLambda([=](FHttpRequestPtr HttpRequest, int32 BytesSent, int32 BytesReceived)
        {

            // 获取响应
            FHttpResponsePtr Response = HttpRequest->GetResponse();
            FString ChunkedContent;
            //流式响应，注：Response.IsValid()一定要判断，OnRequestProgress()过程会产生Response空指针
            if (stream && Response.IsValid()) //流式响应
            {
                //FString streamCompletionResult = Response->GetContentAsString();

                TArray<FString> ContentLines;
                Response->GetContentAsString().ParseIntoArrayLines(ContentLines);

                int lineSize = ContentLines.Num();

                FString streamCompletionResult = ContentLines[lineSize - 1];

                //UE_LOG(LogTemp, Warning, TEXT("Chat stream result_: %s"), *streamCompletionResult);
                if (streamCompletionResult.StartsWith(TEXT("data: ")))
                {
                    // 移除前缀 "data: "
                    ChunkedContent = streamCompletionResult.RightChop(6);
                    //UE_LOG(LogTemp, Warning, TEXT("Stream data: %s"), *ChunkedContent);
                    //UE_LOG(LogTemp, Warning, TEXT("******************"));
                    // 检查是否为结束标记
                    if (ChunkedContent.Equals(TEXT("[DONE]")))
                    {
                        //UE_LOG(LogTemp, Warning, TEXT("End of stream."));
                        //触发答案结束委托(这里操作可能会因为ParseJson还没有执行完，导致[DONE]被提前处理)
                        //OnChatCompletion.Broadcast("[DONE]");
                        //currentCounter++;//最后生成的文件有时目前是被锁定的，主要是为了解锁最后一个正常文件； 

                        //这里的currentCounter将用于蓝图中数组的游标，所以从0开始的。而fileNameCounter是wav文件的总数量，体现的数值要currentCounter+1
                        fileNameCounter = currentCounter + 1;
                        OnChatCompletion.Broadcast("[DONE]", fileNameCounter, currentCounter);
                        words_sentence = "";

                    }
                    else {
                        // 解析 JSON 数据
                        ParseJson(ChunkedContent);
                    }

                }

            }
            else
            {
                //非流式响应在OnProcessRequestComplete()中处理
                //ParseJson_(ChunkedContent);
            }
        });

    // 发送HTTP请求
    HttpRequest->ProcessRequest();
}
//流式解析，组成一句一句推送
void UChatglmAPI::ParseJson(const FString& ChunkedContent)
{
    UE_LOG(LogTemp, Warning, TEXT("Parsing JSON: %s"), *ChunkedContent);
    try
    {
        nlohmann::json JsonObject = nlohmann::json::parse(TCHAR_TO_UTF8(*ChunkedContent));

        // 检查 object 字段是否为 chat.completion.chunk
        if (JsonObject["object"] == "chat.completion.chunk")
        {
            // 提取 "choices" 字段
            auto ChoicesArray = JsonObject["choices"];
            //UE_LOG(LogTemp, Warning, TEXT("ChoicesArray length: %d"), ChoicesArray.size());

            for (const auto& Choice : ChoicesArray)
            {
                // 检查 "delta" 和 "finish_reason" 字段是否存在
                if (Choice.contains("delta") && Choice.contains("finish_reason"))
                {
                    // 提取 "content" 字段
                    FString Content = FString(UTF8_TO_TCHAR(Choice["delta"]["content"].get<std::string>().c_str()));
                    UE_LOG(LogTemp, Warning, TEXT("Choice Content: %s"), *Content);

                    // 检查 Content 是否为逗号或句号
                    if (Content.Equals(TEXT(",")) || Content.Equals(TEXT("，")) || Content.Equals(TEXT("。")))
                    {
                        // 如果 Content 是逗号或句号，则广播拼接的句子，并清空变量以便下一次迭代
                        words_sentence += Content;
                        // 在words_sentence中去除空格和换行符
                        words_sentence = words_sentence.Replace(TEXT(" "), TEXT("")).Replace(TEXT("\n"), TEXT("")).Replace(TEXT("\r"), TEXT(""));

                        OnChatCompletion.Broadcast(words_sentence, fileNameCounter, currentCounter);
                        currentCounter++;
                        words_sentence = "";
                    }
                    else
                    {
                        // 如果 Content 不是逗号或句号，则将其拼接到句子中
                        words_sentence += Content;
                    }


                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Choice is missing required fields (delta or finish_reason)."));
                }
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Object is not 'chat.completion.chunk'."));
        }
    }
    catch (const std::exception& e)
    {
        UE_LOG(LogTemp, Error, TEXT("JSON parsing error: %s"), UTF8_TO_TCHAR(e.what()));
    }
}


//非流式解析（注：与流式响应处理的json参数不同）
void UChatglmAPI::ParseJson_(const FString& ChunkedContent)
{
    //UE_LOG(LogTemp, Warning, TEXT("Parsing JSON: %s"), *ChunkedContent);
    try
    {
        nlohmann::json JsonObject = nlohmann::json::parse(TCHAR_TO_UTF8(*ChunkedContent));

        // 检查 object 字段是否为 chat.completion.chunk
        if (JsonObject["object"] == "chat.completion")
        {
            // 提取 "choices" 字段
            auto ChoicesArray = JsonObject["choices"];
            //UE_LOG(LogTemp, Warning, TEXT("ChoicesArray length: %d"), ChoicesArray.size());

            for (const auto& Choice : ChoicesArray)
            {
                // 检查 "delta" 和 "finish_reason" 字段是否存在
                if (Choice.contains("message") && Choice.contains("finish_reason"))
                {
                    // 提取 "content" 字段
                    FString Content = FString(UTF8_TO_TCHAR(Choice["message"]["content"].get<std::string>().c_str()));
                    UE_LOG(LogTemp, Warning, TEXT("Choice Content: %s"), *Content);
                    // 触发委托
                    OnChatCompletion.Broadcast(Content, fileNameCounter, currentCounter);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Choice is missing required fields (delta or finish_reason)."));
                }
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Object is not 'chat.completion.chunk'."));
        }
    }
    catch (const std::exception& e)
    {
        UE_LOG(LogTemp, Error, TEXT("JSON parsing error: %s"), UTF8_TO_TCHAR(e.what()));
    }
}
