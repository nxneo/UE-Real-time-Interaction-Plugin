#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Http.h"
#include "UpDressRestAPI.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUpdressResponseDelegate, const FString&, dress_id, const FString&, created_at);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUpdressRequestFailedDelegate, const FString&, ErrorMessage);

UCLASS(ClassGroup = (Django), meta = (BlueprintSpawnableComponent))
class NXAZURESPEECH_API UUpDressRestAPI : public UActorComponent
{
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    UUpDressRestAPI();

    UFUNCTION(BlueprintCallable, Category = "Up Dress API")
    void GetUpDressSet(int32 ProjectId);

    UPROPERTY(BlueprintAssignable)
    FUpdressResponseDelegate OnUpdressResponse;

    UPROPERTY(BlueprintAssignable)
    FUpdressRequestFailedDelegate OnUpdressRequestFailed;

protected:
    virtual void BeginPlay() override;

private:
    void HandleRequestComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);
};
