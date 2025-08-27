// Fill out your copyright notice in the Description page of Project Settings.


#include "QuestionAnswerRestApi.h"

// Sets default values for this component's properties
UQuestionAnswerRestApi::UQuestionAnswerRestApi()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = false;

    // ...
}


void UQuestionAnswerRestApi::BeginPlay()
{
    Super::BeginPlay();
}
//移动版获取答案，从一个中间件服务器中；
void UQuestionAnswerRestApi::GetAndDeleteEarliestAnswer(int32 ProjectId)
{
    FString Url = TEXT("http://192.168.110.237:8000/restAPI/get-earliest-answer/");

    TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetVerb(TEXT("POST"));
    HttpRequest->SetURL(Url);
    HttpRequest->SetHeader(TEXT("accept"), TEXT("application/json"));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    // Build request data
    FString JsonPayload = FString::Printf(
        TEXT("{\"project_id\": %d}"), ProjectId

    );

    HttpRequest->SetContentAsString(JsonPayload);

    HttpRequest->OnProcessRequestComplete().BindUObject(this, &UQuestionAnswerRestApi::HandleRequestComplete);

    HttpRequest->ProcessRequest();
}

void UQuestionAnswerRestApi::HandleRequestComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
{
    if (bSuccess && Response.IsValid() && Response->GetContentLength() > 0)
    {
        FString Content = Response->GetContentAsString();
        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(Content);

        if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
        {
            if (JsonObject->HasField(TEXT("question")) && JsonObject->HasField(TEXT("answer")))
            {
                FString Question = JsonObject->GetStringField(TEXT("question"));
                FString Answer = JsonObject->GetStringField(TEXT("answer"));

                OnQuestionAnswerResponse.Broadcast(Question, Answer);
            }
            else
            {
                OnQuestionAnswerRequestFailed.Broadcast(TEXT("question or answer field not found in JSON response"));
            }

        }
        else
        {
            OnQuestionAnswerRequestFailed.Broadcast(TEXT("Invalid JSON response"));
        }
    }
    else
    {
        OnQuestionAnswerRequestFailed.Broadcast(TEXT("Request failed"));
    }
}
