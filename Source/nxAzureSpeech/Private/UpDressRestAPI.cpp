#include "UpDressRestAPI.h"

UUpDressRestAPI::UUpDressRestAPI()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UUpDressRestAPI::BeginPlay()
{
    Super::BeginPlay();
}

void UUpDressRestAPI::GetUpDressSet(int32 ProjectId)
{
    UE_LOG(LogTemp, Warning, TEXT("启动换装调用..."));
    FString Url = TEXT("http://192.168.45.48:8000/restAPI/get-dress-up/");

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

    HttpRequest->OnProcessRequestComplete().BindUObject(this, &UUpDressRestAPI::HandleRequestComplete);

    HttpRequest->ProcessRequest();
}

void UUpDressRestAPI::HandleRequestComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
{
    if (bSuccess && Response.IsValid() && Response->GetContentLength() > 0)
    {
        FString Content = Response->GetContentAsString();
        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(Content);

        if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
        {
            if (JsonObject->HasField(TEXT("dress_id")) && JsonObject->HasField(TEXT("created_at")))
            {
                FString dress_id = JsonObject->GetStringField(TEXT("dress_id"));
                FString created_at = JsonObject->GetStringField(TEXT("created_at"));

                OnUpdressResponse.Broadcast(dress_id, created_at);
            }
            else
            {
                OnUpdressRequestFailed.Broadcast(TEXT("dress_id or created_at field not found in JSON response"));
            }
        }
        else
        {
            OnUpdressRequestFailed.Broadcast(TEXT("Invalid JSON response"));
        }
    }
    else
    {
        OnUpdressRequestFailed.Broadcast(TEXT("Request failed"));
    }
}
